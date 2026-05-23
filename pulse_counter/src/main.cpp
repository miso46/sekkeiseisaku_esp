#include "HardwareSerial.h"
#include "esp32-hal-ledc.h"
#include <Arduino.h>
#include <driver/pcnt.h>

const int PCN_PIN = 4;
const int PWM_PIN = 12;
pcnt_unit_t unit = PCNT_UNIT_0;

void setup() {
  Serial.begin(115200);

  const int freq = 10;
  const int bit = 8;
  ledcSetup(0, freq, bit);
  ledcAttachPin(PWM_PIN, 0);
  ledcWrite(0, 128);

  pcnt_config_t pcnt_config = {
      .pulse_gpio_num = PCN_PIN,
      .ctrl_gpio_num = PCNT_PIN_NOT_USED,
      .lctrl_mode = PCNT_MODE_KEEP,
      .hctrl_mode = PCNT_MODE_KEEP,
      .pos_mode = PCNT_COUNT_INC,
      .neg_mode = PCNT_COUNT_DIS,
      .counter_h_lim = 32767,
      .counter_l_lim = -32768,
      .unit = unit,
      .channel = PCNT_CHANNEL_0,
  };
  pcnt_unit_config(&pcnt_config);

  pcnt_counter_clear(unit);
  pcnt_counter_resume(unit);
}

void loop() {
  int16_t count;
  pcnt_get_counter_value(unit, &count);
  Serial.printf("PCNT count: %d\n", count);
  delay(1000);
}
