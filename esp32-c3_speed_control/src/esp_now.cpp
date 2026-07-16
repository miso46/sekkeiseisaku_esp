#include "esp_now.hpp"
#include <HardwareSerial.h>

// Target peer address centralized here
uint8_t targetAddress[] = {0xD4, 0xE9, 0xF4, 0xA7, 0xA0, 0x60};

esp_now_peer_info_t peerInfo;

namespace espnow {

static Data car2_buf = {0.f, 0.f, std::numeric_limits<float>::infinity()};
static SemaphoreHandle_t car2_mutex = xSemaphoreCreateMutex();

// Sending queue and task
static QueueHandle_t send_queue = nullptr;
static TaskHandle_t send_task_handle = nullptr;

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

static void sendTask(void *arg) {
  (void)arg;
  Data d;
  for (;;) {
    if (xQueueReceive(send_queue, &d, portMAX_DELAY) == pdTRUE) {
      esp_err_t res = esp_now_send(targetAddress, (uint8_t *)&d, sizeof(d));
      if (res != ESP_OK) {
        Serial.println("esp_now_send failed (queued)");
      }
      // small delay to allow other tasks if necessary
      vTaskDelay(pdMS_TO_TICKS(1));
    }
  }
}

void enqueueSend(const Data &d) {
  if (send_queue == nullptr)
    return;
  xQueueSend(send_queue, &d, 0); // non-blocking
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
    // continue; we may still want to start queue
  }

  // create send queue and task
  if (send_queue == nullptr) {
    send_queue = xQueueCreate(8, sizeof(Data));
    xTaskCreate(sendTask, "espnow_send", 3072, nullptr, tskIDLE_PRIORITY + 1, &send_task_handle);
  }
}

} // namespace espnow
