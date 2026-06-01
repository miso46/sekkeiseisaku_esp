#include "esp_now.hpp"
#include <HardwareSerial.h>

esp_now_peer_info_t peerInfo;

namespace espnow {

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("送信: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "成功" : "失敗");
}

void onReceive(const uint8_t *mac_addr, const uint8_t *data, int len) {
  Data received;
  memcpy(&received, data, sizeof(received));
  car2 = received;
  Serial.print("受信 position: ");
  Serial.println(received.current_position);
  Serial.print("受信 speed: ");
  Serial.println(received.speed);
  Serial.print("受信 arrival: ");
  Serial.println(received.arrival_time_ms);
}

void setupEspNow() {
  WiFi.mode(WIFI_STA);
  Serial.print("Mac address: ");
  Serial.println(WiFi.macAddress());

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
  esp_now_register_recv_cb(onReceive);
}

} // namespace espnow
