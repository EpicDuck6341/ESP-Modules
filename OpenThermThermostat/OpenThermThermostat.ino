#include <Arduino_JSON.h>
#include <OpenTherm.h>



/*
OpenTherm Master Communication Example
By: Ihor Melnyk
Date: January 19th, 2018

Uses the OpenTherm library to get/set boiler status and water temperature
Open serial monitor at 115200 baud to see output.

Hardware Connections (OpenTherm Adapter (http://ihormelnyk.com/pages/OpenTherm) to Arduino/ESP8266):
-OT1/OT2 = Boiler X1/X2
-VCC = 5V or 3.3V
-GND = GND
-IN  = Arduino (3) / ESP8266 (5) Output Pin
-OUT = Arduino (2) / ESP8266 (4) Input Pin

Controller(Arduino/ESP8266) input pin should support interrupts.
Arduino digital pins usable for interrupts: Uno, Nano, Mini: 2,3; Mega: 2, 3, 18, 19, 20, 21
ESP8266: Interrupts may be attached to any GPIO pin except GPIO16,
but since GPIO6-GPIO11 are typically used to interface with the flash memory ICs on most esp8266 modules, applying interrupts to these pins are likely to cause problems
*/

#include <Arduino.h>
// #include <ArduinoJson.h>


const int inPin = 2;  // for Arduino, 4 for ESP8266 (D2), 21 for ESP32
const int outPin = 3; // for Arduino, 5 for ESP8266 (D1), 22 for ESP32
OpenTherm ot(inPin, outPin);

float Kp = 1.0;
float Ki = 0.01;
float Kd = 0.1;


float integral = 0;
float lastError = 0;
unsigned long lastTime = 0;
float setPoint = 0;

int minBoilerTemp = 30;
int maxBoilerTemp = 80;
int baseTemp = 30;

bool enableCentralHeating = false;
bool enableHotWater = false;
bool enableCooling = false;

float currentTemp = 0;

unsigned long lastPIDTime = 0;
const unsigned long PID_INTERVAL = 1000;
const float INTEGRAL_MAX = 20.0;       
const float INTEGRAL_MIN = -20.0;
const float DEAD_BAND = 0.2;           // Only act if error > 0.2°C


struct OTParam {
    uint8_t id;
    float lastValue;          // last value sent to C#
    float threshold;          // CoV threshold (0 = periodic only)
    unsigned long interval;   // guaranteed send interval in ms
    unsigned long lastReadMillis; // last time we read
    unsigned long lastSentMillis; // last time we sent
};


// Initialize all your IDs with default intervals and thresholds
OTParam params[] = {
    {5, 0, 0, 60000, 0},
    {15, 0, 0, 60000, 0},
    {17, 0, 1.0, 1000, 0},
    {18, 0, 0, 60000, 0},
    {25, 0, 0.5, 1000, 0},
    {28, 0, 0.5, 1000, 0},
    {35, 0, 10, 500, 0},
    {36, 0, 50, 500, 0},
    {113, 0, 0, 60000, 0},
    {114, 0, 0, 60000, 0},
    {116, 0, 0, 1000, 0},
    {120, 0, 0, 60000, 0},
    {121, 0, 0, 60000, 0}
};

const int paramCount = sizeof(params) / sizeof(params[0]);



void IRAM_ATTR handleInterrupt()
{
    ot.handleInterrupt();
}

//Receive JSON from bluebot over serial, switch case 
//Json contains ID : boilerstatus
void setStatus(bool enableCentralHeating, bool enableHotWater, bool enableCooling){

    unsigned long response = ot.setBoilerStatus(enableCentralHeating, enableHotWater, enableCooling);
    OpenThermResponseStatus responseStatus = ot.getLastResponseStatus();
    if (responseStatus == OpenThermResponseStatus::SUCCESS)
    {
        Serial.println("Central Heating: " + String(ot.isCentralHeatingActive(response) ? "on" : "off"));
        Serial.println("Hot Water: " + String(ot.isHotWaterActive(response) ? "on" : "off"));
        Serial.println("Flame: " + String(ot.isFlameOn(response) ? "on" : "off"));
    }
     if (responseStatus == OpenThermResponseStatus::NONE)
    {
        Serial.println("Error: OpenTherm is not initialized");
    }
    else if (responseStatus == OpenThermResponseStatus::INVALID)
    {
        Serial.println("Error: Invalid response " + String(response, HEX));
    }
    else if (responseStatus == OpenThermResponseStatus::TIMEOUT)
    {
        Serial.println("Error: Response timeout");
    }
}

