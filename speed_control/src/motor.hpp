#pragma once
#include <Arduino.h>

namespace Motor {

constexpr int RUN_SWITCH_PIN = 22;
constexpr int ROTATE_PIN_1 = 18;
constexpr int ROTATE_PIN_2 = 19;
constexpr int PWM_PIN = 12;
constexpr int PWM_CHANNEL = 0;
constexpr int PWM_FREQ = 20000;
constexpr int PWM_RES = 8;

void setup() {
  pinMode(RUN_SWITCH_PIN, OUTPUT);
  pinMode(ROTATE_PIN_1, OUTPUT);
  pinMode(ROTATE_PIN_2, OUTPUT);
  digitalWrite(RUN_SWITCH_PIN, HIGH);
  digitalWrite(ROTATE_PIN_1, HIGH); // CW固定
  digitalWrite(ROTATE_PIN_2, LOW);

  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RES);
  ledcAttachPin(PWM_PIN, PWM_CHANNEL);
  ledcWrite(PWM_CHANNEL, 0);
}

void write(float duty) {
  if (duty < 0.f)
    duty = 0.f;
  if (duty > 255.f)
    duty = 255.f;
  ledcWrite(PWM_CHANNEL, static_cast<uint32_t>(duty));
}

void stop() { ledcWrite(PWM_CHANNEL, 0); }

} // namespace Motor
