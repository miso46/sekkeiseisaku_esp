#pragma once
#include <Arduino.h>
#include <driver/pcnt.h>

namespace PhotoReflector {

constexpr int PCN_PIN = 4;
constexpr pcnt_unit_t UNIT = PCNT_UNIT_0;

void setupPcnt() {
  pinMode(PCN_PIN, INPUT);

  pcnt_config_t pcnt_config = {
      .pulse_gpio_num = PCN_PIN,
      .ctrl_gpio_num = PCNT_PIN_NOT_USED,
      .lctrl_mode = PCNT_MODE_KEEP,
      .hctrl_mode = PCNT_MODE_KEEP,
      .pos_mode = PCNT_COUNT_INC,
      .neg_mode = PCNT_COUNT_INC,
      .counter_h_lim = 32767,
      .counter_l_lim = -32768,
      .unit = UNIT,
      .channel = PCNT_CHANNEL_0,
  };

  pcnt_unit_config(&pcnt_config);
  pcnt_set_filter_value(UNIT, 1023);
  pcnt_filter_enable(UNIT);
  pcnt_counter_pause(UNIT);
  pcnt_counter_clear(UNIT);
  pcnt_counter_resume(UNIT);
}

} // namespace PhotoReflector
