#include <WiFi.h>
#include <esp_now.h>

uint8_t targetAddress[] = {0xD4, 0xE9, 0xF4, 0xA7, 0xA0, 0x60};
esp_now_peer_info_t peerInfo;

struct Data {
  int num;
  char str[32];
};

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("送信: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "成功" : "失敗");
}

void onReceive(const uint8_t *mac_addr, const uint8_t *data, int len) {
  Data received;
  memcpy(&received, data, sizeof(received));
  Serial.println(received.num);
  Serial.println(received.str);
}

void setup() {
  Serial.begin(115200);
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

int num = 0;

void loop() {
  Data sent_data;
  sent_data.num = num;
  strncpy(sent_data.str, "hello esp-now", sizeof(sent_data.str));

  esp_now_send(targetAddress, (uint8_t *)&sent_data, sizeof(sent_data));
  num++;
  delay(200);
}
