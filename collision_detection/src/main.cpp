#include "car_state.hpp" // .cpp → .hpp に変更
#include "esp_now.hpp"
#include <HardwareSerial.h>
#include <chrono>
#include <cstdlib>
#include <ctime>

// uint8_t targetAddress[] = {0xD4, 0xE9, 0xF4, 0xA7, 0xA0, 0x60}; // 文字正常
uint8_t targetAddress[] = {0xA4, 0xF0, 0x0F, 0x81, 0xE5, 0x34}; // 文字縦長
std::chrono::system_clock::time_point start, end;
int count;
CarState car_state;
espnow::Data car2 = {0.f, 0.f, std::numeric_limits<float>::infinity()};

void check_and_print_collision_from_data(const espnow::Data &dataA,
                                         const espnow::Data &dataB,
                                         float threshold_ms = 500.0f) {
  float tA = dataA.arrival_time_ms;
  float tB = dataB.arrival_time_ms;

  if (isinf(tA) || isinf(tB)) {
    Serial.println("衝突リスク: なし（どちらかが到達不能）");
    return;
  }

  if (tA < 0.0f || tB < 0.0f) {
    Serial.println("衝突リスク: なし（どちらかが通過済み）");
    return;
  }

  float diff = fabs(tA - tB);
  if (diff <= threshold_ms) {
    Serial.print("衝突リスク: あり（到着時間差 = ");
    Serial.print(diff);
    Serial.println(" ms）");
  } else {
    Serial.print("衝突リスク: なし（到着時間差 = ");
    Serial.print(diff);
    Serial.println(" ms）");
  }
}

void setup() {
  Serial.begin(115200);
  std::srand(static_cast<unsigned int>(std::time(nullptr)));
  count = 0;
  espnow::setupEspNow();
  start = std::chrono::system_clock::now();
  delay(100);
}

void loop() {
  end = std::chrono::system_clock::now();
  const double elapsed =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
          .count();
  Serial.print("経過時間: ");
  Serial.println(elapsed);

  count += std::rand() % 10;
  car_state.calculation_data(count, elapsed);
  start = end;

  espnow::Data sent_data = car_state.create_data();
  esp_now_send(targetAddress, (uint8_t *)&sent_data, sizeof(sent_data));
  check_and_print_collision_from_data(car_state.create_data(), car2, 300.0f);
  delay(200);
}
