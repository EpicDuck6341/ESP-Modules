#pragma once
#include "hardware_pins.h"
#include "pid_config.h"
#include "display_config.h"
#include "opentherm_params.h"
#include <Arduino_JSON.h>

/* ----------  Display Functions ---------- */
void drawBitmapInverted(int16_t x, int16_t y,
  const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color) {
    int16_t i, j, byteWidth = (w + 7) / 8;
    uint8_t byte;
    for(j=0; j<h; j++) {
        for(i=0; i<w; i++) {
            if(i & 7) byte <<= 1;
            else      byte   = pgm_read_byte(bitmap + j * byteWidth + i / 8);
            if(!(byte & 0x80)) tft.drawPixel(x+i, y+j, color);
        }
    }
}

void pushValue(float arr[], float newValue) {
  for (int i = 0; i < maxSize - 1; i++) arr[i] = arr[i + 1];
  arr[maxSize - 1] = newValue;
}

void drawTempGraph(float array[], int sz, int maxSZ) {
    if (sz < 2) return;
    const float minTemp = 15.0, maxTemp = 35.0;
    const int graphTop = 51, graphBottom = 175, graphHeight = graphBottom - graphTop, graphWidth = 180;
    tft.fillRect(0, graphTop, graphWidth, graphHeight, ILI9341_WHITE);
    float graphXGap = (float)graphWidth / (sz - 1);
    for (int i = 0; i < sz - 1; i++) {
        float v1 = array[i], v2 = array[i+1];
        v1 = constrain(v1, minTemp, maxTemp);
        v2 = constrain(v2, minTemp, maxTemp);
        int y1 = graphBottom - ((v1 - minTemp) * graphHeight) / (maxTemp - minTemp);
        int y2 = graphBottom - ((v2 - minTemp) * graphHeight) / (maxTemp - minTemp);
        int x1 = graphXGap * i, x2 = graphXGap * (i+1);
        tft.drawLine(x1, y1, x2, y2, ILI9341_BLACK);
    }
}

void updateTarget(float newT) {
  tft.setTextSize(4);
  tft.setCursor(185, 145);
  tft.setTextColor(ILI9341_BLACK, ILI9341_WHITE);
  tft.print(newT,1);
}

void updateCurrent(float newT) {
  tft.setTextSize(4);
  tft.setCursor(185, 80);
  tft.setTextColor(ILI9341_BLACK, ILI9341_WHITE);
  tft.print(newT,1);
}

bool isPressed(int x, int y, int w, int h, TS_Point p) {
  return (p.x >= x-w && p.x <= x + w && p.y >= y-h && p.y <= y + h);
}

void IRAM_ATTR handleInterrupt() { ot.handleInterrupt(); }

/* ---------- OpenTherm Functions ---------- */
void setStatus(bool enableCH, bool enableDHW, bool enableCool) {
    unsigned long response = ot.setBoilerStatus(enableCH, enableDHW, enableCool);
    OpenThermResponseStatus responseStatus = ot.getLastResponseStatus();
    if (responseStatus == OpenThermResponseStatus::SUCCESS) {
        Serial.println("Central Heating: " + String(ot.isCentralHeatingActive(response) ? "on" : "off"));
        Serial.println("Hot Water: " + String(ot.isHotWaterActive(response) ? "on" : "off"));
        Serial.println("Flame: " + String(ot.isFlameOn(response) ? "on" : "off"));
    } else if (responseStatus == OpenThermResponseStatus::NONE) {
        Serial.println("Error: OpenTherm not initialized");
    } else if (responseStatus == OpenThermResponseStatus::INVALID) {
        Serial.println("Error: Invalid response " + String(response, HEX));
    } else if (responseStatus == OpenThermResponseStatus::TIMEOUT) {
        Serial.println("Error: Response timeout");
    }
}

