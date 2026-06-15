#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

uint8_t targetAddress[] = {0xA4, 0xF0, 0x0F,
                           0x81, 0xE5, 0x34}; // モータ側(縦長)

esp_now_peer_info_t peerInfo;

struct Data {
  int16_t duty; // -255~255、マイナスで逆転
};

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("送信: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "成功" : "失敗");
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init error");
    return;
  }

  memcpy(peerInfo.peer_addr, targetAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  esp_now_register_send_cb(onDataSent);

  Serial.println("duty値(-255~255)を入力してEnter:");
  Serial.println("0でブレーキ、マイナスで逆転");
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.length() == 0)
      return;

    int16_t val = (int16_t)constrain(input.toInt(), -255, 255);

    Data sent_data;
    sent_data.duty = val;

    esp_now_send(targetAddress, (uint8_t *)&sent_data, sizeof(sent_data));
    Serial.printf("送信: duty = %d\n", val);
  }
}
