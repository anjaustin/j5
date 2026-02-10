/**
 * RISC-V Guardian: ESP32 Anomaly Detector
 *
 * Receives streaming telemetry from ESP32 via UART1.
 * Runs CfC neural network to learn normal patterns.
 * Detects anomalies when ESP32 is perturbed.
 *
 * Hardware: SiFive HiFive1 Rev B01 (E31 RISC-V core)
 * UART1: Connected to ESP32 UART1 TX
 */

#include <stdint.h>
#include <string.h>

#define MAGIC_BYTE     0xAA
#define TELEMETRY_SIZE 12
#define WINDOW_SIZE    64

// ============================================================
// Register Definitions (FE310-G002)
// ============================================================

// GPIO
#define GPIO_BASE      0x10012000
#define GPIO_IOF_EN    (*(volatile uint32_t *)(GPIO_BASE + 0x38))
#define GPIO_IOF_SEL   (*(volatile uint32_t *)(GPIO_BASE + 0x3C))

// UART0 (Console)
#define UART0_BASE     0x10013000
#define UART0_TXDATA   (*(volatile uint32_t *)(UART0_BASE + 0x00))
#define UART0_RXDATA   (*(volatile uint32_t *)(UART0_BASE + 0x04))
#define UART0_TXCTRL   (*(volatile uint32_t *)(UART0_BASE + 0x08))
#define UART0_RXCTRL   (*(volatile uint32_t *)(UART0_BASE + 0x0C))
#define UART0_IE       (*(volatile uint32_t *)(UART0_BASE + 0x10))
#define UART0_IP       (*(volatile uint32_t *)(UART0_BASE + 0x14))
#define UART0_DIV      (*(volatile uint32_t *)(UART0_BASE + 0x18))

// UART1 (Telemetry)
#define UART1_BASE     0x10023000
#define UART1_TXDATA   (*(volatile uint32_t *)(UART1_BASE + 0x00))
#define UART1_RXDATA   (*(volatile uint32_t *)(UART1_BASE + 0x04))
#define UART1_TXCTRL   (*(volatile uint32_t *)(UART1_BASE + 0x08))
#define UART1_RXCTRL   (*(volatile uint32_t *)(UART1_BASE + 0x0C))
#define UART1_IE       (*(volatile uint32_t *)(UART1_BASE + 0x10))
#define UART1_IP       (*(volatile uint32_t *)(UART1_BASE + 0x14))
#define UART1_DIV      (*(volatile uint32_t *)(UART1_BASE + 0x18))

// ============================================================
// Globals
// ============================================================

volatile uint8_t g_rx_buf[TELEMETRY_SIZE];
volatile uint8_t g_rx_idx = 0;
volatile uint8_t g_rx_state = 0;

volatile uint32_t g_rx_count = 0;
volatile uint32_t g_rx_errors = 0;

typedef struct {
    uint8_t magic;
    uint32_t timestamp_ms;
    uint32_t free_heap;
    uint8_t cpu_load;
    uint16_t tx_rate;
} telemetry_t;

typedef struct {
    uint32_t timestamp;
    uint32_t heap;
    uint8_t cpu;
    uint16_t tx_rate;
} feature_t;

static feature_t g_window[WINDOW_SIZE];
static uint8_t g_window_idx = 0;
static uint32_t g_sample_count = 0;

typedef struct {
    uint64_t pos_mask;
    uint64_t neg_mask;
} neuron_t;

static neuron_t g_neurons[64];
static uint8_t g_outputs[64];

#define PREACT_OFFSET  32
#define PREACT_MAX     64

static const uint8_t sigmoid_lut[33] = {
    0,  0,  0,  0,  0,  0,  0,  1,  1,  1,
    1,  2,  2,  3,  3,  4,  5,  6,  7,  8,
    9, 10, 12, 13, 15, 17, 19, 21, 24, 26,
    29, 32, 36
};

// ============================================================
// Utilities
// ============================================================

