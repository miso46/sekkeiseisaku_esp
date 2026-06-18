#ifdef STRESS_TEST

#include "photoreflector.hpp"
#include "esp_now.hpp"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Stress test tasks: simulate high-frequency pulses on PCN_PIN and flood ESP-NOW sends

static void pulseGeneratorTask(void *arg) {
  (void)arg;
  pinMode(PCN_PIN, OUTPUT);
  const unsigned int pulseHighUs = 50; // high time in microseconds
  const unsigned int pulseLowUs = 50;  // low time -> ~10kHz pulses
  unsigned long toggles = 0;
  unsigned long lastPrint = millis();
  Serial.println("pulse_gen: start");
  for (;;) {
    digitalWrite(PCN_PIN, HIGH);
    delayMicroseconds(pulseHighUs);
    digitalWrite(PCN_PIN, LOW);
    delayMicroseconds(pulseLowUs);
    toggles++;
    if (millis() - lastPrint >= 1000) {
      lastPrint = millis();
      // Print the number of toggles and last measured interval from PhotoReflector
      Serial.printf("pulse_gen: toggles=%lu  last_interval_us=%lu\n", toggles, PhotoReflector::getLastIntervalUs());
    }
  }
}

static void floodEspNowTask(void *arg) {
  (void)arg;
  espnow::Data d = {0.f, 0.f, 0.f};
  for (;;) {
    // enqueue many messages quickly
    for (int i = 0; i < 50; ++i) {
      d.current_position = random(0, 1000);
      d.speed_mm_ms = random(0, 1000) / 1000.0f;
      d.arrival_time_ms = random(0, 10000) / 10.0f;
      espnow::enqueueSend(d);
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void startStressTest() {
  xTaskCreatePinnedToCore(pulseGeneratorTask, "pulse_gen", 2048, nullptr, configMAX_PRIORITIES - 3, nullptr, tskNO_AFFINITY);
  xTaskCreatePinnedToCore(floodEspNowTask, "esp_flood", 4096, nullptr, tskIDLE_PRIORITY + 2, nullptr, tskNO_AFFINITY);
}

#endif // STRESS_TEST
