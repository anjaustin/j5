/**
 * CfC Neural Network Demo for ESP32-S0WD
 *
 * Closed-form Continuous-time neural network using POPCOUNT operations.
 * Based on EntroMorphic's Reflex OS CfC implementation.
 *
 * Key concepts:
 * - Ternary weights: {-1, 0, +1} encoded as bit masks
 * - Binary activations: {0, 1}
 * - Pure POPCOUNT operations - NO multiply, NO floating point
 * - LUT-based sigmoid activation
 *
 * v2.0: Fixed sigmoid LUT, added 64-bit support, added benchmarking
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "soc/uart_reg.h"
#include "soc/dport_reg.h"

static const char *TAG = "CFC";

// Configuration
#define CFC_NUM_NEURONS      64
#define CFC_INPUT_BITS       64
#define CFC_OUTPUT_BITS      64

// Pre-activation range: [-64, +64] for full CfC
#define CFC_PREACT_OFFSET    64
#define CFC_PREACT_MAX       128

// Sigmoid LUT: sigmoid(x) = 255 / (1 + exp(-x/8)), scaled for 8-bit output
// Maps pre-activation [-64..+64] to activation values [0..255]
static const uint8_t sigmoid_lut[CFC_PREACT_MAX + 1] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  1,  1,  1,  1,  1,
    2,  2,  2,  3,  3,  4,  4,  5,  6,  7,
    8,  9, 10, 11, 13, 14, 16, 18, 20, 22,
   25, 27, 30, 33, 37, 40, 44, 49, 53, 58,
   64, 69, 75, 82, 88, 95, 102,109,117,124,
  132,139,147,154,162,169,176,183,189,195,
  201,206,212,217,221,226,230,234,237,240,
  243,245,248,250,252,253,254,254,255,255,
  255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255
};

// Ternary weight storage (bit-packed, 64-bit)
// One neuron with pos/neg masks for all input bits
typedef struct {
    uint64_t pos_mask;   // Bits where weight = +1
    uint64_t neg_mask;   // Bits where weight = -1
} cfc_neuron_t;

// Array of neurons for CfC layer
typedef struct {
    cfc_neuron_t neurons[CFC_NUM_NEURONS];
} cfc_layer_t;

// ============================================================
// ESP32 Cycle Counter (for benchmarking)
// ============================================================

static inline uint32_t cycles_start(void) {
    uint32_t ccount;
    asm volatile ("rsr.ccount %0" : "=r"(ccount));
    return ccount;
}

static inline uint32_t cycles_end(uint32_t start) {
    uint32_t ccount;
    asm volatile ("rsr.ccount %0" : "=r"(ccount));
    return ccount - start;
}

// ============================================================
// UART output (direct register access with timing)
// ============================================================

void uart_putchar(char c) {
    volatile uint32_t *txfifo = (volatile uint32_t *)0x3FF40000;
    volatile uint32_t *stat = (volatile uint32_t *)0x3FF4001C;
    while (*stat & (1 << 23)) {}
    *txfifo = (uint8_t)c;
}

void uart_puts(const char *s) {
    while (*s) {
        uart_putchar(*s++);
    }
}

void uart_putchar_delayed(char c) {
    volatile uint32_t *txfifo = (volatile uint32_t *)0x3FF40000;
    volatile uint32_t *stat = (volatile uint32_t *)0x3FF4001C;
    while (*stat & (1 << 23)) {}
    *txfifo = (uint8_t)c;
    for (volatile int i = 0; i < 50; i++) {}
}

void uart_hex32(uint32_t v) {
    for (int i = 28; i >= 0; i -= 4) {
        uart_putchar("0123456789ABCDEF"[(v >> i) & 0xF]);
    }
}

void uart_bin32(uint32_t v) {
    for (int i = 31; i >= 0; i--) {
        uart_putchar((v >> i) & 1 ? '1' : '0');
    }
}

void uart_dec32(uint32_t v) {
    char buf[12];
    int i = 0;
    if (v == 0) {
        uart_putchar('0');
        return;
    }
    while (v > 0) {
        buf[i++] = '0' + (v % 10);
        v /= 10;
    }
    while (i--) {
        uart_putchar(buf[i]);
    }
}

// ============================================================
// CfC Forward Pass (v2.0 - 64-bit, 64 neurons, with benchmarking)
//
// Each neuron computes:
//   pre_act = popcount(input & pos_mask) - popcount(input & neg_mask)
//   output = sigmoid(pre_act) > sigmoid(0)
// ============================================================

uint32_t cfc_forward_bench(cfc_layer_t *layer, uint64_t input_val, uint8_t *output) {
    uint32_t start = cycles_start();

    for (int n = 0; n < CFC_NUM_NEURONS; n++) {
        uint64_t pos = layer->neurons[n].pos_mask;
        uint64_t neg = layer->neurons[n].neg_mask;

        uint8_t pos_ct = __builtin_popcountll(input_val & pos);
        uint8_t neg_ct = __builtin_popcountll(input_val & neg);

        int pre_act = pos_ct - neg_ct;

        int lut_idx = pre_act + CFC_PREACT_OFFSET;
        if (lut_idx < 0) lut_idx = 0;
        if (lut_idx > CFC_PREACT_MAX) lut_idx = CFC_PREACT_MAX;

        output[n] = sigmoid_lut[lut_idx] >= sigmoid_lut[CFC_PREACT_OFFSET] ? 1 : 0;
    }

    return cycles_end(start);
}

// ============================================================
// Initialize weights for pattern detection
// Each neuron detects a different bit position in the pattern
// Pattern: 0xAAAAAAAAAAAAAAAA = 101010... (64 bits)
// ============================================================

void init_pattern_weights(cfc_layer_t *layer) {
    memset(layer, 0, sizeof(cfc_layer_t));

    for (int n = 0; n < CFC_NUM_NEURONS; n++) {
        layer->neurons[n].pos_mask = 0xAAAAAAAAAAAAAAAAULL;
        layer->neurons[n].neg_mask = 0x5555555555555555ULL;
    }
}

// ============================================================
// Random weight initialization (sparse ternary)
// Each neuron gets random sparse weights
// Sparsity: ~10% non-zero (5% positive, 5% negative)
// ============================================================

void init_random_weights(cfc_layer_t *layer, uint32_t seed) {
    uint32_t state = seed;

    for (int n = 0; n < CFC_NUM_NEURONS; n++) {
        uint64_t pos = 0, neg = 0;

        for (int i = 0; i < CFC_INPUT_BITS; i++) {
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;
            uint32_t r = state % 100;

            if (r < 5) {
                pos |= (1ULL << i);
            } else if (r < 10) {
                neg |= (1ULL << i);
            }
        }

        layer->neurons[n].pos_mask = pos;
        layer->neurons[n].neg_mask = neg;
    }
}

// ============================================================
// Hebbian Learning Step
// "Neurons that fire together, wire together"
// For each neuron:
//   If input_bit[i] == 1 AND output[n] == 1: strengthen positive weight
//   If input_bit[i] == 1 AND output[n] == 0: strengthen negative weight
//   If input_bit[i] == 0: no change (anti-hebbian)
// ============================================================

void cfc_hebbian_step(cfc_layer_t *layer, uint64_t input_val, uint8_t *output) {
    for (int n = 0; n < CFC_NUM_NEURONS; n++) {
        if (output[n] == 0) continue;

        uint64_t pos = layer->neurons[n].pos_mask;
        uint64_t neg = layer->neurons[n].neg_mask;

        for (int i = 0; i < CFC_INPUT_BITS; i++) {
            uint64_t bit = 1ULL << i;
            uint8_t in_bit = (input_val & bit) ? 1 : 0;

            if (in_bit) {
                if ((pos & bit) == 0 && (neg & bit) == 0) {
                    uint32_t r = xTaskGetTickCount() % 2;
                    if (r == 0) {
                        pos |= bit;
                    } else {
                        neg |= bit;
                    }
                }
            }
        }

        layer->neurons[n].pos_mask = pos;
        layer->neurons[n].neg_mask = neg;
    }
}

// ============================================================
// Anti-Hebbian Learning Step
// Weaken connections where input is 1 but neuron doesn't fire
// ============================================================

void cfc_anti_hebbian_step(cfc_layer_t *layer, uint64_t input_val, uint8_t *output) {
    for (int n = 0; n < CFC_NUM_NEURONS; n++) {
        if (output[n] == 1) continue;

        uint64_t pos = layer->neurons[n].pos_mask;
        uint64_t neg = layer->neurons[n].neg_mask;

        for (int i = 0; i < CFC_INPUT_BITS; i++) {
            uint64_t bit = 1ULL << i;
            uint8_t in_bit = (input_val & bit) ? 1 : 0;

            if (in_bit) {
                pos &= ~bit;
                neg &= ~bit;
            }
        }

        layer->neurons[n].pos_mask = pos;
        layer->neurons[n].neg_mask = neg;
    }
}

// ============================================================
// Main Application
// ============================================================

void app_main(void) {
    cfc_layer_t layer;
    uint8_t output[CFC_NUM_NEURONS];
    uint32_t total_cycles = 0;
    uint32_t num_samples = 0;
    uint32_t learning_count = 0;

    uart_puts("\r\n========================================\r\n");
    uart_puts("  CfC Neural Network Demo v2.0\r\n");
    uart_puts("  ESP32-S0WD - 64 neurons, POPCOUNT only\r\n");
    uart_puts("  Features: Benchmarking + Hebbian Learning\r\n");
    uart_puts("========================================\r\n\r\n");

    ESP_LOGI(TAG, "CfC Demo v2.0 Started");
    ESP_LOGI(TAG, "Neurons: %d", CFC_NUM_NEURONS);
    ESP_LOGI(TAG, "Input bits: %d", CFC_INPUT_BITS);

    init_pattern_weights(&layer);

    uart_puts("Pattern: 0b");
    uart_bin32(0xAAAAAAAA);
    uart_puts(" (0xAAAAAAAA)\r\n");
    uart_puts("64 neurons detect alternating 1/0 pattern\r\n");
    uart_puts("Hebbian learning: strengthen synapses on activation\r\n");
    uart_puts("Benchmarking: cycles per forward pass\r\n\r\n");

    uint32_t test_count = 0;
    uint32_t bench_interval = 100;
    uint32_t last_bench_print = 0;

    while (1) {
        uint64_t input_val = test_count++;

        uint32_t cycles = cfc_forward_bench(&layer, input_val, output);
        total_cycles += cycles;
        num_samples++;
        last_bench_print++;

        uint8_t active = 0;
        for (int i = 0; i < CFC_NUM_NEURONS; i++) {
            if (output[i]) active++;
        }

        if (learning_count < 1000) {
            cfc_hebbian_step(&layer, input_val, output);
            cfc_anti_hebbian_step(&layer, input_val, output);
            learning_count++;
        }

        uart_puts("In: 0b");
        uart_bin32((uint32_t)input_val);
        uart_puts(" -> ");

        for (int i = 0; i < CFC_NUM_NEURONS; i++) {
            uart_putchar_delayed(output[i] ? '#' : '.');
        }

        uart_puts(" (");
        uart_dec32(active);
        uart_puts("/64)");

        if (learning_count < 1000) {
            uart_puts(" [L:");
            uart_dec32(learning_count);
            uart_puts("]");
        }

        if (last_bench_print >= bench_interval) {
            uart_puts(" | Cycles: ");
            uart_dec32(cycles);
            uart_puts(" | Avg: ");
            uart_dec32(total_cycles / num_samples);
            last_bench_print = 0;
        }

        uart_puts("\r\n");

        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}
