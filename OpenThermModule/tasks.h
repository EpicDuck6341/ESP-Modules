#pragma once
#include "functions.h"

/* ---------- FreeRTOS Tasks ---------- */
void task_serial_json_handler(void *parameter){
    for(;;){
        if(Serial.available()){
            String jsonString=Serial.readStringUntil('\n');
            JSONVar obj=JSON.parse(jsonString);
            handleJson(obj);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void task_pid_control(void *parameter){
    for(;;){
        if(enableCentralHeating && enableHotWater && currentTemp>0) runPID();
        vTaskDelay(pdMS_TO_TICKS(PID_INTERVAL));
    }
}

void task_opentherm_poller(void *parameter){
    for(;;){
        unsigned long now=millis();
        checkAndSendParam(params[otIndex],now);
        otIndex=(otIndex+1)%paramCount;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void task_touchscreen_handler(void *parameter){
    unsigned long lastTouch=0;
    for(;;){
        if(ts.touched()){
            unsigned long now=millis();
            TS_Point p=ts.getPoint();
            int x=map(p.y,200,3800,0,tft.width());
            int y=map(p.x,200,3800,0,tft.height());
            TS_Point mapped={(int16_t)x,(int16_t)y,p.z};
            if(now-lastTouch>=200){
                bool touched=false;
                if(isPressed(37.5,28.75,37.5,28.75,mapped) && setPoint<35){ setPoint+=0.5f; updateTarget(setPoint); touched=true; }
                else if(isPressed(37.5,86.25,37.5,28.75,mapped) && setPoint>17){ setPoint-=0.5f; updateTarget(setPoint); touched=true; }
                if(touched) lastTouch=now;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
