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
    // Set all PCF8574 pins to input mode
    for (uint8_t i = 0; i < NUM_LASERS; i++) {
        pcf.write(i, HIGH);
    }
    pinMode(LED_BUILTIN, OUTPUT);

    // Set relay pins (shift register outputs) to initial state
    setRedLighting(false);
    setGreenLighting(false);
    setLasers(true); // Lasers ON at start

    bool laserWorking[NUM_LASERS];

    // Initial scan to detect broken lasers (not interrupted, but reading 1)
    uint8_t pcfState = pcf.read8();
    for (uint8_t i = 0; i < NUM_LASERS; i++) {
        laserWorking[i] = !(pcfState & (1 << i));
    }

    Serial.print("Initial laser working state: ");
    for (uint8_t i = 0; i < NUM_LASERS; i++) {
        Serial.print(laserWorking[i] ? "OK " : "BROKEN ");
    }
    Serial.println();

    // --- Wait for short/long presses to start game ---
    Serial.println("Press RF1 (short press) to turn on lights, then short press to play instructions, long press to start game...");
    MainTaskMsg msg;
    bool lightsOn = false;
    while (1) {
        if (xQueueReceive(mainTaskQueue, &msg, portMAX_DELAY) == pdTRUE) {
            if (msg.channel == 0 && msg.type == SHORT_PRESS) {
                if (!lightsOn) {
                    setRedLighting(true);
                    setGreenLighting(true);
                    lightsOn = true;
                } else {
                    playAudioInterrupt(1); // Play instructions
                }
            }
            if (msg.channel == 0 && msg.type == LONG_PRESS) {
                break; // Start the game
            }
        }
    }
    // After this, turn off both lights before starting the player loop
    setRedLighting(false);
    setGreenLighting(false);

    const int LIVES_PER_PLAYER = 3;
    const unsigned long PLAYER_TIME_LIMIT = 60000; // 1 minute

    int playerNumber = 1;
    while (1) { // Infinite player loop
        // Reset lighting and lasers for new player
        setRedLighting(false);
        setGreenLighting(false);
        setLasers(true);

        // Wait for long press on RF button 1 to start player
        Serial.printf("Waiting for player %d to start (long press RF1)...\n", playerNumber);
        while (1) {
            if (xQueueReceive(mainTaskQueue, &msg, portMAX_DELAY) == pdTRUE) {
                if (msg.channel == 0 && msg.type == LONG_PRESS) {
                    break;
                }
            }
        }

        int lives = LIVES_PER_PLAYER;
        unsigned long startTime = millis();
        bool playerWon = false;

        Serial.printf("Player %d started!\n", playerNumber);

        while (lives > 0 && (millis() - startTime) < PLAYER_TIME_LIMIT && !playerWon) {
            // Check for laser interruption
            pcfState = pcf.read8();
            bool anyInterrupted = false;
            for (uint8_t i = 0; i < NUM_LASERS; i++) {
                if (laserWorking[i] && (pcfState & (1 << i))) {
                    anyInterrupted = true;
                    break;
                }
            }

            // Check for RF2 events (lose life or win)
            bool rf2Event = false;
            MainTaskMsg rf2msg;
            while (uxQueueMessagesWaiting(mainTaskQueue) > 0) {
                if (xQueueReceive(mainTaskQueue, &rf2msg, 0) == pdTRUE) {
                    if (rf2msg.channel == 1) {
                        if (rf2msg.type == SHORT_PRESS) {
                            // Lose a life by RF2 short press
                            rf2Event = true;
                            break;
                        } else if (rf2msg.type == LONG_PRESS) {
                            // Win by RF2 long press
                            playerWon = true;
                            Serial.printf("Player %d wins!\n", playerNumber);
                            playAudioInterrupt(6); // Game ended, player won
                            break;
                        }
                    }
                }
            }
            if (playerWon) break;

            // Lose a life by laser interruption or RF2 short press
            if (anyInterrupted || rf2Event) {
                lives--;
                Serial.printf("Player %d lost a life! Lives left: %d\n", playerNumber, lives);

                blinkLasers(3); // Blink lasers 3 times

                if (lives == 2) playAudioInterrupt(2); // First laser interrupted
                else if (lives == 1) playAudioInterrupt(3); // Second laser interrupted
                else playAudioInterrupt(4); // Third laser interrupted, game ended by laser

                // Wait for all lasers to clear and RF2 to be released
                while (1) {
                    pcfState = pcf.read8();
                    bool stillInterrupted = false;
                    for (uint8_t i = 0; i < NUM_LASERS; i++) {
                        if (laserWorking[i] && (pcfState & (1 << i))) {
                            stillInterrupted = true;
                            break;
                        }
                    }
                    if (!stillInterrupted) break;
                    vTaskDelay(50 / portTICK_PERIOD_MS);
                }
            }

            // Time check
            if ((millis() - startTime) >= PLAYER_TIME_LIMIT) {
                Serial.printf("Player %d ran out of time!\n", playerNumber);
                playAudioInterrupt(5); // Game ended by timeout
                break;
            }

            vTaskDelay(50 / portTICK_PERIOD_MS);
        }

        // After game ends, turn off lasers
        setLasers(false);

        if (playerWon) {
            setGreenLighting(true);
            setRedLighting(false);
            // Already played win audio
        } else if (lives == 0) {
            setRedLighting(true);
            setGreenLighting(false);
            // Already played "game ended by laser" audio
        } else {
            // Timeout case, already played timeout audio
            setRedLighting(true);
            setGreenLighting(false);
        }

        Serial.printf("Player %d's turn is over.\n", playerNumber);
        playerNumber++;
        vTaskDelay(500 / portTICK_PERIOD_MS); // Short pause before next player
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