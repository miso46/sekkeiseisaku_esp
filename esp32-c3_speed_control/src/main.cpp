#include "car_state.hpp"
#include "esp_now.hpp"
#include "motor.hpp"
#include "photoreflector.hpp"
#include "pid.hpp"
#include "pulse_processing.hpp"
#include "stress_test.hpp"
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

  // start high-priority pulse processing task
  startPulseProcessing(&car_state);

#ifdef STRESS_TEST
  // start internal pulse generator + espnow flood for stress testing
  startStressTest();
#endif

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

  // Pulse counting and CarState update are handled in the high-priority pulse processing task.
  // Do not call PhotoReflector::getAndClearCount() or car_state.update() here to avoid
  // stealing counts from the pulse processing task.

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

  // ESP-NOW sends are enqueued by the pulse processing task. No direct esp_now_send() here.
  // If needed, main can still request a send via espnow::enqueueSend(car_state.create_data());

  delay(50);
}
