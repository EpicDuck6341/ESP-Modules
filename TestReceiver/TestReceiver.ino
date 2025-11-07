// Copyright 2024 Espressif Systems (Shanghai) PTE LTD
// Licensed under the Apache License, Version 2.0 (the "License");

#ifndef ZIGBEE_MODE_ED
#error "Zigbee end device mode is not selected in Tools->Zigbee mode"
#endif

#include "Zigbee.h"
#include "Sensor.h"
#include "esp_sleep.h"

/* Zigbee dimmable light configuration */
#define TEMP_SENSOR_ENDPOINT_NUMBER 1
#define CO2_SENSOR_ENDPOINT_NUMBER 2
#define ZIGBEE_LIGHT_ENDPOINT 10


// State variables
int counter = 0;
int minRep = 0;
int maxRep = 0;
float covTemp = 0;
float covHum = 0;
float covCO2 = 0;
bool firstValue = false;
uint32_t nextSleepTimeSec = 0; // default 0 minutes
bool newConfigReceived = false;

uint8_t led = RGB_BUILTIN;
uint8_t button = BOOT_PIN;

// Optional Time cluster variables
struct tm timeinfo;
struct tm *localTime;
int32_t timezone;

// Sensor objects
ZigbeeTempSensor zbTempSensor(TEMP_SENSOR_ENDPOINT_NUMBER);
ZigbeeCarbonDioxideSensor zbCO2Sensor(CO2_SENSOR_ENDPOINT_NUMBER);
ZigbeeDimmableLight zbDimmableLight = ZigbeeDimmableLight(ZIGBEE_LIGHT_ENDPOINT);

/********************* RGB LED functions **************************/
void setLight(bool state, uint8_t level) {
  if (!state) {
    rgbLedWrite(led, 0, 0, 0);
    return;
  }
  if(!firstValue){
    firstValue = true;
    return;
  }
  Serial.println(level);
  switch (counter) {
    case 0:
      minRep = 60 * level; // minutes → seconds
      break;
    case 1:
      maxRep = 60 * level; // minutes → seconds
      break;
    case 2:
      covTemp = level; // input: 0.01°C units → 0.1°C per step
      covTemp = covTemp/10;
      break;
    case 3:
      covHum= level; // input already in 0.01% RH
      break;
    case 4:
      covCO2 = level; // input in ppm

      onNewConfig(minRep);

      // Print the settings
      Serial.println("------ Reporting Settings ------");
      Serial.println(minRep);
      Serial.println(maxRep);
      Serial.println(covTemp);
      Serial.println(covHum);
      Serial.println(covCO2);
      Serial.println("--------------------------------");

      // Apply reporting to all sensors
      // zbTempSensor.setTolerance(covTemp);
      // zbTempSensor.setReporting(minRep, maxRep, covTemp);

      // zbTempSensor.setHumidityReporting(minRep, maxRep, covHum);

      // // zbCO2Sensor.setTolerance(covCO2);
      // zbCO2Sensor.setReporting(minRep, maxRep, covCO2);
      
      break;
  }

  counter = (counter < 4) ? counter + 1 : 0;
  rgbLedWrite(led, level, level, level);
}


// Create a task on identify call to handle the identify function
void identify(uint16_t time) {
  static uint8_t blink = 1;
  log_d("Identify called for %d seconds", time);
  if (time == 0) {
    // If identify time is 0, stop blinking and restore light as it was used for identify
    zbDimmableLight.restoreLight();
    return;
  }
  rgbLedWrite(led, 255 * blink, 255 * blink, 255 * blink);
  blink = !blink;
}

void temp_sensor_value_update(void *arg) {
  static float lastTemp = NAN;
  static float lastHum = NAN;

  for (;;) {
    float tsens_value = readTemp();
    float hsens_value = readHumidity();
  //  float tsens_value = 10;
  //   float hsens_value = 50;
    // Report only if changed significantly
    if (isnan(lastTemp) || fabs(tsens_value - lastTemp) > 0.2) {
      zbTempSensor.setTemperature(tsens_value);
      lastTemp = tsens_value;
    }
    if (isnan(lastHum) || fabs(hsens_value - lastHum) > 1.0) {
      zbTempSensor.setHumidity(hsens_value);
      lastHum = hsens_value;
    }

    vTaskDelay(5000 / portTICK_PERIOD_MS);  // ~5s update period
  }
}

// CO₂ sensor
void co2_sensor_value_update(void *arg) {
  static float lastCO2 = NAN;

  for (;;) {
    float csens_value = 5; // Replace with actual readCO2() if available

    if (isnan(lastCO2) || fabs(csens_value - lastCO2) > 20.0) {
      zbCO2Sensor.setCarbonDioxide(csens_value);
      lastCO2 = csens_value;
    }

    vTaskDelay(5000 / portTICK_PERIOD_MS);  // ~5s update period
  }
}


