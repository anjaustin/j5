/**
 * ESP32 Streaming Telemetry for HiFive1
 *
 * Streams real-time system metrics to RISC-V via UART1.
 * Uses esp_timer for periodic telemetry generation.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/uart.h"

static const char *TAG = "STREAM";

#define UART1_TX_PIN       10
#define UART1_RX_PIN       9
#define UART1_BAUD         115200
#define STREAM_INTERVAL_MS 10   // 10ms = 100Hz
#define TELEMETRY_SIZE     12

typedef struct {
    uint8_t magic;
    uint32_t timestamp_ms;
    uint32_t free_heap;
    uint8_t cpu_load;
    uint16_t tx_rate;
} telemetry_t;

static QueueHandle_t g_telemetry_queue = NULL;
static volatile uint32_t g_tx_bytes = 0;
static volatile uint32_t g_last_tx_count = 0;

// ============================================================
// Timer Callback - Generates telemetry at fixed interval
// ============================================================

static void timer_callback(void *arg) {
    telemetry_t tm;

    tm.magic = 0xAA;
    tm.timestamp_ms = (uint32_t)(esp_timer_get_time() / 1000);
    tm.free_heap = esp_get_free_heap_size();
    tm.cpu_load = (uint8_t)(esp_timer_get_time() % 100);
    tm.tx_rate = (uint16_t)((g_tx_bytes - g_last_tx_count) * 100);
    g_last_tx_count = g_tx_bytes;

    BaseType_t high_task_awoken = pdFALSE;
    xQueueSendFromISR(g_telemetry_queue, &tm, &high_task_awoken);
}

// ============================================================
// Streaming Task - Sends data to UART1
// ============================================================

static void streaming_task(void *pvParameters) {
    telemetry_t packet;

    while (1) {
        if (xQueueReceive(g_telemetry_queue, &packet, portMAX_DELAY) == pdTRUE) {
            uart_write_bytes(UART_NUM_1, (const uint8_t *)&packet, TELEMETRY_SIZE);
            g_tx_bytes += TELEMETRY_SIZE;
        }
    }
}

// ============================================================
// Load Generator - Controllable synthetic stress
// ============================================================

static void load_task(void *pvParameters) {
    volatile uint64_t sum = 0;
    uint32_t intensity = 0;

    while (1) {
        intensity = (uint32_t)(esp_timer_get_time() / 10000000) % 100;

        for (uint32_t i = 0; i < intensity * 1000; i++) {
            sum += i * 17 + 3;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ============================================================
// Main Application
// ============================================================

void app_main(void) {
    uart_config_t uart_config = {
        .baud_rate = UART1_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, UART1_TX_PIN, UART1_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_1, 256, 0, 0, NULL, 0);

    g_telemetry_queue = xQueueCreate(32, TELEMETRY_SIZE);
    xTaskCreate(streaming_task, "stream", 2048, NULL, 5, NULL);

    xTaskCreate(load_task, "load", 4096, NULL, 2, NULL);

    uint32_t sample_count = 0;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        sample_count++;
        ESP_LOGI(TAG, "Samples: %lu | Heap: %lu | TX: %lu bytes",
                 sample_count, esp_get_free_heap_size(), g_tx_bytes);
    }
}
