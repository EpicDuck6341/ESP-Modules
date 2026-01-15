#ifndef ZIGBEE_MODE_ED
#error "Zigbee end device mode is not selected in Tools->Zigbee mode"
#endif

#include "Globals.h"
#include "ZigbeeCallbacks.h"
#include "Sensors.h"
#include "SleepAndBatching.h"

void setup() {

  Serial.begin(115200);
  sensorSetup();

  rgbLedWrite(led, 0, 0, 0);
  pinMode(button, INPUT_PULLUP);

  zbDimmableLight.onLightChange(setLight);
  zbDimmableLight.onIdentify(identify);

  zbDimmableLight.setManufacturerAndModel("Espressif", "ZBLightBulb");

  zbTempSensor.setManufacturerAndModel("Espressif", "ZigbeeTempSensor");
  zbTempSensor.setMinMaxValue(0, 50);
  zbTempSensor.addHumiditySensor(0, 100, 100);

  zbCO2Sensor.setManufacturerAndModel("Espressif", "ZigbeeCarbonDioxideSensor");
  zbCO2Sensor.setMinMaxValue(0, 5000);

  Zigbee.addEndpoint(&zbTempSensor);
  Zigbee.addEndpoint(&zbCO2Sensor);
  Zigbee.addEndpoint(&zbDimmableLight);

  if (!Zigbee.begin()) {
    ESP.restart();
  }

  while (!Zigbee.connected()) {
    delay(100);
  }

  uint32_t configuredInterval = (minRep > 0) ? minRep : 900;
  sleepAndReport(configuredInterval, 30);
}

void loop() {
  if (digitalRead(button) == LOW) {
    delay(100);
    if (digitalRead(button) == LOW) {
      Zigbee.factoryReset();
    }
  }
}
