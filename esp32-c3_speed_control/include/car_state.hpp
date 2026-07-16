#pragma once

#include "photoreflector.hpp"
#include "esp_now.hpp"
#include <limits>

class CarState {
private:
  long accumulated_count = 0;
  float current_position = 0.f;
  float speed_mm_ms = 0.f;
  float arrival_time_ms = 0.f;
  const float distance_intersection = 2000.f;
  const float pulse_spacing_mm = 10.f;

public:
  float get_current_position() const { return current_position; }
  float get_speed_mm_ms() const { return speed_mm_ms; }
  float get_arrival_time_ms() const { return arrival_time_ms; }

  void update(long delta_count, float dt_ms) {
    if (dt_ms <= 0.f)
      return;

    accumulated_count += delta_count;
    current_position = accumulated_count * pulse_spacing_mm;

    unsigned long interval_us = PhotoReflector::getLastIntervalUs();
    unsigned long last_us = PhotoReflector::getLastPulseUs();

    if (interval_us == 0 || micros() - last_us > 100000UL) {
      speed_mm_ms = 0.f;
    } else {
      speed_mm_ms = pulse_spacing_mm / (interval_us / 1000.f);
    }

    if (speed_mm_ms <= 0.f) {
      arrival_time_ms = std::numeric_limits<float>::infinity();
    } else {
      arrival_time_ms = (distance_intersection - current_position) / speed_mm_ms;
    }
  }

  espnow::Data create_data() const { return {current_position, speed_mm_ms, arrival_time_ms}; }
};