void sleepAndReport(uint32_t sleepTimeSec, uint32_t awakeTimeSec) {
    // Check wakeup reason
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    if (cause != ESP_SLEEP_WAKEUP_TIMER) {
        // First boot, just go to sleep
        Serial.printf("Initial sleep for %d seconds...\n", sleepTimeSec);
        esp_sleep_enable_timer_wakeup(sleepTimeSec * 1000000ULL);
        esp_deep_sleep_start();
    }

    // After waking from deep sleep
    Serial.printf("Woke up! Sending reports...\n");
    zbTempSensor.reportTemperature();
    zbCO2Sensor.report();
    zbDimmableLight.setLightLevel(zbDimmableLight.getLightLevel() + 50);

    // Stay awake for awakeTimeSec
    uint32_t start = millis();
    while ((millis() - start) < awakeTimeSec * 1000) {
        // optional: process incoming config
          if (newConfigReceived) {
            sleepTimeSec = nextSleepTimeSec; // update sleep timer
            newConfigReceived = false;
            Serial.printf("Updated sleep time to %d seconds\n", sleepTimeSec);
        }
        delay(100);
    }

    // Go back to sleep again
    Serial.printf("Going back to sleep for %d seconds...\n", sleepTimeSec);
    esp_sleep_enable_timer_wakeup(sleepTimeSec * 1000000ULL);
    esp_deep_sleep_start();
}

void onNewConfig(uint32_t newMinRepSec) {
    nextSleepTimeSec = newMinRepSec;
    newConfigReceived = true;
}


/********************* Arduino functions **************************/
void setup() {
  Serial.begin(115200);
  sensorSetup();
  // Init RMT and leave light OFF
  rgbLedWrite(led, 0, 0, 0);

  // Init button for factory reset
  pinMode(button, INPUT_PULLUP);

  // Set callback function for light change
  zbDimmableLight.onLightChange(setLight);

  // Optional: Set callback function for device identify
  zbDimmableLight.onIdentify(identify);

  // Optional: Set Zigbee device name and model
  zbDimmableLight.setManufacturerAndModel("Espressif", "ZBLightBulb");

   zbTempSensor.setManufacturerAndModel("Espressif", "ZigbeeTempSensor");
  zbTempSensor.setMinMaxValue(0, 50);
  zbTempSensor.addHumiditySensor(0, 100, 100);

  zbCO2Sensor.setManufacturerAndModel("Espressif", "ZigbeeCarbonDioxideSensor");
  zbCO2Sensor.setMinMaxValue(0, 5000);

  zbTempSensor.setTolerance(1);
  zbCO2Sensor.setTolerance(1);

  // Add endpoint to Zigbee Core
  Serial.println("Adding ZigbeeLight endpoint to Zigbee Core");
   Zigbee.addEndpoint(&zbTempSensor);
  Zigbee.addEndpoint(&zbCO2Sensor);
  Zigbee.addEndpoint(&zbDimmableLight);

  // When all EPs are registered, start Zigbee in End Device mode
  if (!Zigbee.begin()) {
    Serial.println("Zigbee failed to start!");
    Serial.println("Rebooting...");
    ESP.restart();
  }
  Serial.println("Connecting to network");
  while (!Zigbee.connected()) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();

  xTaskCreate(temp_sensor_value_update, "temp_sensor_update", 4096, NULL, 3, NULL);
  xTaskCreate(co2_sensor_value_update, "co2_sensor_update", 4096, NULL, 3, NULL);

  // Set reporting interval for temperature measurement in seconds, must be called after Zigbee.begin()
  // min_interval and max_interval in seconds, delta (temp change in 0,1 °C)
  // if min = 1 and max = 0, reporting is sent only when temperature changes by delta
  // if min = 0 and max = 10, reporting is sent every 10 seconds or temperature changes by delta
  // if min = 0, max = 10 and delta = 0, reporting is sent every 10 seconds regardless of temperature change
  // zbTempSensor.setReporting(1, 0, 0);
  // zbTempSensor.setHumidityReporting(1, 0, 0);
  // zbCO2Sensor.setReporting(1, 0, 1);

  sleepAndReport(5,30);
}

void loop() {
  // Checking button for factory reset
  if (digitalRead(button) == LOW) {  // Push button pressed
    // Key debounce handling
    delay(100);
    int startTime = millis();
    while (digitalRead(button) == LOW) {
      delay(50);
      if ((millis() - startTime) > 3000) {
        // If key pressed for more than 3secs, factory reset Zigbee and reboot
        Serial.println("Resetting Zigbee to factory and rebooting in 1s.");
        delay(1000);
        Zigbee.factoryReset();
      }
    }
    // Increase blightness by 50 every time the button is pressed
    zbDimmableLight.setLightLevel(zbDimmableLight.getLightLevel() + 50);
    // zbTempSensor.reportTemperature();
    // zbCO2Sensor.report();
  }
  delay(100);
}
