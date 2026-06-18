#include <Arduino.h>

// === XIAO ESP32C3 ピン割り当て ===
// D0=GPIO2, D1=GPIO3 はストラッピングピンのため使用を避ける
// D2~D7 (GPIO4,5,6,7,21,20) を使用
const int RUN_SWITCH_PIN = 4; // D2: HIGHのときドライバRUN
// 10_CW 01_CCW 00_Brake
const int ROTATE_PIN_1 = 5; // D3
const int ROTATE_PIN_2 = 6; // D4
const int PWM_PIN = 7;      // D5: 速度制御(PWM)

// PWM関連
// (このプロジェクトのframework-arduinoespressif32は2.0.17系のため旧API使用)
const int ledcChannel = 0;
const int freq = 20000;
const int bit = 8; // 8bit = 0~255
int duty = 0;
bool flag = false;
int count = 0;

void setup() {
  Serial.begin(115200); // シリアル通信開始

  // PinをOUTPUTに設定
  pinMode(RUN_SWITCH_PIN, OUTPUT);
  pinMode(ROTATE_PIN_1, OUTPUT);
  pinMode(ROTATE_PIN_2, OUTPUT);

  // PinをHIGHにする（CW方向に設定）
  digitalWrite(RUN_SWITCH_PIN, HIGH);
  digitalWrite(ROTATE_PIN_1, HIGH);
  digitalWrite(ROTATE_PIN_2, LOW);

  // PWMのピンを設定して割り当て (core 2.0.17系の旧API)
  ledcSetup(ledcChannel, freq, bit);
  ledcAttachPin(PWM_PIN, ledcChannel);
}

void loop() {
  Serial.print("duty: ");
  Serial.println(duty);

  // core 2.0.17系: ledcWriteはチャンネル番号を渡す
  ledcWrite(ledcChannel, duty);

  delay(100);
  duty += 2;
  count++;
  if (duty > 255) {
    duty = 255;
  }
  if (count > 1000) {
    duty = 0;
  }
}
