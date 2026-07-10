#pragma once

// Force RMT branch in this debug project
//#ifndef USE_RMT
//#define USE_RMT
//#endif

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Common pin mapping
static constexpr int PCN_PIN = 21; // D6 (GPIO21) to avoid conflict with motor pins (D2-D5)

// Optional status output pin: set to a GPIO number to drive an LED or probe for RMT status.
// If left as -1, status output is disabled. Define STATUS_PIN in platformio.ini to override.
#ifndef STATUS_PIN
#define STATUS_PIN -1
#endif

#ifndef USE_RMT

namespace PhotoReflector {

// Using GPIO interrupt instead of PCNT for ESP32-C3 / XIAO

inline volatile unsigned long last_pulse_us = 0;
inline volatile unsigned long pulse_interval_us = 0;
inline volatile unsigned long pulse_count = 0;

// Task handle that will process pulses; set by pulse processing module
inline TaskHandle_t processing_task_handle = nullptr;

inline void setProcessingTaskHandle(TaskHandle_t h) { processing_task_handle = h; }

static constexpr unsigned long MIN_INTERVAL_US = 500;
inline void IRAM_ATTR onPulse() {
  unsigned long now = micros();
  if (last_pulse_us != 0 && (now - last_pulse_us) < MIN_INTERVAL_US) {
    return; // ノイズとして無視
  }
  if (last_pulse_us != 0) {
    pulse_interval_us = now - last_pulse_us;
  }
  last_pulse_us = now;
  pulse_count++;
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

// Read last pulse interval (microseconds) - return an atomic snapshot to avoid races with ISR
inline unsigned long getLastIntervalUs() {
  noInterrupts();
  unsigned long v = pulse_interval_us;
  interrupts();
  return v;
}

inline unsigned long getLastPulseUs() {
  noInterrupts();
  unsigned long v = last_pulse_us;
  interrupts();
  return v;
}

// Read current accumulated pulse_count without clearing (atomic)
inline unsigned long peekCount() {
  noInterrupts();
  unsigned long v = pulse_count;
  interrupts();
  return v;
}
inline bool isReady() { return true; }
inline unsigned long getRmtBatches() { return 0; }

} // namespace PhotoReflector

#else // USE_RMT

namespace PhotoReflector {

// RMT-based implementation declared; defined in src/photoreflector_rmt.cpp

void setup();
unsigned long getAndClearCount();
unsigned long peekCount();
unsigned long getLastIntervalUs();
unsigned long getLastPulseUs();
void setProcessingTaskHandle(TaskHandle_t h);

// Return true if underlying implementation is active (RMT driver installed or non-RMT available)
bool isReady();
unsigned long getRmtBatches();

} // namespace PhotoReflector

#endif // USE_RMT
