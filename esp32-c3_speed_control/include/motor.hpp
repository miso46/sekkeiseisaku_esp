#pragma once

#include <Arduino.h>

// XIAO ESP32C3 pin mapping (aligned with esp32-c3/src/main.cpp)
// D2=GPIO4, D3=GPIO5, D4=GPIO6, D5=GPIO7
// RUN_SWITCH_PIN: D2 (GPIO4)
// ROTATE_PIN_1:   D3 (GPIO5)
// ROTATE_PIN_2:   D4 (GPIO6)
// PWM_PIN:        D5 (GPIO7)

namespace Motor {
static constexpr int RUN_SWITCH_PIN = 4; // D2
static constexpr int ROTATE_PIN_1 = 5;   // D3
static constexpr int ROTATE_PIN_2 = 6;   // D4
static constexpr int PWM_PIN = 7;        // D5
static constexpr int PWM_CHANNEL = 0;
static constexpr int PWM_FREQ = 20000;
static constexpr int PWM_RES = 8; // 0..255

inline void setup() {
  pinMode(RUN_SWITCH_PIN, OUTPUT);
  pinMode(ROTATE_PIN_1, OUTPUT);
  pinMode(ROTATE_PIN_2, OUTPUT);
  pinMode(PWM_PIN, OUTPUT);

  digitalWrite(RUN_SWITCH_PIN, HIGH); // RUN
  digitalWrite(ROTATE_PIN_1, HIGH);  // CW
  digitalWrite(ROTATE_PIN_2, LOW);

  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RES);
  ledcAttachPin(PWM_PIN, PWM_CHANNEL);
  ledcWrite(PWM_CHANNEL, 0);
}

inline void write(float duty) {
  if (duty < 0.f) duty = 0.f;
  if (duty > 255.f) duty = 255.f;
  ledcWrite(PWM_CHANNEL, static_cast<uint32_t>(duty));
}

inline void stop() { ledcWrite(PWM_CHANNEL, 0); }

} // namespace Motor
