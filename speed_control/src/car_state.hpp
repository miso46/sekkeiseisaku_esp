#pragma once
#include "esp_now.hpp"
#include <limits>

class CarState {
private:
  long accumulated_count = 0; // オーバーフロー対策済み累積カウント
  float current_position = 0.f;
  float speed_mm_ms = 0.f;
  float arrival_time_ms = 0.f;
  const float distance_intersection = 2000.f;
  const float pulse_spacing_mm = 10.f;

public:
  float get_current_position() const { return current_position; }
  float get_speed_mm_ms() const { return speed_mm_ms; }
  float get_arrival_time_ms() const { return arrival_time_ms; }

  // delta_count: 今回ループの差分カウント（PCNTクリア運用前提）
  void update(int16_t delta_count, float dt_ms) {
    if (dt_ms <= 0.f)
      return;

    accumulated_count += delta_count;
    const float new_pos = accumulated_count * pulse_spacing_mm;
    this->speed_mm_ms = (new_pos - current_position) / dt_ms;
    this->current_position = new_pos;

    if (speed_mm_ms <= 0.f) {
      arrival_time_ms = std::numeric_limits<float>::infinity();
    } else {
      arrival_time_ms =
          (distance_intersection - current_position) / speed_mm_ms;
    }
  }

  espnow::Data create_data() const {
    return {current_position, speed_mm_ms, arrival_time_ms};
  }
};
