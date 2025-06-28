#include "globals.h"
#include "isr.h"
#include "functions.h"

esp_err_t gpio_declarations(void) {
  for (int i = 0; i < 4; i++) {
    pinMode(rfPins[i], INPUT);
  }
  attachInterrupt(digitalPinToInterrupt(rfPins[0]), rf_isr0, CHANGE);
  attachInterrupt(digitalPinToInterrupt(rfPins[1]), rf_isr1, CHANGE);
  attachInterrupt(digitalPinToInterrupt(rfPins[2]), rf_isr2, CHANGE);
  attachInterrupt(digitalPinToInterrupt(rfPins[3]), rf_isr3, CHANGE);
  return ESP_OK;
}