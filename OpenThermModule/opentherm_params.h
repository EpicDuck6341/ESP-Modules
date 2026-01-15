#pragma once
#include <Arduino.h>

struct OTParam {
    uint8_t id;
    float lastValue;
    float threshold;
    unsigned long interval;
    unsigned long lastReadMillis;
    unsigned long lastSentMillis;
};

OTParam params[] = {
    {5, 0, 0, 60000, 0, 0},
    {15, 0, 0, 60000, 0, 0},
    {17, 0, 1.0, 1000, 0, 0},
    {18, 0, 0, 60000, 0, 0},
    {25, 0, 0.5, 1000, 0, 0},
    {28, 0, 0.5, 1000, 0, 0},
    {35, 0, 10, 500, 0, 0},
    {36, 0, 50, 500, 0, 0},
    {113, 0, 0, 60000, 0, 0},
    {114, 0, 0, 60000, 0, 0},
    {116, 0, 0, 1000, 0, 0},
    {120, 0, 0, 60000, 0, 0},
    {121, 0, 0, 60000, 0, 0}
};
const int paramCount = sizeof(params) / sizeof(params[0]);
