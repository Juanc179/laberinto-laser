#include <Arduino.h>
#include "globals.h"
#include "functions.h"
#include <ShiftRegister74HC595.h>
ShiftRegister74HC595<2> sr(5, 19, 18); // Use your actual pins here
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Relay test start!");

    // Initialize shift register if needed (depends on your codebase)
    // sr.begin(); // Uncomment if you have an init function

    // Test sequence: turn each relay on for 1s, then off
    setRedLighting(true);
    Serial.println("Red lighting ON");
    delay(1000);
    setRedLighting(false);
    Serial.println("Red lighting OFF");
    delay(500);

    setGreenLighting(true);
    Serial.println("Green lighting ON");
    delay(1000);
    setGreenLighting(false);
    Serial.println("Green lighting OFF");
    delay(500);

    setLasers(true);
    Serial.println("Lasers ON");
    delay(1000);
    setLasers(false);
    Serial.println("Lasers OFF");
    delay(500);

    // Blink lasers 3 times
    Serial.println("Blinking lasers 3 times...");
    blinkLasers(3, 300);

    Serial.println("Relay test complete!");
}

void loop() {
    // Nothing to do in loop
}