static void *g_memset(void *s, int c, size_t n) {
    uint8_t *p = s;
    for (size_t i = 0; i < n; i++) p[i] = (uint8_t)c;
    return s;
}

static void *g_memcpy(void *dest, const void *src, size_t n) {
    uint8_t *d = dest;
    const uint8_t *s = src;
    for (size_t i = 0; i < n; i++) d[i] = s[i];
    return dest;
}

static uint32_t popcount64(uint64_t x) {
    uint32_t c = 0;
    while (x) {
        c++;
        x &= x - 1;
    }
    return c;
}

// ============================================================
// UART Output (debug)
// ============================================================

void uart0_putc(char c) {
    while (UART0_TXDATA & 0x80000000) {} // Wait for TX FIFO not full
    UART0_TXDATA = (uint8_t)c;
}

void uart0_puts(const char *s) {
    while (*s) uart0_putc(*s++);
}

void uart0_puthex(uint32_t v) {
    for (int i = 28; i >= 0; i -= 4) {
        uart0_putc("0123456789ABCDEF"[(v >> i) & 0xF]);
    }
}

void uart0_putdec(uint32_t v) {
    char buf[12];
    int i = 0;
    if (v == 0) {
        uart0_putc('0');
        return;
    }
    while (v > 0) {
        buf[i++] = '0' + (v % 10);
        v /= 10;
    }
    while (i--) uart0_putc(buf[i]);
}

// ============================================================
// Feature Extraction & CfC
// ============================================================

static void extract_features(telemetry_t *tm, feature_t *f) {
    f->timestamp = tm->timestamp_ms;
    f->heap = tm->free_heap;
    f->cpu = tm->cpu_load;
    f->tx_rate = tm->tx_rate;
}

static void add_to_window(feature_t *f) {
    g_window[g_window_idx] = *f;
    g_window_idx = (g_window_idx + 1) % WINDOW_SIZE;
    g_sample_count++;
}

static void cfc_forward(void) {
    feature_t *f = &g_window[(g_window_idx + WINDOW_SIZE - 1) % WINDOW_SIZE];

    uint64_t input_val = 0;
    input_val |= ((uint64_t)f->heap & 0xFF) << 0;
    input_val |= ((uint64_t)f->heap >> 8 & 0xFF) << 8;
    input_val |= ((uint64_t)f->cpu & 0xFF) << 16;
    input_val |= ((uint64_t)f->tx_rate & 0xFFFF) << 24;

    uint32_t threshold = sigmoid_lut[PREACT_OFFSET];

    for (int n = 0; n < 64; n++) {
        uint64_t pos = g_neurons[n].pos_mask;
        uint64_t neg = g_neurons[n].neg_mask;

        uint8_t pos_ct = popcount64(input_val & pos);
        uint8_t neg_ct = popcount64(input_val & neg);

        int pre_act = (int)pos_ct - (int)neg_ct;
        int idx = pre_act + PREACT_OFFSET;
        if (idx < 0) idx = 0;
        if (idx > PREACT_MAX) idx = PREACT_MAX;

        g_outputs[n] = sigmoid_lut[idx] >= threshold ? 1 : 0;
    }
}

static void hebbian_learn(void) {
    feature_t *f = &g_window[(g_window_idx + WINDOW_SIZE - 1) % WINDOW_SIZE];

    uint64_t input_val = 0;
    input_val |= ((uint64_t)f->heap & 0xFF) << 0;
    input_val |= ((uint64_t)f->heap >> 8 & 0xFF) << 8;
    input_val |= ((uint64_t)f->cpu & 0xFF) << 16;
    input_val |= ((uint64_t)f->tx_rate & 0xFFFF) << 24;

    for (int n = 0; n < 64; n++) {
        if (g_outputs[n] == 1) {
            g_neurons[n].pos_mask |= (input_val & 0xFF);
            g_neurons[n].neg_mask &= ~(input_val & 0xFF);
        } else {
            g_neurons[n].pos_mask &= ~(input_val & 0xFF);
            g_neurons[n].neg_mask |= (input_val & 0xFF);
        }
    }
}

