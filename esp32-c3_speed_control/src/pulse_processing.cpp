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

    // Update car state
    if (g_state) {
      g_state->update(static_cast<long>(pulses), static_cast<float>(dt_ms));
      // Enqueue ESP-NOW send of latest state (non-blocking)
      espnow::enqueueSend(g_state->create_data());
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
