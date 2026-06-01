#include "car_state.hpp"
#include "esp_now.hpp"
#include "motor.hpp"
#include "photoreflector.hpp"
#include "pid.hpp"
#include <HardwareSerial.h>
#include <chrono>
#include <limits>

constexpr float TARGET_SPEED_MM_MS = 0.5f;

uint8_t targetAddress[] = {0xA4, 0xF0, 0x0F, 0x81, 0xE5, 0x34};
CarState car_state;
espnow::Data car2 = {0.f, 0.f, std::numeric_limits<float>::infinity()};
PID pid(2.0f, 0.f, 0.1f);

std::chrono::system_clock::time_point t_prev;

void setup() {
  Serial.begin(115200);
  Motor::setup();
  PhotoReflector::setupPcnt();
  espnow::setupEspNow();
  t_prev = std::chrono::system_clock::now();
}

void loop() {
  // 経過時間
  auto t_now = std::chrono::system_clock::now();
  const float dt_ms = static_cast<float>(
      std::chrono::duration_cast<std::chrono::milliseconds>(t_now - t_prev)
          .count());
  t_prev = t_now;
  if (dt_ms <= 0.f)
    return;

  // PCNT読み取り&クリア
  int16_t delta_count;
  pcnt_get_counter_value(PhotoReflector::UNIT, &delta_count);
  pcnt_counter_clear(PhotoReflector::UNIT);

  // CarState更新
  car_state.update(delta_count, dt_ms);

  // PIDによりモータ出力計算、書き込み
  const float speed = car_state.get_speed_mm_ms();
  const float error = TARGET_SPEED_MM_MS - speed;
  const float output = pid.update(TARGET_SPEED_MM_MS, speed, dt_ms);
  Motor::write(output);

  // Serial出力
  Serial.printf("[PID] target=%.3f  speed=%.3f  err=%.3f  pwm=%.0f\n",
                TARGET_SPEED_MM_MS, speed, error, output);
  Serial.printf("[POS] pos=%.1f mm  arrival=%.1f ms\n",
                car_state.get_current_position(),
                car_state.get_arrival_time_ms());

  // ESP-NOW送信
  espnow::Data sent = car_state.create_data();
  esp_now_send(targetAddress, (uint8_t *)&sent, sizeof(sent));

  delay(50);
}