void runPID() {
    float error = setPoint - currentTemp;
    //  bool enableCentralHeating = true;
    // bool enableHotWater = true;
    // bool enableCooling = false;
    unsigned long response = ot.setBoilerStatus(enableCentralHeating, enableHotWater, enableCooling);

    if (abs(error) < DEAD_BAND) {
        return;
    }

    float dt = (float)PID_INTERVAL / 1000.0; 

    integral += error * dt;
    integral = constrain(integral, INTEGRAL_MIN, INTEGRAL_MAX);

    float derivative = (error - lastError) / dt;
    lastError = error;

    float pidOutput = Kp * error + Ki * integral + Kd * derivative;

    float boilerTemp = constrain(baseTemp + pidOutput, minBoilerTemp, maxBoilerTemp);

    ot.setBoilerTemperature(boilerTemp);
    float ch_temperature = ot.getBoilerTemperature();

    Serial.println("--------------------------------------------------");
    Serial.print("Current Temp: "); Serial.println(currentTemp);
    Serial.print("Setpoint: "); Serial.println(setPoint);
    Serial.print("Error: "); Serial.println(error);
    Serial.print("Integral: "); Serial.println(integral);
    Serial.print("Derivative: "); Serial.println(derivative);
    Serial.print("PID output: "); Serial.println(pidOutput);
    Serial.print("Boiler Temp Sent: "); Serial.println(boilerTemp);
    Serial.println("CH temperature is " + String(ch_temperature) + " degrees C");
    Serial.println("--------------------------------------------------");
}

void setDHW(float DHW){
// Set DHW setpoint to 40 degrees C
    ot.setDHWSetpoint(DHW);

    // Get DHW Temperature
    float dhw_temperature = ot.getDHWTemperature();
    Serial.println("DHW temperature is " + String(dhw_temperature) + " degrees C");
}


void updateConfigFromJson(JSONVar &config) {
    for (int i = 0; i < paramCount; i++) {
        OTParam &p = params[i];
        String idStr = String(p.id);

        if (config[idStr] != undefined) {
            JSONVar obj = config[idStr];

            if (obj["interval"] != undefined) {
                p.interval = (unsigned long)obj["interval"];
            }

            if (obj["threshold"] != undefined) {
                p.threshold = (float)(double)obj["threshold"];
            }

            Serial.print("Updated ID "); Serial.print(p.id);
            Serial.print(": interval="); Serial.print(p.interval);
            Serial.print(", threshold="); Serial.println(p.threshold);
        }
    }
}

float readOTValue(uint8_t id) {
    unsigned long request = ot.buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)id, 0);
    unsigned long response = ot.sendRequest(request);
    if (ot.isValidResponse(response)) return (float)(response & 0xFFFF)/256.0;
    return NAN;
}

void checkAndSendParam(OTParam &p, unsigned long now) {
    float value = readOTValue(p.id);
    if (isnan(value)) return;
    bool send = false;
    if (p.threshold > 0 && abs(value - p.lastValue) >= p.threshold) send = true;
    if (now - p.lastSentMillis >= p.interval) send = true;
    if (send) {
        JSONVar outJson;
        outJson["id"] = p.id;
        outJson["value"] = value;
        outJson["timestamp"] = now;
        Serial.println(JSON.stringify(outJson));
        p.lastValue = value;
        p.lastSentMillis = now;
    }
    p.lastReadMillis = now;
}


void handleJson(JSONVar myObject){
    if(JSON.typeof(myObject)=="undefined") return;
    if(!myObject.hasOwnProperty("ID")) { Serial.println("JSON missing ID"); return; }
    String ID = (const char*)myObject["ID"];
    if(ID=="boilerStatus"){
        enableCentralHeating = (bool)myObject["enableCentralHeating"];
        enableHotWater = (bool)myObject["enableHotWater"];
        enableCooling = (bool)myObject["enableCooling"];
        setStatus(enableCentralHeating, enableHotWater, enableCooling);
    } else if(ID=="setPoint") { setPoint=(float)(double)myObject["value"]; updateTarget(setPoint); }
    else if(ID=="currentTemp") { currentTemp=(float)(double)myObject["value"];
        if(size==maxSize) pushValue(currentTemps,currentTemp); else currentTemps[size++]=currentTemp;
        updateCurrent(currentTemp);
        drawTempGraph(currentTemps,size,maxSize);
    } else if(ID=="config") { JSONVar cfg = myObject["values"]; updateConfigFromJson(cfg); }
}

