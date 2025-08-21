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

void playAudioInterrupt(uint8_t trackIdx) {
    myDFPlayer.stop();
    vTaskDelay(100 / portTICK_PERIOD_MS); // Ensure stop
    myDFPlayer.play(audioTracks[trackIdx].trackNum);
}

void setRedLighting(bool on)   { sr.set(k1, on ? HIGH : LOW); }
void setGreenLighting(bool on) { sr.set(k2, on ? HIGH : LOW); }
void setLasers(bool on)        { sr.set(k3, on ? HIGH : LOW); }

void blinkLasers(int times, int delayMs) {
    for (int i = 0; i < times; ++i) {
        setLasers(false);
        vTaskDelay(delayMs / portTICK_PERIOD_MS);
        setLasers(true);
        vTaskDelay(delayMs / portTICK_PERIOD_MS);
    }
}
/*
void clearMainTaskQueue() {
    MainTaskMsg dummyMsg;
    while (uxQueueMessagesWaiting(mainTaskQueue) > 0) {
        xQueueReceive(mainTaskQueue, &dummyMsg, 0);
    }
} */

void flushMainTaskQueue() {
    MainTaskMsg dummyMsg;
    while (uxQueueMessagesWaiting(mainTaskQueue) > 0) {
        xQueueReceive(mainTaskQueue, &dummyMsg, 0);
    }
}