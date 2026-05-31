// #include <Arduino.h>
// #include <driver/pcnt.h>
// const int SENSOR_PIN = 4;
//
// void setup() {
//   Serial.begin(115200);
//   pinMode(SENSOR_PIN, INPUT); // ピンを入力モードに設定
// }
//
// void loop() {
//   int sensorState = digitalRead(SENSOR_PIN);
//
//   if (sensorState == HIGH) {
//     Serial.println("HIGH (1) : 遮光されています (物が挟まっている状態)");
//   } else {
//     Serial.println("LOW  (0) : 受光しています (何もない状態)");
//   }
//
//   delay(500); // 0.5秒ごとに確認
// }

#include <Arduino.h>
#include <driver/pcnt.h>

const int PCN_PIN = 4;
pcnt_unit_t unit = PCNT_UNIT_0;

void setup() {
  Serial.begin(115200);

  pinMode(PCN_PIN, INPUT);

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

  pcnt_set_filter_value(unit, 1023);
  pcnt_filter_enable(unit);

  pcnt_counter_pause(unit);
  pcnt_counter_clear(unit);
  pcnt_counter_resume(unit);
}

void loop() {
  int16_t count;
  pcnt_get_counter_value(unit, &count);
  Serial.printf("PCNT count: %d\n", count);
  delay(1000);
}
