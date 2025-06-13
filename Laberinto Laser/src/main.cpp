#include <Arduino.h>
#include <ShiftRegister74HC595.h>
#include <DFRobotDFPlayerMini.h>
// Set LED_BUILTIN if it is not defined by Arduino framework
#ifndef LED_BUILTIN
    #define LED_BUILTIN 2
#endif
#define RF_CH1 23
#define RF_CH2 4
#define RF_CH3 15
#define RF_CH4 25



volatile unsigned long pressStart[4] = {0, 0, 0, 0};
volatile bool pressed[4] = {false, false, false, false};
volatile bool eventPending[4] = {false, false, false, false};
volatile unsigned long pressDuration[4] = {0, 0, 0, 0};

const int rfPins[4] = {RF_CH1, RF_CH2, RF_CH3, RF_CH4};
const unsigned long LONG_PRESS_MS = 800; // adjust as needed

void handle_rf_isr(int idx);
// Interrupt Service Routines for RF channels

void IRAM_ATTR rf_isr0() { handle_rf_isr(0); }
void IRAM_ATTR rf_isr1() { handle_rf_isr(1); }
void IRAM_ATTR rf_isr2() { handle_rf_isr(2); }
void IRAM_ATTR rf_isr3() { handle_rf_isr(3); }

int serialDataPin = 5; // 14 DS
int clockPin = 19; // 11 SHCP
int latchPin = 18; // 12 STCP
const int numberOfShiftRegisters = 2; // number of shift registers attached in series
ShiftRegister74HC595<numberOfShiftRegisters> sr(serialDataPin, clockPin, latchPin);

// sr 1 pin references
int o1 = 0; // salida a 12v numero 1. gnd comun.
int o2 = 1; // salida a 12v numero 2. gnd comun.
int o3 = 2; // salida a 12v numero 3. gnd comun.
int o4 = 3; // salida a 12v numero 4. gnd comun.
int k1 = 4; // luces
int k2 = 5; // sensor de movimiento
int k3 = 6; // electroiman
int k4 = 7; // flash

// sr 2 pin references

int led1 = 8;
int led2 = 9;
int led3 = 10;
int led4 = 11;
int led5 = 12;
int led6 = 13;
int led7 = 14;
int led8 = 15;

HardwareSerial Seral2(1); // Use Serial1 for DFPlayer Mini

DFRobotDFPlayerMini myDFPlayer;

esp_err_t gpio_declarations(void);
int prep_0(void);
int quest_0(void);
int postQuest(void);
int restartProtocol(void);


void setup()
{
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 16, 17); // RX, TX for DFPlayer Mini
  Serial.println("Start");
  gpio_declarations();

  sr.setAllLow();
  Serial.println("DFPlayer Mini test");
  if (!myDFPlayer.begin(Serial2)){
    Serial.println("Unable to begin DFPlayer Mini:");
    Serial.println("1.Please recheck the connection!");
    Serial.println("2.Please insert the SD card!");
    while(true);
  }
  Serial.println("DFPlayer Mini online.");
  myDFPlayer.volume(20);  // Set volume value (0~30).
  myDFPlayer.play(1);     // Play the first mp3
}

void loop(){
  for (int i = 0; i < 4; i++) {
    if (eventPending[i]) {
      eventPending[i] = false;
      if (pressDuration[i] >= LONG_PRESS_MS) {
        Serial.print("Long press detected on channel "); Serial.println(i+1);
        // Call your long press function here
      } else {
        Serial.print("Short press detected on channel "); Serial.println(i+1);
        // Call your short press function here
      }
    }
  }
  delay(100); // Adjust delay as needed
}

void handle_rf_isr(int idx) {
  bool state = digitalRead(rfPins[idx]);
  unsigned long now = millis();
  if (state && !pressed[idx]) { // Button pressed
    pressed[idx] = true;
    pressStart[idx] = now;
  } else if (!state && pressed[idx]) { // Button released
    pressed[idx] = false;
    pressDuration[idx] = now - pressStart[idx];
    eventPending[idx] = true;
  }
}

esp_err_t gpio_declarations(void){
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RF_CH1, INPUT);
  pinMode(RF_CH2, INPUT);
  pinMode(RF_CH3, INPUT);
  pinMode(RF_CH4, INPUT);
  for (int i = 0; i < 4; i++) {
    pinMode(rfPins[i], INPUT);
  }
  attachInterrupt(digitalPinToInterrupt(RF_CH1), rf_isr0, CHANGE);
  attachInterrupt(digitalPinToInterrupt(RF_CH2), rf_isr1, CHANGE);
  attachInterrupt(digitalPinToInterrupt(RF_CH3), rf_isr2, CHANGE);
  attachInterrupt(digitalPinToInterrupt(RF_CH4), rf_isr3, CHANGE);
  return ESP_OK;
}
