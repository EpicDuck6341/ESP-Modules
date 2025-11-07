/*
 * Copyright (c) 2025, Sensirion AG
 * All rights reserved.
 */
#include <Arduino.h>
#include <SensirionI2cScd4x.h>
#include <Wire.h>
#include <arduino-sht.h>
#include <Insights.h>

// macro definitions
// make sure that we use the proper definition of NO_ERROR
#ifdef NO_ERROR
#undef NO_ERROR
#endif
#define NO_ERROR 0


#define SDA_PIN 6
#define SCL_PIN 7

SensirionI2cScd4x sensor;
SHTSensor sht;

static char errorMessage[64];
static int16_t error;

// void PrintUint64(uint64_t& value) {
//   Serial.print("0x");
//   Serial.print((uint32_t)(value >> 32), HEX);
//   Serial.print((uint32_t)(value & 0xFFFFFFFF), HEX);
// }



void sensorSetup() {

  Serial.begin(115200);
  while (!Serial) {
    delay(100);
  }
  Wire.begin(SDA_PIN, SCL_PIN);
  // sensor.begin(Wire, SCD41_I2C_ADDR_62);
  sht.init(Wire);

  Serial.println("Sensors initialized!");

  // uint64_t serialNumber = 0;
  // delay(30);
  // // Ensure sensor is in clean state
  // error = sensor.wakeUp();
  // if (error != NO_ERROR) {
  //   Serial.print("Error trying to execute wakeUp(): ");
  //   errorToString(error, errorMessage, sizeof errorMessage);
  //   Serial.println(errorMessage);
  // }
  // error = sensor.stopPeriodicMeasurement();
  // if (error != NO_ERROR) {
  //   Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
  //   errorToString(error, errorMessage, sizeof errorMessage);
  //   Serial.println(errorMessage);
  // }
  // error = sensor.reinit();
  // if (error != NO_ERROR) {
  //   Serial.print("Error trying to execute reinit(): ");
  //   errorToString(error, errorMessage, sizeof errorMessage);
  //   Serial.println(errorMessage);
  // }
  // // Read out information about the sensor
  // error = sensor.getSerialNumber(serialNumber);
  // if (error != NO_ERROR) {
  //   Serial.print("Error trying to execute getSerialNumber(): ");
  //   errorToString(error, errorMessage, sizeof errorMessage);
  //   Serial.println(errorMessage);
  //   return;
  // }
  // Serial.print("serial number: ");
  // PrintUint64(serialNumber);
  // Serial.println();
  // //
  // // If temperature offset and/or sensor altitude compensation
  // // is required, you should call the respective functions here.
  // // Check out the header file for the function definitions.
  // // Start periodic measurements (5sec interval)
  // error = sensor.startPeriodicMeasurement();
  // if (error != NO_ERROR) {
  //   Serial.print("Error trying to execute startPeriodicMeasurement(): ");
  //   errorToString(error, errorMessage, sizeof errorMessage);
  //   Serial.println(errorMessage);
  //   return;
  // }
  //
  // If low-power mode is required, switch to the low power
  // measurement function instead of the standard measurement
  // function above. Check out the header file for the definition.
  // For SCD41, you can also check out the single shot measurement example.
  //
}


// void errorCheck(uint16_t &co2Concentration) {
//   bool dataReady = false;

//   error = sensor.getDataReadyStatus(dataReady);
//   if (error != NO_ERROR) {
//     Serial.print("Error trying to execute getDataReadyStatus(): ");
//     errorToString(error, errorMessage, sizeof errorMessage);
//     Serial.println(errorMessage);
//     return;
//   }

//   while (!dataReady) {
//     delay(100);
//     error = sensor.getDataReadyStatus(dataReady);
//     if (error != NO_ERROR) {
//       Serial.print("Error trying to execute getDataReadyStatus(): ");
//       errorToString(error, errorMessage, sizeof errorMessage);
//       Serial.println(errorMessage);
//       return;
//     }
//   }

//   float temperature = 0.0;
//   float humidity = 0.0;

//   error = sensor.readMeasurement(co2Concentration,temperature,humidity);
//   if (error != NO_ERROR) {
//     Serial.print("Error trying to execute readMeasurement(): ");
//     errorToString(error, errorMessage, sizeof errorMessage);
//     Serial.println(errorMessage);
//     return;
//   }
// }


// uint16_t readCO2() {
  
//   uint16_t co2Concentration = 0;
//   delay(1000);
//   errorCheck(co2Concentration);
//   Serial.print("CO2 concentration [ppm]: ");
//   Serial.print(co2Concentration);
//   Serial.println();

//   return co2Concentration;
// }

float readTemp() {
  sht.readSample();
  float temp = sht.getTemperature();
  // Serial.print("The Temperature is: ");
  // Serial.print(temp);
  // Serial.println(" C");
  return temp;
}

float readHumidity() {
  sht.readSample();
  float humidity = sht.getHumidity();
  // Serial.print("The Humidity is: ");
  // Serial.print(humidity);
  // Serial.println("%");
  return humidity;
}
