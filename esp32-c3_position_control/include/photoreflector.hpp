#pragma once

#include <Arduino.h>

namespace PhotoReflector {

static constexpr int PCN_PIN = 21;

// 黒帯5mm、CHANGE(両エッジ)なので1エッジ=5mm。V_MAX=1.5m/sで理論最短間隔約3.3ms。
static constexpr unsigned long MIN_INTERVAL_US = 1500;

inline volatile unsigned long last_pulse_us = 0;
inline volatile unsigned long pulse_interval_us = 0;
inline volatile unsigned long pulse_count = 0;

inline void IRAM_ATTR onPulse() {
  unsigned long now = micros();
  if (last_pulse_us != 0 && (now - last_pulse_us) < MIN_INTERVAL_US) {
    return;
  }
  if (last_pulse_us != 0) {
    pulse_interval_us = now - last_pulse_us;
  }
  last_pulse_us = now;
  pulse_count++;
}

inline void setup() {
  pinMode(PCN_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PCN_PIN), onPulse, CHANGE);
}

inline unsigned long getAndClearCount() {
  noInterrupts();
  unsigned long c = pulse_count;
  pulse_count = 0;
  interrupts();
  return c;
}

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

} // namespace PhotoReflector