static uint32_t detect_anomaly(void) {
    feature_t *f = &g_window[(g_window_idx + WINDOW_SIZE - 1) % WINDOW_SIZE];

    if (f->cpu > 80) return 2;
    if (f->heap < 120000) return 2;
    if (f->tx_rate > 5000) return 2;

    if (f->cpu > 30) return 1;
    if (f->heap < 140000) return 1;
    if (f->tx_rate > 2000) return 1;

    return 0;
}

// ============================================================
// Initialization
// ============================================================

static void gpio_init(void) {
    // 16(0x10000), 17(0x20000), 18(0x40000), 23(0x800000)
    uint32_t mask = 0x870000; 
    
    // Enable IOF
    GPIO_IOF_EN |= mask;
    
    // Select IOF0 (UART) - clear bits
    GPIO_IOF_SEL &= ~mask;
}

static void uart_init(void) {
    // Divisor 157 for 115200 baud at 18.125 MHz
    uint32_t div = 157;
    
    UART0_DIV = div;
    UART0_TXCTRL = 1; // TXEN
    UART0_RXCTRL = 1; // RXEN

    UART1_DIV = div;
    UART1_TXCTRL = 1; // TXEN
    UART1_RXCTRL = 1; // RXEN
}

// ============================================================
// Main
// ============================================================

int main(void) {
    gpio_init();
    uart_init();

    g_memset(g_neurons, 0, sizeof(g_neurons));

    uart0_puts("\r\n========================================\r\n");
    uart0_puts("  RISC-V Guardian v1.0\r\n");
    uart0_puts("  ESP32 Anomaly Detector\r\n");
    uart0_puts("  Clock: ~18.125 MHz\r\n");
    uart0_puts("========================================\r\n\r\n");

    uart0_puts("Waiting for telemetry from ESP32...\r\n");

    telemetry_t tm;
    feature_t f;

    while (1) {
        // Poll UART1
        uint32_t val = UART1_RXDATA;
        if (!(val & 0x80000000)) { // Not empty
            uint8_t c = (uint8_t)(val & 0xFF);
            
            // State machine
             switch (g_rx_state) {
                case 0:
                    if (c == MAGIC_BYTE) {
                        g_rx_buf[0] = c;
                        g_rx_idx = 1;
                        g_rx_state = 1;
                    }
                    break;
                case 1:
                    g_rx_buf[g_rx_idx++] = c;
                    if (g_rx_idx >= TELEMETRY_SIZE) {
                        g_rx_idx = 0;
                        g_rx_state = 0;
                        g_rx_count++;
                    }
                    break;
                default:
                    g_rx_state = 0;
                    g_rx_idx = 0;
                    break;
            }
        }

        if (g_rx_count > 0) {
            g_rx_count = 0;
            g_memcpy(&tm, (void*)g_rx_buf, TELEMETRY_SIZE);

            extract_features(&tm, &f);
            add_to_window(&f);

            if (g_sample_count > 128) {
                cfc_forward();
                hebbian_learn();
                uint32_t anomaly = detect_anomaly();

                if (g_sample_count % 100 == 0) {
                    uart0_puts("[");
                    uart0_putdec(g_sample_count);
                    uart0_puts("] H:");
                    uart0_puthex(f.heap);
                    uart0_puts(" C:");
                    uart0_putdec(f.cpu);
                    uart0_puts("% T:");
                    uart0_putdec(f.tx_rate);
                    uart0_puts(" -> ");

                    if (anomaly == 0) uart0_puts("NORMAL");
                    else if (anomaly == 1) uart0_puts("WARN");
                    else uart0_puts("ANOMALY!");

                    uart0_puts("\r\n");
                }
            } else if (g_sample_count % 100 == 0) {
                 uart0_puts("Learning... [");
                 uart0_putdec(g_sample_count);
                 uart0_puts("/128]\r\n");
            }
        }
    }

    return 0;
}
