#include <Arduino.h>

const int RUN_SWITCH_PIN = 22; // HIGHのときドライバRUN
// 10_CW 01_CCW 00_Brake
const int ROTATE_PIN_1 = 18;
const int ROTATE_PIN_2 = 19;
const int PWM_PIN = 12; // 速度制御(PWM)

// PWM関連
const int ledcChannel = 0;
const int freq = 20000;
const int bit = 8;

int duty = 255;
bool flag = false;
int count = 0;

void setup() {
  Serial.begin(115200); // シリアル通信開始

  // PinをOUTPUTに設定
  pinMode(RUN_SWITCH_PIN, OUTPUT);
  pinMode(ROTATE_PIN_1, OUTPUT);
  pinMode(ROTATE_PIN_1, OUTPUT);
  pinMode(ROTATE_PIN_2, OUTPUT);
  pinMode(PWM_PIN, OUTPUT);

  // PinをHIGHにする
  digitalWrite(RUN_SWITCH_PIN, HIGH);
  digitalWrite(ROTATE_PIN_1, HIGH);
  digitalWrite(ROTATE_PIN_2, LOW);

  // PWMのピンを設定して割り当て
  ledcSetup(ledcChannel, freq, bit);
  ledcAttachPin(PWM_PIN, ledcChannel);
}

void loop() {
  if (count%2 == 1) {
    digitalWrite(ROTATE_PIN_1, LOW);
    digitalWrite(ROTATE_PIN_2, HIGH);
  }else{
    digitalWrite(ROTATE_PIN_1, HIGH);
    digitalWrite(ROTATE_PIN_2, LOW);
  }

  ledcWrite(ledcChannel, duty);
  Serial.print("duty: ");
  Serial.println(duty);
  delay(100);
  duty--;
  if(duty < 0){
	  duty = 255;
	  count++;
  }


}

