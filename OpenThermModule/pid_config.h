#pragma once
#include <Arduino.h>

/* ----------  PID & setpoint  ---------- */
float Kp = 1.0, Ki = 0.01, Kd = 0.1;
float integral = 0, lastError = 0;
float setPoint = 22.0;
int minBoilerTemp = 30, maxBoilerTemp = 80, baseTemp = 30;
bool enableCentralHeating = true, enableHotWater = true, enableCooling = true;
float currentTemp = 20;
unsigned long lastPIDTime = 0;
const unsigned long PID_INTERVAL = 1000;
const float INTEGRAL_MAX = 20.0, INTEGRAL_MIN = -20.0, DEAD_BAND = 0.2;
