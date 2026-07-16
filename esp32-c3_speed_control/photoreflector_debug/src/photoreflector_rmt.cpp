#ifdef USE_RMT

#include "photoreflector.hpp"
#include <driver/rmt.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/ringbuf.h>
#include <esp_log.h>

namespace PhotoReflector {

static const rmt_channel_t RMT_CHANNEL = RMT_CHANNEL_0;
static RingbufHandle_t rb = NULL;

// internal state
static volatile unsigned long last_pulse_us = 0;
static volatile unsigned long pulse_interval_us = 0;
static volatile unsigned long pulse_count = 0;
static TaskHandle_t processing_task_handle = nullptr;
static int prev_level = 0; // assume low initially
static bool rmt_active = false;
static volatile unsigned long rmt_batches = 0;

void setProcessingTaskHandle(TaskHandle_t h) { processing_task_handle = h; }

void setup() {
  Serial.println("PhotoReflector::setup(): start");
  rmt_config_t rmt_cfg = {};
  rmt_cfg.gpio_num = (gpio_num_t)PCN_PIN;
  rmt_cfg.clk_div = 80; // 1MHz (80MHz / 80) -> durations in us
  rmt_cfg.rmt_mode = RMT_MODE_RX;
  rmt_cfg.channel = RMT_CHANNEL;
  rmt_cfg.mem_block_num = 2;
  rmt_cfg.rx_config.filter_en = false; // disable hardware filter to capture all edges
  rmt_cfg.rx_config.filter_ticks_thresh = 1; // unused when filter_en=false
  rmt_cfg.rx_config.idle_threshold = 1000; // timeout

  rmt_config(&rmt_cfg);
  esp_err_t err = rmt_driver_install(rmt_cfg.channel, 1000, 0);
  Serial.printf("rmt_driver_install err=%d (chan=%d)\n", (int)err, (int)rmt_cfg.channel);
  rmt_get_ringbuf_handle(rmt_cfg.channel, &rb);
  Serial.printf("rmt_get_ringbuf_handle rb=%p\n", (void*)rb);
  if (err == ESP_OK && rb != NULL) {
    rmt_active = true;
    Serial.println("rmt: active");
    // initialize optional status pin (visible without serial)
    if (STATUS_PIN != -1) {
      pinMode(STATUS_PIN, OUTPUT);
      digitalWrite(STATUS_PIN, HIGH); // indicate active
    }
  } else {
    rmt_active = false;
    Serial.println("rmt: inactive");
    if (STATUS_PIN != -1) {
      pinMode(STATUS_PIN, OUTPUT);
      digitalWrite(STATUS_PIN, LOW);
    }
  }
  esp_err_t rx_ret = rmt_rx_start(rmt_cfg.channel, true);
  Serial.printf("rmt_rx_start ret=%d\n", (int)rx_ret);

  // ensure input pullup and initialize prev_level to actual pin state
  pinMode(PCN_PIN, INPUT_PULLUP);
  prev_level = digitalRead(PCN_PIN);

  // spawn a task to read ringbuffer and parse edges
  xTaskCreate([](void *arg) {
    (void)arg;
    for (;;) {
      size_t item_size = 0;
      rmt_item32_t *items = (rmt_item32_t *)xRingbufferReceive(rb, &item_size, portMAX_DELAY);
      if (items) {
        // debug: how many rmt items received
        size_t num = item_size / sizeof(rmt_item32_t);
        Serial.printf("rmt_rx: received %u items\n", (unsigned)num);

        // compute total duration
        unsigned long total_us = 0;
        for (size_t i = 0; i < num; ++i) {
          total_us += items[i].duration0 + items[i].duration1;
        }
        unsigned long now_us = esp_timer_get_time();
        unsigned long base_us = now_us - total_us;
        unsigned long cum = 0;
        unsigned long pulses_in_batch = 0;
        for (size_t i = 0; i < num; ++i) {
          rmt_item32_t it = items[i];
          // level0 transition
          if ((int)it.level0 != prev_level) {
            // transition occurred at base_us + cum
            int new_level = it.level0;
            unsigned long edge_time = base_us + cum;
            // count both rising and falling edges for debug
            {
              unsigned long now_p = edge_time;
              if (last_pulse_us != 0) {
                pulse_interval_us = now_p - last_pulse_us;
              }
              last_pulse_us = now_p;
              pulse_count++;
              pulses_in_batch++;
            }
            prev_level = new_level;
          }
          cum += it.duration0;

          // level1 transition
          if ((int)it.level1 != prev_level) {
            int new_level = it.level1;
            unsigned long edge_time = base_us + cum;
            // count both rising and falling edges for debug
            {
              unsigned long now_p = edge_time;
              if (last_pulse_us != 0) {
                pulse_interval_us = now_p - last_pulse_us;
              }
              last_pulse_us = now_p;
              pulse_count++;
              pulses_in_batch++;
            }
            prev_level = new_level;
          }
          cum += it.duration1;
        }
        // debug: pulses counted in this batch
        Serial.printf("rmt_rx: pulses_in_batch=%u  total_pulse_count=%lu\n", (unsigned)pulses_in_batch, pulse_count);
        rmt_batches++;
        // toggle status pin each batch so activity is externally visible (if enabled)
        if (STATUS_PIN != -1) {
          // blink by toggling level
          int v = digitalRead(STATUS_PIN);
          digitalWrite(STATUS_PIN, v ? LOW : HIGH);
        }

        // return the items
        vRingbufferReturnItem(rb, (void *)items);

        // notify processing task (if set)
        if (processing_task_handle != nullptr) {
          xTaskNotifyGive(processing_task_handle);
        }
      } else {
        // should not happen often; indicate timeout
        Serial.println("rmt_rx: no items (timeout)");
      }
    }
  }, "rmt_rx", 4096, NULL, tskIDLE_PRIORITY + 3, NULL);
}

unsigned long getAndClearCount() {
  taskENTER_CRITICAL(NULL);
  unsigned long c = pulse_count;
  pulse_count = 0;
  taskEXIT_CRITICAL(NULL);
  return c;
}

unsigned long peekCount() {
  taskENTER_CRITICAL(NULL);
  unsigned long v = pulse_count;
  taskEXIT_CRITICAL(NULL);
  return v;
}

unsigned long getLastIntervalUs() { return pulse_interval_us; }
unsigned long getLastPulseUs() { return last_pulse_us; }

bool isReady() { return rmt_active; }
unsigned long getRmtBatches() { return rmt_batches; }

} // namespace PhotoReflector

// global wrapper calling the namespaced setup
extern "C" void photoreflector_rmt_setup() {
  Serial.println("photoreflector_rmt_setup(): called");
  PhotoReflector::setup();
}

// add an entry print inside namespaced setup for clarity

#endif // USE_RMT
