#pragma once
#include "Globals.h"
#include "Sensors.h"
#include "esp_sleep.h"

void sleepAndReport(uint32_t sleepTimeSec, uint32_t awakeTimeSec) {

  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

  if (cause != ESP_SLEEP_WAKEUP_TIMER) {
    Serial.printf("Initial sleep for %d seconds...\n", sleepTimeSec);
    esp_sleep_enable_timer_wakeup(sleepTimeSec * 1000000ULL);
    esp_deep_sleep_start();
  }

  Serial.printf("Woke up! Sending reports...\n");

  uint32_t subInterval = (sleepTimeSec > 0) ? (sleepTimeSec / SUB_SAMPLES) : 0;
  if (subInterval == 0) subInterval = sleepTimeSec;

  float tsens_value = readTemp();
  float hsens_value = readHumidity();
  float csens_value = 5;

  if (rtc_sampleCount < SUB_SAMPLES) {
    rtc_storedTemp[rtc_sampleCount] = tsens_value;
    rtc_storedHum[rtc_sampleCount] = hsens_value;
    rtc_storedCO2[rtc_sampleCount] = csens_value;
    rtc_sampleCount++;
  }

  if (rtc_sampleCount < SUB_SAMPLES) {
    esp_sleep_enable_timer_wakeup(subInterval * 1000000ULL);
    esp_deep_sleep_start();
  }

  for (int i = 0; i < SUB_SAMPLES; i++) {
    zbTempSensor.setTemperature(rtc_storedTemp[i]);
    zbTempSensor.setHumidity(rtc_storedHum[i]);
    zbCO2Sensor.setCarbonDioxide(rtc_storedCO2[i]);
    delay(100);
  }

  rtc_sampleCount = 0;
  rtc_secondsSinceLastWeekly += sleepTimeSec;

  if (rtc_secondsSinceLastWeekly >= WEEKLY_PERIOD_SEC) {
    rtc_secondsSinceLastWeekly = 0;

    zbTempSensor.setTemperature(0);
    zbTempSensor.setHumidity(0);
    zbCO2Sensor.setCarbonDioxide(0);

    uint32_t start = millis();
    while ((millis() - start) < WEEKLY_EXT_AWAKE_SEC * 1000UL) {
      if (newConfigReceived) {
        sleepTimeSec = nextSleepTimeSec;
        newConfigReceived = false;
      }
      delay(100);
    }
  }

  uint32_t start = millis();
  while ((millis() - start) < awakeTimeSec * 1000) {
    if (newConfigReceived) {
      sleepTimeSec = nextSleepTimeSec;
      newConfigReceived = false;
    }
    delay(100);
  }

  esp_sleep_enable_timer_wakeup(subInterval * 1000000ULL);
  esp_deep_sleep_start();
}
