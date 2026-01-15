#pragma once
#include "Globals.h"

/********************* RGB LED functions **************************/

void onNewConfig(uint32_t newMinRepSec) {
  nextSleepTimeSec = newMinRepSec;
  newConfigReceived = true;
}

void setLight(bool state, uint8_t level) {
  if (!state) {
    rgbLedWrite(led, 0, 0, 0);
    return;
  }
  if (!firstValue) {
    firstValue = true;
    return;
  }

  Serial.println(level);
  switch (counter) {
    case 0:
      minRep = 60 * level;
      break;
    case 1:
      maxRep = 60 * level;
      break;
    case 2:
      covTemp = level;
      covTemp = covTemp / 10;
      break;
    case 3:
      covHum = level;
      break;
    case 4:
      covCO2 = level;
      onNewConfig(minRep);

      Serial.println("------ Reporting Settings ------");
      Serial.println(minRep);
      Serial.println(maxRep);
      Serial.println(covTemp);
      Serial.println(covHum);
      Serial.println(covCO2);
      Serial.println("--------------------------------");
      break;
  }

  counter = (counter < 4) ? counter + 1 : 0;
  rgbLedWrite(led, level, level, level);
}

/********************* Identify **************************/

void identify(uint16_t time) {
  static uint8_t blink = 1;
  log_d("Identify called for %d seconds", time);
  if (time == 0) {
    zbDimmableLight.restoreLight();
    return;
  }
  rgbLedWrite(led, 255 * blink, 255 * blink, 255 * blink);
  blink = !blink;
}
