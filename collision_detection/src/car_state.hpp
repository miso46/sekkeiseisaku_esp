#pragma once
#include "esp_now.hpp"
#include <limits>

class CarState {
private:
  float current_position = 0.f;
  float speed_ms = 0.f;
  float arrival_time_ms = 0.f;
  const float distance_intersection = 2000.f;
  const float pulse_spacing_mm = 10.f;

public:
  float get_current_position() const { return current_position; }
  float get_speed_ms() const { return speed_ms; }
  float get_arrival_time_ms() const { return arrival_time_ms; }

  void calculation_data(int count, double time_ms) {
    if (time_ms <= 0.0)
      return;

    const float new_position = count * this->pulse_spacing_mm;
    this->speed_ms =
        (new_position - this->current_position) / static_cast<float>(time_ms);
    this->current_position = new_position;

    if (this->speed_ms == 0.0f) {
      this->arrival_time_ms = std::numeric_limits<float>::infinity();
    } else {
      float remaining_distance =
          this->distance_intersection - this->current_position;
      this->arrival_time_ms = remaining_distance / this->speed_ms;
    }
  }

  espnow::Data create_data() {
    return {this->current_position, this->speed_ms, this->arrival_time_ms};
  }
};
