#pragma once
#include "hardware_pins.h"
#include "pid_config.h"
#include "display_config.h"
#include "opentherm_params.h"
#include "functions.h"
#include "tasks.h"

void setup(){
    Serial.begin(115200);
    Serial.println("Start RTOS Thermostat");
    ot.begin(handleInterrupt);
    tft.begin(); ts.begin(); ts.setRotation(1); tft.setRotation(1);
    drawInterface();
    xTaskCreatePinnedToCore(task_serial_json_handler,"JSON_Handler",10000,NULL,1,NULL,1);
    xTaskCreatePinnedToCore(task_pid_control,"PID_Control",5000,NULL,2,NULL,1);
    xTaskCreatePinnedToCore(task_opentherm_poller,"OT_Poller",5000,NULL,2,NULL,1);
    xTaskCreatePinnedToCore(task_touchscreen_handler,"Touch_Handler",5000,NULL,3,NULL,1);
}

void loop(){
    vTaskDelay(1);
}
