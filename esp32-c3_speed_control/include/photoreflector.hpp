#pragma once

#include <Arduino.h>

namespace PhotoReflector {

// Using GPIO interrupt instead of PCNT for ESP32-C3 / XIAO
static constexpr int PCN_PIN = 4; // D2 on board mapping

volatile unsigned long last_pulse_us = 0;
volatile unsigned long pulse_interval_us = 0;
volatile unsigned long pulse_count = 0;

void IRAM_ATTR onPulse() {
  unsigned long now = micros();
  if (last_pulse_us != 0) {
    pulse_interval_us = now - last_pulse_us;
  }
  last_pulse_us = now;
  pulse_count++;
}

inline void setupInterrupt() {
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
