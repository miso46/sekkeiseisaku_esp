#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace PhotoReflector {

// Using GPIO interrupt instead of PCNT for ESP32-C3 / XIAO
static constexpr int PCN_PIN = 4; // D2 on board mapping

inline volatile unsigned long last_pulse_us = 0;
inline volatile unsigned long pulse_interval_us = 0;
inline volatile unsigned long pulse_count = 0;

// Task handle that will process pulses; set by pulse processing module
inline TaskHandle_t processing_task_handle = nullptr;

inline void setProcessingTaskHandle(TaskHandle_t h) { processing_task_handle = h; }

inline void IRAM_ATTR onPulse() {
  // Minimal work in ISR: record time and increment count, notify processing task
  unsigned long now = micros();
  if (last_pulse_us != 0) {
    pulse_interval_us = now - last_pulse_us;
  }
  last_pulse_us = now;
  pulse_count++;

  // Notify processing task (if set) that new pulse(s) arrived
  if (processing_task_handle != nullptr) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(processing_task_handle, &xHigherPriorityTaskWoken);
    // If a higher priority task was woken, yield
    if (xHigherPriorityTaskWoken == pdTRUE) {
      portYIELD_FROM_ISR();
    }
  }
}

inline void setup() {
  pinMode(PCN_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PCN_PIN), onPulse, RISING);
}

// Atomically get and clear the accumulated pulse count
inline unsigned long getAndClearCount() {
  noInterrupts();
  unsigned long c = pulse_count;
  pulse_count = 0;
  interrupts();
  return c;
}

// Read last pulse interval (microseconds) - non-atomic but sufficient for diagnostics
inline unsigned long getLastIntervalUs() { return pulse_interval_us; }
inline unsigned long getLastPulseUs() { return last_pulse_us; }

} // namespace PhotoReflector
