#include <Arduino.h>
#include "photoreflector.hpp"

// Simple debug program: print photoreflector count and last interval every 200 ms

TaskHandle_t g_pulse_print_task = nullptr;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("PhotoReflector Debug Start");

  // デバッグ用: ISR発火のたびに通知を受けてprintするタスクを作成
  xTaskCreate([](void *arg) {
    (void)arg;
    for (;;) {
      // ISRからの通知が来るまでここでブロック（ポーリングではない）
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
      Serial.println("ISR: pulse detected!");
    }
  }, "pulse_print", 4096, NULL, 1, &g_pulse_print_task);

  Serial.println("calling PhotoReflector::setup()...");
  PhotoReflector::setup();
  PhotoReflector::setProcessingTaskHandle(g_pulse_print_task); // ← これが重要
  Serial.println("returned from PhotoReflector::setup()");
  Serial.printf("post-setup: rmt_active=%d  rmt_batches=%lu\n", PhotoReflector::isReady() ? 1 : 0, PhotoReflector::getRmtBatches());
}

// generate test pulses on PCN_PIN (temporarily switch to OUTPUT)
void generateTestPulses(unsigned int freq_hz, unsigned long duration_ms) {
  unsigned long period_us = 1000000UL / freq_hz;
  unsigned long half = period_us / 2;
  unsigned long end = millis() + duration_ms;
  pinMode(PCN_PIN, OUTPUT);
  while (millis() < end) {
    digitalWrite(PCN_PIN, HIGH);
    delayMicroseconds(half);
    digitalWrite(PCN_PIN, LOW);
    delayMicroseconds(half);
  }
  pinMode(PCN_PIN, INPUT_PULLUP);
}

unsigned long last_print = 0;
unsigned long total_count = 0; // 累積総数（起動からの合計）
unsigned long prev_total_count = 0; // 前回出力時の合計（差分計算用）

// configuration: 1パルスあたり進む距離（mm）
const float MM_PER_PULSE = 5.0f;

// raw level tracking for debugging
int prev_raw_level = -1;

void loop() {
  // check raw digital level and print on change
  int raw = digitalRead(PCN_PIN);
  if (raw != prev_raw_level) {
    prev_raw_level = raw;
    Serial.printf("raw_edge: level=%d  micros=%lu\n", raw, micros());

    // Always increment total_count on observed raw edge so total reflects detected pulses
    total_count += 1;
    Serial.printf("total_count now=%lu\n", total_count);
  }

  unsigned long now = millis();
  // handle serial commands
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 't') {
      Serial.println("Test pulses: generating 500Hz for 500ms");
      generateTestPulses(500, 500);
      Serial.println("Test pulses: done");
    }
  }

  if (now - last_print >= 200) {
    unsigned long delta_ms = now - last_print;
    // read and clear counts since last read
    unsigned long count = PhotoReflector::getAndClearCount();
    total_count += count;
    unsigned long interval_us = PhotoReflector::getLastIntervalUs();
    unsigned long last_pulse = PhotoReflector::getLastPulseUs();
    unsigned long peek = PhotoReflector::peekCount();

    // distance & time deltas
    unsigned long delta_count = total_count - prev_total_count;
    float delta_time_s = delta_ms / 1000.0f; // s
    float delta_distance_m = (delta_count * MM_PER_PULSE) / 1000.0f; // m
    float total_distance_m = (total_count * MM_PER_PULSE) / 1000.0f; // m

    // average speed based on counts since last print
    float avg_speed_m_s = (delta_time_s > 0.0f) ? (delta_distance_m / delta_time_s) : 0.0f;

    // instantaneous speed from last interval (preferred), otherwise fall back to avg
    float inst_speed_m_s = 0.0f;
    float pulses_per_sec = 0.0f;
    if (interval_us > 0) {
      float interval_s = interval_us / 1000000.0f;
      float dist_per_pulse_m = MM_PER_PULSE / 1000.0f;
      inst_speed_m_s = dist_per_pulse_m / interval_s;
      pulses_per_sec = 1.0f / interval_s;
    } else {
      inst_speed_m_s = avg_speed_m_s;
      pulses_per_sec = (delta_time_s > 0.0f) ? ((float)delta_count / delta_time_s) : 0.0f;
    }

    Serial.printf("counts(cleared)=%lu  peek=%lu  last_interval_us=%lu  last_pulse_us=%lu  total=%lu\n",
                  count, peek, interval_us, last_pulse, total_count);
    Serial.printf("distance: total=%.3fm  delta=%.3fm\n", total_distance_m, delta_distance_m);
    unsigned long rmt_batches = 0;
    bool rmt_ok = false;
    // get RMT debug stats if available
    rmt_ok = PhotoReflector::isReady();
    rmt_batches = PhotoReflector::getRmtBatches();
    Serial.printf("rmt: active=%d  batches=%lu\n", rmt_ok ? 1 : 0, rmt_batches);

    Serial.printf("speed: instant=%.3fm/s (%.2fkm/h)  avg(%lums)=%.3fm/s (%.2fkm/h)  pulses/s=%.2f\n",
                  inst_speed_m_s, inst_speed_m_s * 3.6f,
                  delta_ms, avg_speed_m_s, avg_speed_m_s * 3.6f,
                  pulses_per_sec);

    prev_total_count = total_count;
    last_print = now;
  }
  delay(10);
}
