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

void setProcessingTaskHandle(TaskHandle_t h) { processing_task_handle = h; }

void setup() {
  rmt_config_t rmt_cfg = {};
  rmt_cfg.gpio_num = (gpio_num_t)PCN_PIN;
  rmt_cfg.clk_div = 80; // 1MHz (80MHz / 80) -> durations in us
  rmt_cfg.rmt_mode = RMT_MODE_RX;
  rmt_cfg.channel = RMT_CHANNEL;
  rmt_cfg.mem_block_num = 2;
  rmt_cfg.rx_config.filter_en = false;
  rmt_cfg.rx_config.idle_threshold = 10000; // timeout

  rmt_config(&rmt_cfg);
  rmt_driver_install(rmt_cfg.channel, 1000, 0);
  rmt_get_ringbuf_handle(rmt_cfg.channel, &rb);
  rmt_rx_start(rmt_cfg.channel, true);

  // spawn a task to read ringbuffer and parse edges
  xTaskCreate([](void *arg) {
    (void)arg;
    for (;;) {
      size_t item_size = 0;
      rmt_item32_t *items = (rmt_item32_t *)xRingbufferReceive(rb, &item_size, portMAX_DELAY);
      if (items) {
        // compute total duration
        size_t num = item_size / sizeof(rmt_item32_t);
        unsigned long total_us = 0;
        for (size_t i = 0; i < num; ++i) {
          total_us += items[i].duration0 + items[i].duration1;
        }
        unsigned long now_us = esp_timer_get_time();
        unsigned long base_us = now_us - total_us;
        unsigned long cum = 0;
        for (size_t i = 0; i < num; ++i) {
          rmt_item32_t it = items[i];
          // level0 transition
          if ((int)it.level0 != prev_level) {
            // transition occurred at base_us + cum
            int new_level = it.level0;
            unsigned long edge_time = base_us + cum;
            if (new_level == 1) {
              // rising edge
              unsigned long now_p = edge_time;
              if (last_pulse_us != 0) {
                pulse_interval_us = now_p - last_pulse_us;
              }
              last_pulse_us = now_p;
              pulse_count++;
            }
            prev_level = new_level;
          }
          cum += it.duration0;

          // level1 transition
          if ((int)it.level1 != prev_level) {
            int new_level = it.level1;
            unsigned long edge_time = base_us + cum;
            if (new_level == 1) {
              unsigned long now_p = edge_time;
              if (last_pulse_us != 0) {
                pulse_interval_us = now_p - last_pulse_us;
              }
              last_pulse_us = now_p;
              pulse_count++;
            }
            prev_level = new_level;
          }
          cum += it.duration1;
        }
        // return the items
        vRingbufferReturnItem(rb, (void *)items);

        // notify processing task (if set)
        if (processing_task_handle != nullptr) {
          xTaskNotifyGive(processing_task_handle);
        }
      }
    }
  }, "rmt_rx", 4096, NULL, tskIDLE_PRIORITY + 3, NULL);
}

unsigned long getAndClearCount() {
  taskENTER_CRITICAL();
  unsigned long c = pulse_count;
  pulse_count = 0;
  taskEXIT_CRITICAL();
  return c;
}

unsigned long getLastIntervalUs() { return pulse_interval_us; }
unsigned long getLastPulseUs() { return last_pulse_us; }

} // namespace PhotoReflector

#endif // USE_RMT
