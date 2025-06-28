#include "globals.h"
#include "isr.h"

void IRAM_ATTR rf_isr0() { handle_rf_isr(0); }
void IRAM_ATTR rf_isr1() { handle_rf_isr(1); }
void IRAM_ATTR rf_isr2() { handle_rf_isr(2); }
void IRAM_ATTR rf_isr3() { handle_rf_isr(3); }

void handle_rf_isr(int idx) {
  bool state = digitalRead(rfPins[idx]);
  unsigned long now = millis();
  static BaseType_t xHigherPriorityTaskWoken;
  if (state && !pressed[idx]) {
    pressed[idx] = true;
    pressStart[idx] = now;
  } else if (!state && pressed[idx]) {
    pressed[idx] = false;
    unsigned long duration = now - pressStart[idx];
    RfEvent event;
    event.channel = idx;
    event.type = (duration >= LONG_PRESS_MS) ? LONG_PRESS : SHORT_PRESS;
    xQueueSendFromISR(rfEventQueue, &event, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) portYIELD_FROM_ISR();
  }
}