//Runs as long as setpoint is not equal to temp received through JSON
//As long as BlueBot doesnt send temp, use old value
void runPID() {
    float error = setPoint - currentTemp;


    if (abs(error) < DEAD_BAND) {
        Serial.println("Error within deadband. No PID adjustment.");
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

    // Debug print
    Serial.println("--------------------------------------------------");
    Serial.print("Current Temp: "); Serial.println(currentTemp);
    Serial.print("Setpoint: "); Serial.println(setPoint);
    Serial.print("Error: "); Serial.println(error);
    Serial.print("Integral: "); Serial.println(integral);
    Serial.print("Derivative: "); Serial.println(derivative);
    Serial.print("PID output: "); Serial.println(pidOutput);
    Serial.print("Boiler Temp Sent: "); Serial.println(boilerTemp);
    Serial.println("--------------------------------------------------");
}

//Set DHW, IDK when json contains  ID : DHW
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
    // Build a read request for the given ID
    unsigned long request = ot.buildRequest(OpenThermMessageType::READ_DATA, (OpenThermMessageID)id, 0);
    
    // Send the request
    unsigned long response = ot.sendRequest(request);

    // Check if response is valid
    if (ot.isValidResponse(response)) {
        // For F8.8 format (typical temperature/pressure)
        int rawValue = (response & 0xFFFF);          // lower 16 bits
        return (float)rawValue / 256.0;
    } else {
        return NAN;  // or some error code
    }
}



void checkAndSendParam(OTParam &p, unsigned long now) {
    float value = readOTValue(p.id);

    if (isnan(value)) {
        Serial.print("Invalid response for ID "); Serial.println(p.id);
        return;
    }

    bool send = false;

    // CoV override
    if (p.threshold > 0 && abs(value - p.lastValue) >= p.threshold) send = true;

    // Interval send
    if (now - p.lastSentMillis >= p.interval) send = true;

    if (send) {
        // Send JSON over Serial
        JSONVar outJson;
        outJson["id"] = p.id;
        outJson["value"] = value;
        outJson["timestamp"] = now;
        Serial.println(JSON.stringify(outJson));

        // Update tracking
        p.lastValue = value;
        p.lastSentMillis = now;
    }

    p.lastReadMillis = now;
}


void setup()
{
    Serial.begin(115200);
    Serial.println("Start");

    ot.begin(handleInterrupt); 
}


void loop() {
    unsigned long now = millis();

    // --- Check all parameters and send JSON if needed ---
    for (int i = 0; i < paramCount; i++) {
        checkAndSendParam(params[i], now);
    }


    if (Serial.available()) {
        String jsonString = Serial.readStringUntil('\n'); 
        JSONVar myObject = JSON.parse(jsonString);

        if (JSON.typeof(myObject) != "undefined") {
    String ID = (const char*) myObject["ID"];

    if (ID == "boilerStatus") {
        enableCentralHeating = (bool) myObject["enableCentralHeating"];
        enableHotWater = (bool) myObject["enableHotWater"];
        enableCooling = (bool) myObject["enableCooling"];
        setStatus(enableCentralHeating, enableHotWater, enableCooling);
    } 
    else if (ID == "setPoint") {
        setPoint = (float)(double) myObject["value"];
        Serial.print("Target Temp: "); Serial.println(setPoint);
    } 
    else if (ID == "currentTemp") {
        currentTemp = (float)(double) myObject["value"];
        Serial.print("Current Temp: "); Serial.println(currentTemp);
    }
    else if (ID == "config") {
        // This is the new config JSON
        JSONVar cfg = myObject["values"];  // expects {"5": {...}, "17": {...}, ...}
        updateConfigFromJson(cfg);
    }
}

    }

    // --- Run PID periodically ---
    if (enableCentralHeating && enableHotWater && currentTemp > 0 && now - lastPIDTime >= PID_INTERVAL) {
        lastPIDTime = now;
        runPID();
    }
}



