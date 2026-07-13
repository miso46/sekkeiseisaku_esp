#pragma once

#include <Arduino.h>

// ===== モータ制御(DRV8835 PHASE/ENABLE方式) =====
// memo.txtの配線に基づく (7/8修正後の仕様)
// D2 - GPIO4 - VCC/MODE   -> 常時HIGH (PHASE/ENABLEモード固定)
// D3 - GPIO5 - IN2/ENABLE -> PWMで速度制御、0でコースト(空転)
// D5 - GPIO7 - IN1/PHASE  -> 0で正転、1で逆転

namespace Motor {

static constexpr int RUN_SWITCH_PIN = 4; // D2 (VCC/MODE)
static constexpr int PHASE_PIN      = 7; // D5 (回転方向)
static constexpr int ENABLE_PIN     = 5; // D3 (PWM速度)

static constexpr int PWM_CHANNEL   = 0;
static constexpr int PWM_FREQ      = 20000;
static constexpr int PWM_RES_BITS  = 8; // 0-255

// このduty以下だと静止摩擦で車輪が回らない、という下限値。
// 実測して調整すること(小さすぎるとduty>0でも動かない領域が出る)。
static constexpr int PWM_MIN = 20;
static constexpr int PWM_MAX = 230;

inline void setup() {
  pinMode(RUN_SWITCH_PIN, OUTPUT);
  pinMode(PHASE_PIN, OUTPUT);

  digitalWrite(RUN_SWITCH_PIN, HIGH); // ドライバ有効化
  digitalWrite(PHASE_PIN, LOW);       // 正転方向に固定

  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RES_BITS);
  ledcAttachPin(ENABLE_PIN, PWM_CHANNEL);

  ledcWrite(PWM_CHANNEL, 0); // 起動時は停止(コースト)
}

// duty: 0 = コースト(空転,ブレーキではない), それ以外は[PWM_MIN, PWM_MAX]にクランプして出力
inline void driveForward(int duty) {
  if (duty <= 0) {
    ledcWrite(PWM_CHANNEL, 0);
    return;
  }
  duty = constrain(duty, PWM_MIN, PWM_MAX);
  ledcWrite(PWM_CHANNEL, duty);
}

inline void coast() {
  ledcWrite(PWM_CHANNEL, 0);
}

} // namespace Motor
