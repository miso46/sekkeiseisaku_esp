#pragma once

#include <Arduino.h>

// ===== フォトリフレクタ(エンコーダ)関連 =====
// GPIO割り込みベースのパルスカウント + パルス間隔計測

namespace PhotoReflector {

static constexpr int PCN_PIN = 21; // D6 (GPIO21)

// デバウンス閾値(us)。これより短い間隔のパルスはチャタリングとして無視する。
// 大きすぎると本物の高速パルスを間引いてしまうので、実測して調整すること。
static constexpr unsigned long MIN_INTERVAL_US = 1500; // 5ms

inline volatile unsigned long last_pulse_us = 0;
inline volatile unsigned long pulse_interval_us = 0;
inline volatile unsigned long pulse_count = 0;

inline void IRAM_ATTR onPulse() {
  unsigned long now = micros();
  if (last_pulse_us != 0 && (now - last_pulse_us) < MIN_INTERVAL_US) {
    return; // チャタリングとして無視
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

// 累積カウントを取得してクリア(距離の積算用)
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
