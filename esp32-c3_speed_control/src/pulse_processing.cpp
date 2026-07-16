#include "pulse_processing.hpp"
#include "photoreflector.hpp"
#include "car_state.hpp"
#include "esp_now.hpp"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static CarState *g_state = nullptr;
static TaskHandle_t task_handle = nullptr;

static void pulseTask(void *arg) {
  (void)arg;
  unsigned long prev_us = micros();
  for (;;) {
    // Wait for notification from ISR or timeout to process periodically
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100)); // clear count on wake

    unsigned long now_us = micros();
    unsigned long dt_ms = (now_us - prev_us) / 1000UL;
    if (dt_ms == 0) dt_ms = 1;
    prev_us = now_us;

    unsigned long pulses = PhotoReflector::getAndClearCount();

    // Diagnostic accumulation: count pulses and print once per second
    static unsigned long pulses_accum = 0;
    static unsigned long last_diag_ms = 0;
    pulses_accum += pulses;

    // Update car state
    if (g_state) {
      g_state->update(static_cast<long>(pulses), static_cast<float>(dt_ms));
      // Enqueue ESP-NOW send of latest state (non-blocking)
      espnow::enqueueSend(g_state->create_data());
    }

    unsigned long now_ms = millis();
    if (now_ms - last_diag_ms >= 1000) {
      last_diag_ms = now_ms;
      unsigned long last_interval = PhotoReflector::getLastIntervalUs();
      Serial.printf("pulse_proc: pulses_last_sec=%lu  last_interval_us=%lu  accumulated=%ld\n",
                    pulses_accum, last_interval, (g_state ? (long)(g_state->get_current_position() / 10.0f) : 0));
      pulses_accum = 0;
    }
  }
}

void startPulseProcessing(CarState *state) {
  g_state = state;
  // create task with high priority
  xTaskCreatePinnedToCore(pulseTask, "pulse_proc", 2048, nullptr, configMAX_PRIORITIES - 2, &task_handle, tskNO_AFFINITY);
  // give the handle to photoreflector so ISR can notify
  PhotoReflector::setProcessingTaskHandle(task_handle);
}
