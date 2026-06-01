#pragma once
#include <WiFi.h>
#include <esp_now.h>

extern uint8_t targetAddress[]; // main.cpp で定義
extern esp_now_peer_info_t peerInfo;

namespace espnow {

struct Data {
  float current_position;
  float speed;
  float arrival_time_ms;
};

// コールバック関数のプロトタイプ（esp_now.cpp で定義）
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void onReceive(const uint8_t *mac_addr, const uint8_t *data, int len);

void setupEspNow();

} // namespace espnow

extern espnow::Data car2;
