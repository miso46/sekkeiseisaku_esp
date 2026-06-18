#pragma once
#include <WiFi.h>
#include <esp_now.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

extern uint8_t targetAddress[];
extern esp_now_peer_info_t peerInfo;

namespace espnow {

struct Data {
  float current_position;
  float speed_mm_ms;
  float arrival_time_ms;
};

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void onReceive(const uint8_t *mac_addr, const uint8_t *data, int len);
void setupEspNow();

Data getCar2();

// Enqueue a Data object to be sent via ESP-NOW from the send task (non-blocking)
void enqueueSend(const Data &d);

} // namespace espnow
