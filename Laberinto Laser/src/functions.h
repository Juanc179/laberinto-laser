#pragma once
#include <Arduino.h>
esp_err_t gpio_declarations(void);
void playAudioInterrupt(uint8_t trackIdx);
void setRedLighting(bool on);  
void setGreenLighting(bool on);
void setLasers(bool on);
void blinkLasers(int times, int delayMs = 200);