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

// Global variables for game state
extern unsigned long gameTimeLimit;
extern bool systemReady;
extern TaskHandle_t mainTaskHandle;

// shift register outputs assignments.
enum srOutputs {
  ecm_1 = 1,
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

struct AudioTrack {
    uint8_t trackNum;
    uint32_t durationMs; // in milliseconds
};

const AudioTrack audioTracks[] = {
    {1, 25000},   // Instructions (track 01.mp3) - 00:25
    {2, 6000},    // 2 lives left
    {3, 9000},    // 1 life left
    {4, 8000},    // 0 lives left
    {5, 11000},   // Mission successful
    {6, 4000},    // Timeout 1
    {7, 6000},    // Timeout 2
    {8, 5000},    // Timeout 3
    {9, 5000},    // Reset audio (track 09.mp3) - 00:05
    {10, 204000}, // All for now (track 10.mp3) - 3:24
    {11, 347000}, // Disturbed lines (track 11.mp3) - 5:47
    {12, 171000}, // Katana Zero OST (track 12.mp3) - 2:51
    {13, 60000},  // Stranger Things song (track 13.mp3) - 1:00
    {14, 190000}  // delusive bunker (track 14.mp3) - 3:10
};