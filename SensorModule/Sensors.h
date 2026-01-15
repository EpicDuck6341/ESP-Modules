#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <SensirionI2cScd4x.h>
#include <arduino-sht.h>
#include <Insights.h>

#ifdef NO_ERROR 
#undef NO_ERROR 
#endif 
#define NO_ERROR 0

/* ================= I2C PINS ================= */

#define SDA_PIN 6
#define SCL_PIN 7

/* ================= SENSOR OBJECTS ================= */

SensirionI2cScd4x sensor;
SHTSensor sht;

static char errorMessage[64]; 

static int16_t error;

void PrintUint64(uint64_t& value) { 
   Serial.print("0x"); 
   Serial.print((uint32_t)(value >> 32), HEX); 
   Serial.print((uint32_t)(value & 0xFFFFFFFF), HEX); 
    }


/* ================= SENSOR SETUP ================= */

void sensorSetup() {

  Serial.begin(115200);
  while (!Serial) {
    delay(100);
  }

  Wire.begin(SDA_PIN, SCL_PIN);
  sensor.begin(Wire, SCD41_I2C_ADDR_62); 
  sht.init(Wire);

  Serial.println("Sensors initialized!");

  
  uint64_t serialNumber = 0;
  delay(30);

  error = sensor.wakeUp();
  if (error != NO_ERROR) {
    Serial.print("Error trying to execute wakeUp(): ");
    errorToString(error, errorMessage, sizeof errorMessage);
    Serial.println(errorMessage);
  }

  error = sensor.stopPeriodicMeasurement();
  if (error != NO_ERROR) {
    Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
    errorToString(error, errorMessage, sizeof errorMessage);
    Serial.println(errorMessage);
  }

  error = sensor.reinit();
  if (error != NO_ERROR) {
    Serial.print("Error trying to execute reinit(): ");
    errorToString(error, errorMessage, sizeof errorMessage);
    Serial.println(errorMessage);
  }

  error = sensor.getSerialNumber(serialNumber);
  if (error != NO_ERROR) {
    Serial.print("Error trying to execute getSerialNumber(): ");
    errorToString(error, errorMessage, sizeof errorMessage);
    Serial.println(errorMessage);
    return;
  }
  Serial.print("serial number: ");
  PrintUint64(serialNumber);
  Serial.println();

  error = sensor.startPeriodicMeasurement();
  if (error != NO_ERROR) {
    Serial.print("Error trying to execute startPeriodicMeasurement(): ");
    errorToString(error, errorMessage, sizeof errorMessage);
    Serial.println(errorMessage);
    return;
  }
  
}

/* ================= TEMPERATURE & HUMIDITY ================= */

float readTemp() {
  sht.readSample();
  return sht.getTemperature();
}

float readHumidity() {
  sht.readSample();
  return sht.getHumidity();
}

/* ================= CO2 MEASUREMENT (COMMENTED) ================= */


void errorCheck(uint16_t &co2Concentration) {
  bool dataReady = false;

  error = sensor.getDataReadyStatus(dataReady);
  if (error != NO_ERROR) {
    Serial.print("Error trying to execute getDataReadyStatus(): ");
    errorToString(error, errorMessage, sizeof errorMessage);
    Serial.println(errorMessage);
    return;
  }

  while (!dataReady) {
    delay(100);
    error = sensor.getDataReadyStatus(dataReady);
    if (error != NO_ERROR) {
      Serial.print("Error trying to execute getDataReadyStatus(): ");
      errorToString(error, errorMessage, sizeof errorMessage);
      Serial.println(errorMessage);
      return;
    }
  }

  float temperature = 0.0;
  float humidity = 0.0;

  error = sensor.readMeasurement(co2Concentration, temperature, humidity);
  if (error != NO_ERROR) {
    Serial.print("Error trying to execute readMeasurement(): ");
    errorToString(error, errorMessage, sizeof errorMessage);
    Serial.println(errorMessage);
    return;
  }
}

uint16_t readCO2() {
  uint16_t co2Concentration = 0;
  delay(1000);
  errorCheck(co2Concentration);
  Serial.print("CO2 concentration [ppm]: ");
  Serial.print(co2Concentration);
  Serial.println();
  return co2Concentration;
}

