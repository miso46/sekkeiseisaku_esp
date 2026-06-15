#include <Arduino.h>

const int RUN_SWITCH_PIN = 22; // HIGHのときドライバRUN
// 10_CW 01_CCW 00_Brake
const int ROTATE_PIN_1 = 18;
const int ROTATE_PIN_2 = 19;
const int PWM_PIN = 12; // 速度制御(PWM)

// PWM関連
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
  pinMode(PWM_PIN, OUTPUT);

  // PinをHIGHにする（CW方向に設定）
  digitalWrite(RUN_SWITCH_PIN, HIGH);
  digitalWrite(ROTATE_PIN_1, HIGH);
  digitalWrite(ROTATE_PIN_2, LOW);

  // PWMのピンを設定して割り当て
  ledcSetup(ledcChannel, freq, bit);
  ledcAttachPin(PWM_PIN, ledcChannel);
}

void loop() {
  Serial.print("duty: ");
  Serial.println(duty);

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
