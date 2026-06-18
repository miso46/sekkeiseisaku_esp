#include "esp_now.hpp"
#include <HardwareSerial.h>

// Target peer address centralized here
uint8_t targetAddress[] = {0xD4, 0xE9, 0xF4, 0xA7, 0xA0, 0x60};

esp_now_peer_info_t peerInfo;

namespace espnow {

static Data car2_buf = {0.f, 0.f, std::numeric_limits<float>::infinity()};
static SemaphoreHandle_t car2_mutex = xSemaphoreCreateMutex();

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("送信: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "成功" : "失敗");
}

void onReceive(const uint8_t *mac_addr, const uint8_t *data, int len) {
  if (len != sizeof(Data))
    return;

  Data received;
  memcpy(&received, data, sizeof(received));

  if (xSemaphoreTake(car2_mutex, 0) == pdTRUE) {
    car2_buf = received;
    xSemaphoreGive(car2_mutex);
  }

  Serial.printf("受信 position: %.1f  speed: %.3f  arrival: %.1f\n",
                received.current_position, received.speed_mm_ms,
                received.arrival_time_ms);
}

Data getCar2() {
  Data snapshot;
  if (xSemaphoreTake(car2_mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
    snapshot = car2_buf;
    xSemaphoreGive(car2_mutex);
  }
  return snapshot;
}

void setupEspNow() {
  WiFi.mode(WIFI_STA);
  Serial.print("Mac address: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init error");
    return;
  }

  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onReceive);

  memcpy(peerInfo.peer_addr, targetAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
}

} // namespace espnow
