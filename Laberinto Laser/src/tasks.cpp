#include "globals.h"
#include "functions.h"
#include "tasks.h"
/*
A FreeRTOS task runs the code inside its function. When the function returns (reaches the end or executes a return), the task is deleted automatically and its resources are freed.

Best practice:
If you want a task to run forever, use a while(1) loop inside it.
If you want it to run once and finish, just let the function end.

After the task ends:

The task is removed from the scheduler.
If you want to run it again, you must create it again with xTaskCreatePinnedToCore().
Summary:

Task ends → it is deleted.
To run again, create it again.
*/
void rfControllerTask(void *pvParameters) {
  RfEvent event;
  MainTaskMsg msg;
  while (1) {
    if (xQueueReceive(rfEventQueue, &event, portMAX_DELAY) == pdTRUE) {
      if (event.type == LONG_PRESS) {
        Serial.printf("Long press detected on channel %d\n", event.channel + 1);
      } else {
        Serial.printf("Short press detected on channel %d\n", event.channel + 1);
      }
      if (event.type == SHORT_PRESS) {
        pcf.write(event.channel, !pcf.read(event.channel));
      }
      msg.channel = event.channel;
      msg.type = event.type;
      xQueueSend(mainTaskQueue, &msg, 0);
    }
  }
}

void mainTask(void *pvParameters) {
  MainTaskMsg msg;

    for (uint8_t i = 0; i < NUM_LASERS; i++) {
        pcf.write(i, HIGH);
    }

    pinMode(LED_BUILTIN, OUTPUT);

    bool laserWorking[NUM_LASERS];   // true = working, false = broken
    bool laserState[NUM_LASERS];     // true = interrupted, false = clear

    // Initial scan to detect broken lasers (not interrupted, but reading 1)
    uint8_t pcfState = pcf.read8();
    for (uint8_t i = 0; i < NUM_LASERS; i++) {
        // If input is 1 at idle, consider it broken
        laserWorking[i] = !(pcfState & (1 << i));
    }

    Serial.print("Initial laser working state: ");
    for (uint8_t i = 0; i < NUM_LASERS; i++) {
        Serial.print(laserWorking[i] ? "OK " : "BROKEN ");
    }
    Serial.println();

    while (1) {
        pcfState = pcf.read8();
        bool anyInterrupted = false;

        for (uint8_t i = 0; i < NUM_LASERS; i++) {
            // Only check lasers that are working
            if (laserWorking[i]) {
                laserState[i] = (pcfState & (1 << i)) ? true : false; // true = interrupted
                if (laserState[i]) anyInterrupted = true;
            } else {
                laserState[i] = false; // ignore broken
            }
        }

        digitalWrite(LED_BUILTIN, anyInterrupted ? HIGH : LOW);

        Serial.print("Laser states: ");
        for (uint8_t i = 0; i < NUM_LASERS; i++) {
            if (laserWorking[i]) {
                Serial.print(laserState[i] ? "X " : "O ");
            } else {
                Serial.print("- "); // Broken
            }
        }
        Serial.println();

        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}


void preparationTask(void *pvParameters) {
    while (1) {
        // Preparation logic here
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void questTask(void *pvParameters) {
    while (1) {
        // Quest logic here
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void consequenceTask(void *pvParameters) {
    while (1) {
        // Consequence logic here
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void restartSequenceTask(void *pvParameters) {
    while (1) {
        // Restart sequence logic here
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}


/*
preliminar


void mainTask(void *pvParameters) {
    // ... (initial laser check code as before) ...

    const int NUM_PLAYERS = 5; // Set as needed
    const int LIVES_PER_PLAYER = 3;
    const unsigned long PLAYER_TIME_LIMIT = 60000; // 1 minute in ms

    while (1) { // Main loop for all players
        for (int player = 1; player <= NUM_PLAYERS; player++) {
            int lives = LIVES_PER_PLAYER;
            unsigned long startTime = millis();
            bool playerWon = false;

            Serial.printf("Player %d, get ready!\n", player);
            myDFPlayer.play(5); // Play "start" audio (choose your track)
            vTaskDelay(2000 / portTICK_PERIOD_MS);

            while (lives > 0 && (millis() - startTime) < PLAYER_TIME_LIMIT && !playerWon) {
                uint8_t pcfState = pcf.read8();
                bool anyInterrupted = false;

                for (uint8_t i = 0; i < NUM_LASERS; i++) {
                    if (laserWorking[i] && (pcfState & (1 << i))) {
                        anyInterrupted = true;
                        break;
                    }
                }

                if (anyInterrupted) {
                    lives--;
                    Serial.printf("Player %d lost a life! Lives left: %d\n", player, lives);
                    myDFPlayer.play(2); // Play "life lost" audio
                    vTaskDelay(1500 / portTICK_PERIOD_MS); // Wait for audio
                    // Wait until all lasers are clear before continuing
                    while (true) {
                        pcfState = pcf.read8();
                        bool stillInterrupted = false;
                        for (uint8_t i = 0; i < NUM_LASERS; i++) {
                            if (laserWorking[i] && (pcfState & (1 << i))) {
                                stillInterrupted = true;
                                break;
                            }
                        }
                        if (!stillInterrupted) break;
                        vTaskDelay(100 / portTICK_PERIOD_MS);
                    }
                }

                // Check for win (long press on RF channel 2)
                if (pressed[1] && (millis() - pressStart[1]) > LONG_PRESS_MS) {
                    playerWon = true;
                    Serial.printf("Player %d wins!\n", player);
                    myDFPlayer.play(3); // Play "win" audio
                    vTaskDelay(2000 / portTICK_PERIOD_MS);
                }

                // Time check
                if ((millis() - startTime) >= PLAYER_TIME_LIMIT) {
                    Serial.printf("Player %d ran out of time!\n", player);
                    myDFPlayer.play(4); // Play "timeout" audio
                    vTaskDelay(2000 / portTICK_PERIOD_MS);
                    break;
                }

                vTaskDelay(50 / portTICK_PERIOD_MS);
            }

            if (!playerWon && lives == 0) {
                Serial.printf("Player %d lost all lives!\n", player);
                myDFPlayer.play(6); // Play "game over" audio
                vTaskDelay(2000 / portTICK_PERIOD_MS);
            }

            Serial.printf("Player %d's turn is over.\n", player);
            vTaskDelay(3000 / portTICK_PERIOD_MS); // Wait before next player
        }
    }
}
    
*/