#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

const int RUN_SWITCH_PIN = 22;
const int ROTATE_PIN_1 = 18;
const int ROTATE_PIN_2 = 19;
const int PWM_PIN = 12;
const int ledcChannel = 0;
const int freq = 20000;
const int bit = 8;

struct Data {
  int16_t duty; // -255~255、マイナスで逆転
};

void onReceive(const uint8_t *mac_addr, const uint8_t *data, int len) {
  if (len != sizeof(Data))
    return;
  Data received;
  memcpy(&received, data, sizeof(received));

  int16_t duty = received.duty;

  if (duty > 0) {
    // CW（正転）
    digitalWrite(ROTATE_PIN_1, HIGH);
    digitalWrite(ROTATE_PIN_2, LOW);
    ledcWrite(ledcChannel, (uint8_t)duty);
  } else if (duty < 0) {
    // CCW（逆転）
    digitalWrite(ROTATE_PIN_1, LOW);
    digitalWrite(ROTATE_PIN_2, HIGH);
    ledcWrite(ledcChannel, (uint8_t)(-duty));
  } else {
    // ブレーキ
    digitalWrite(ROTATE_PIN_1, LOW);
    digitalWrite(ROTATE_PIN_2, LOW);
    ledcWrite(ledcChannel, 0);
  }

  Serial.printf("受信: duty = %d\n", duty);
}

void setup() {
  Serial.begin(115200);

  pinMode(RUN_SWITCH_PIN, OUTPUT);
  pinMode(ROTATE_PIN_1, OUTPUT);
  pinMode(ROTATE_PIN_2, OUTPUT);

  digitalWrite(RUN_SWITCH_PIN, HIGH);
  digitalWrite(ROTATE_PIN_1, HIGH);
  digitalWrite(ROTATE_PIN_2, LOW);

  ledcSetup(ledcChannel, freq, bit);
  ledcAttachPin(PWM_PIN, ledcChannel);
  ledcWrite(ledcChannel, 0); // 起動時は停止

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init error");
    return;
  }
  esp_now_register_recv_cb(onReceive);
  Serial.println("受信待機中...");
}

void loop() { delay(1000); }
