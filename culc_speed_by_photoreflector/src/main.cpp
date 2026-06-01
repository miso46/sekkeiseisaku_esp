#include "photoreflector.hpp"
#include <Arduino.h>
#include <chrono>
#include <driver/pcnt.h>

const int PCN_PIN = 4;
pcnt_unit_t unit = PCNT_UNIT_0;

int before_count;
const float pulse_spacing_mm = 10.f;
std::chrono::system_clock::time_point start, end;

void setup() {
  Serial.begin(115200);

  PhotoReflector::setupPcnt();

  before_count = 0;
  start = std::chrono::system_clock::now();
}

void loop() {
  end = std::chrono::system_clock::now();

  const double elapsed =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
          .count();
  int16_t current_count;
  pcnt_get_counter_value(unit, &current_count);

  Serial.print("経過時間: ");
  Serial.println(elapsed);
  Serial.printf("PCNT count: %d\n", current_count);

  const float distance = (current_count - before_count) * pulse_spacing_mm;
  const float speed = distance / static_cast<float>(elapsed);

  Serial.print("速さ: ");
  Serial.print(speed);
  Serial.println("[mm/ms]");

  before_count = current_count;
  start = end;
  delay(50);
}
