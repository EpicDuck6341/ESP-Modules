#pragma once
#include <Arduino.h>
#include "Zigbee.h"

/* ================= ENDPOINTS ================= */

#define TEMP_SENSOR_ENDPOINT_NUMBER 1
#define CO2_SENSOR_ENDPOINT_NUMBER  2
#define ZIGBEE_LIGHT_ENDPOINT       10

/* ================= STATE ================= */

int counter = 0;
int minRep = 0;
int maxRep = 0;
float covTemp = 0;
float covHum = 0;
float covCO2 = 0;
bool firstValue = false;

uint32_t nextSleepTimeSec = 0;
bool newConfigReceived = false;

/* ================= GPIO ================= */

uint8_t led = RGB_BUILTIN;
uint8_t button = BOOT_PIN;

/* ================= WEEKLY ================= */

#define SUB_SAMPLES 3
#define WEEKLY_EXT_AWAKE_SEC 10
#define WEEKLY_PERIOD_SEC (7UL * 24UL * 60UL * 60UL)

/* ================= RTC ================= */

RTC_DATA_ATTR uint32_t rtc_sampleCount = 0;
RTC_DATA_ATTR float rtc_storedTemp[SUB_SAMPLES];
RTC_DATA_ATTR float rtc_storedHum[SUB_SAMPLES];
RTC_DATA_ATTR float rtc_storedCO2[SUB_SAMPLES];
RTC_DATA_ATTR uint32_t rtc_secondsSinceLastWeekly = 0;

/* ================= ZIGBEE OBJECTS ================= */

ZigbeeTempSensor zbTempSensor(TEMP_SENSOR_ENDPOINT_NUMBER);
ZigbeeCarbonDioxideSensor zbCO2Sensor(CO2_SENSOR_ENDPOINT_NUMBER);
ZigbeeDimmableLight zbDimmableLight(ZIGBEE_LIGHT_ENDPOINT);
