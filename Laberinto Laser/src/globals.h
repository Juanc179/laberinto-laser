#pragma once
#include <Arduino.h>
#include <ShiftRegister74HC595.h>
#include <DFRobotDFPlayerMini.h>
#include <PCF8574.h>


extern const int rfPins[4];
extern const unsigned long LONG_PRESS_MS;
extern QueueHandle_t rfEventQueue;
extern QueueHandle_t mainTaskQueue;
extern ShiftRegister74HC595<2> sr;
extern HardwareSerial myDFPlayerSerial;
extern DFRobotDFPlayerMini myDFPlayer;
extern PCF8574 pcf;

enum RfEventType { SHORT_PRESS, LONG_PRESS };
struct RfEvent {
  uint8_t channel;
  RfEventType type;
};
struct MainTaskMsg {
  uint8_t channel;
  RfEventType type;
};

extern volatile unsigned long pressStart[4];
extern volatile bool pressed[4];

// shift register outputs assignments.
enum srOutputs {
  ecm_1 = 0,
  ecm_2,
  ecm_3,
  ecm_4,
  k1,
  k2,
  k3,
  k4,
  LED_SETUP_OK, //not available on hardware.
  LED_I2C,
  LED_DFPLAYER,       
  LED_WIFI,            
  LED_PREPARATION_READY,
  LED_QUEST_0,         
  LED_CONSEQUENCE_0,   
  LED_RESTART_PROTOCOL 
};

#define NUM_LASERS 8