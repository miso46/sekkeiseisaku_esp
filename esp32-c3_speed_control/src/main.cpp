#include "car_state.hpp"
#include "esp_now.hpp"
#include "motor.hpp"
#include "photoreflector.hpp"
#include "pid.hpp"
#include <HardwareSerial.h>
#include <chrono>
#include <limits>

constexpr float TARGET_SPEED_MM_MS = 0.4f;
constexpr float FF_BASE = 200.f; // feedforward base

CarState car_state;
espnow::Data car2 = {0.f, 0.f, std::numeric_limits<float>::infinity()};
PID pid(20.0f, 20.f, 0.6f);

std::chrono::system_clock::time_point t_prev;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("System Start (XIAO ESP32-C3)");

  Motor::setup();
  PhotoReflector::setup();
  espnow::setupEspNow();
  t_prev = std::chrono::system_clock::now();
}

void loop() {
  auto t_now = std::chrono::system_clock::now();
  const float dt_ms = static_cast<float>(
      std::chrono::duration_cast<std::chrono::microseconds>(t_now - t_prev)
          .count()) /
      1000.0f;
  t_prev = t_now;
  if (dt_ms <= 0.f)
    return;

  // PCNT のかわりに GPIO 割り込みで集計したパルス数を取得
  unsigned long delta_count = PhotoReflector::getAndClearCount();

  // CarState 更新
  car_state.update(static_cast<long>(delta_count), dt_ms);

  // 他車情報取得
  car2 = espnow::getCar2();

  // PID + FF でモータ出力
  const float speed = car_state.get_speed_mm_ms();
  const float error = TARGET_SPEED_MM_MS - speed;
  const float pid_out = pid.update(TARGET_SPEED_MM_MS, speed, dt_ms);
  Motor::write(FF_BASE + pid_out);

  // シリアル出力
  Serial.printf("[PID] target=%.3f  speed=%.3f  err=%.3f  pwm=%.0f\n",
                TARGET_SPEED_MM_MS, speed, error, FF_BASE + pid_out);
  Serial.printf("[POS] pos=%.1f mm  arrival=%.1f ms\n",
                car_state.get_current_position(), car_state.get_arrival_time_ms());

  if (car2.arrival_time_ms != std::numeric_limits<float>::infinity()) {
    Serial.printf("[CAR2] pos=%.1f mm  arrival=%.1f ms\n",
                  car2.current_position, car2.arrival_time_ms);
  }

  // ESP-NOW 送信
  espnow::Data sent = car_state.create_data();
  esp_now_send(targetAddress, (uint8_t *)&sent, sizeof(sent));

  delay(50);
}
