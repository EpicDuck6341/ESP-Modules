#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <OpenTherm.h>

/* ----------  hardware pins  ---------- */
#define TFT_CS   5
#define TFT_DC  16
#define TFT_RST 17
#define TOUCH_CS 15
#define TOUCH_IRQ 22

/* ----------  objects  ---------- */
Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);
OpenTherm ot(25, 32);

int otIndex = 0;