void drawInterface() {
  tft.fillScreen(BACKGROUND_COLOR);
  tft.fillRect(0, 0, 320, 50, topBar);
  tft.fillRect(0, 180, 180, 60, topBar);
  
  //Draw Logo
  drawBitmapInverted(logoX, logoY, epd_bitmap_images, BMP_WIDTH, BMP_HEIGHT, logoBlue);
  drawBitmapInverted(120, 185, epd_bitmap_cute_green_budgie_512x512, 60, 60, logoBlue);
  // Draw title
  tft.setTextColor(logoRed);
  tft.setTextSize(3.5);
  tft.setCursor(textOffsetX, textOffsetY);
  tft.print("Breijer");


  // Draw Target temperature
  tft.setTextSize(4);
  tft.setCursor(185, 145);
  tft.setTextColor(TEXT_COLOR, BACKGROUND_COLOR);
  tft.print(setPoint,1);

  tft.fillCircle(288,145, 4, ILI9341_BLACK); 
  tft.fillCircle(288,145, 2, ILI9341_WHITE); 
    
  tft.setTextSize(4);
  tft.setCursor(295, 145);
  tft.setTextColor(TEXT_COLOR, BACKGROUND_COLOR);
  tft.print("C");

  
  tft.setTextSize(2);
  tft.setCursor(185, 120);
  tft.setTextColor(TEXT_COLOR, BACKGROUND_COLOR);
  tft.print("Target");


  // Draw Current temperature                                                                                                                                                                                                                                                                
  tft.setTextSize(4);
  tft.setCursor(185, 80);
  tft.setTextColor(TEXT_COLOR, BACKGROUND_COLOR);
  tft.print(currentTemp,1);

  tft.fillCircle(288,80, 4, ILI9341_BLACK); 
  tft.fillCircle(288,80, 2, ILI9341_WHITE); 
    
  tft.setTextSize(4);
  tft.setCursor(295, 80);
  tft.setTextColor(TEXT_COLOR, BACKGROUND_COLOR);
  tft.print("C");

  tft.setTextSize(2);
  tft.setCursor(190, 55);
  tft.setTextColor(TEXT_COLOR, BACKGROUND_COLOR);
  tft.print("Current");


  // Draw plus button
  tft.fillRect(plusX, plusY, buttonWidth, buttonHeight, buttonRed);
  tft.setTextSize(4);
  tft.setCursor(plusX+25, plusY+20);
  tft.setTextColor(BUTTON_TEXT_COLOR);
  tft.print("+");

  // Draw minus button
  tft.fillRect(minusX, minusY, buttonWidth, buttonHeight, buttonBlue);
  tft.setCursor(minusX+25, minusY+20);
  tft.setTextColor(BUTTON_TEXT_COLOR);
  tft.print("-");


  //Interface Lines
  tft.drawLine(180, 50, 180, 240, ILI9341_BLACK);//Vertical Line
  tft.drawLine(0,50, 360, 50, ILI9341_BLACK); //Top line under logo
  tft.drawLine(180,115, 360,115, ILI9341_BLACK);// Right side section top divider
  tft.drawLine(0,180, 360,180, ILI9341_BLACK);// Right side section bottom divider
  tft.drawLine(250,180,250 ,240, ILI9341_BLACK);// Right side section button divider

  tft.setTextSize(3);
  tft.setCursor(5, 183);
  tft.setTextColor(TEXT_COLOR, topBar);
  tft.print("Status");

  tft.setTextSize(3);
  tft.setCursor(5, 215);
  tft.setTextColor(ILI9341_GREEN, topBar);
  tft.print("Good");


  drawTempGraph(currentTemps,size,maxSize);
}
