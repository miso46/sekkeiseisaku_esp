#pragma once

#include <Arduino.h>
#include <driver/pcnt.h>

extern const int PCN_PIN;
extern pcnt_unit_t unit;

namespace PhotoReflector {

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
      .unit = unit,
      .channel = PCNT_CHANNEL_0,
  };

  pcnt_unit_config(&pcnt_config);

  pcnt_set_filter_value(unit, 1023);
  pcnt_filter_enable(unit);

  pcnt_counter_pause(unit);
  pcnt_counter_clear(unit);
  pcnt_counter_resume(unit);
}
} // namespace PhotoReflector
