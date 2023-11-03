#include <Arduino.h>
#include <Wire.h>
#define DEBUGLOG_DEFAULT_LOG_LEVEL_DEBUG
#include "DebugLog.h"

#include "Si4731_driver.h"

radio_modules::Si4731 rad_mod{
    15,14,Wire
}; 

char buffer[255]; 

void setup() {  
    sleep_ms(4000); 
    if (rad_mod.init()) 
    {
        LOG_INFO("init success"); 
    }
    else 
    {
        LOG_ERROR("init failed"); 
    }
}



void loop() {
  // put your main code here, to run repeatedly:
  if (Serial.available() > 0)
  {
    uint8_t out = Serial.read(); 
    if (out == 'u')
    {
        LOG_DEBUG("commanding tune up ");
        rad_mod.tune_frequency(true); 
        rad_mod.get_radio_frequency();  
    }
    else 
    {
        LOG_DEBUG("commanding tune down "); 
        rad_mod.tune_frequency(false); 
        rad_mod.get_radio_frequency(); 
    }
  }
}
