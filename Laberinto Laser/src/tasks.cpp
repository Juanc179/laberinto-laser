#include "globals.h"
#include "functions.h"
#include "tasks.h"

// Game states for main task coordination
enum GameState {
    STATE_IDLE,        // Waiting for initial start
    STATE_PREPARATION,
    STATE_QUEST,
    STATE_CONSEQUENCE
};

// Global variables for task coordination
volatile GameState currentGameState = STATE_IDLE;
volatile bool taskCompleted = false;
volatile bool emergencyRestart = false;
TaskHandle_t preparationTaskHandle = NULL;
TaskHandle_t questTaskHandle = NULL;
TaskHandle_t consequenceTaskHandle = NULL;

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
            
            // Check for RF4 emergency kill switch
            if (event.channel == 3 && event.type == LONG_PRESS) {
                Serial.println("EMERGENCY RESTART - RF4 long press detected!");
                Serial.println("Killing all tasks for complete system restart...");
                
                emergencyRestart = true;
                
                // Kill all active tasks
                if (mainTaskHandle != NULL) {
                    vTaskDelete(mainTaskHandle);
                    mainTaskHandle = NULL;
                    Serial.println("Main task killed");
                }
                if (preparationTaskHandle != NULL) {
                    vTaskDelete(preparationTaskHandle);
                    preparationTaskHandle = NULL;
                    Serial.println("Preparation task killed");
                }
                if (questTaskHandle != NULL) {
                    vTaskDelete(questTaskHandle);
                    questTaskHandle = NULL;
                    Serial.println("Quest task killed");
                }
                if (consequenceTaskHandle != NULL) {
                    vTaskDelete(consequenceTaskHandle);
                    consequenceTaskHandle = NULL;
                    Serial.println("Consequence task killed");
                }
                
                // Reset all hardware to safe state
                setRedLighting(false);
                setGreenLighting(false);
                setLasers(false);
                Serial.println("Hardware reset to safe state");
                
                // Reset global variables
                currentGameState = STATE_IDLE;
                systemReady = false;
                gameTimeLimit = 60000; // Default 1 minute
                taskCompleted = false;
                Serial.println("Global variables and queue reset");
                
                // Wait a moment before restarting
                vTaskDelay(500 / portTICK_PERIOD_MS);
                
                // Restart main task
                xTaskCreatePinnedToCore(mainTask, "Main Task", 4096, NULL, 1, &mainTaskHandle, 1);
                Serial.println("Main task restarted - Emergency restart complete!");
                
                continue; // Don't send this message to queue
            }
            
            if (event.type == SHORT_PRESS) {
                pcf.write(event.channel, !pcf.read(event.channel));
            }
            msg.channel = event.channel;
            msg.type = event.type;
            
            // Send message to main task queue (let queue handle overflow)
            xQueueSend(mainTaskQueue, &msg, 0);
        }
    }
}

void mainTask(void *pvParameters) {
    Serial.println("Main task started - Game coordinator");
    
    // Reset emergency flag if it was set
    emergencyRestart = false;
    
    // Safety cleanup: ensure all task handles are NULL if this is a restart
    preparationTaskHandle = NULL;
    questTaskHandle = NULL;
    consequenceTaskHandle = NULL;
    
    // Initialize all systems to off state
    setRedLighting(false);
    setGreenLighting(false);
    setLasers(false);
    systemReady = false;
    gameTimeLimit = 60000; // Default 1 minute
    
    // Ensure we start in IDLE state
    currentGameState = STATE_IDLE;
    
    Serial.println("System fully reset and ready");
    
    MainTaskMsg msg;
    
    while (1) {
        switch (currentGameState) {
            case STATE_IDLE:
                Serial.println("System ready. Press RF1 (short press) to start preparation...");
                
                // Wait for RF1 short press to start preparation
                while (currentGameState == STATE_IDLE) {
                    if (xQueueReceive(mainTaskQueue, &msg, portMAX_DELAY) == pdTRUE) {
                        if (msg.channel == 0 && msg.type == SHORT_PRESS) {
                            Serial.println("Starting preparation phase...");
                            currentGameState = STATE_PREPARATION;
                            break;
                        }
                    }
                }
                break;
                
            case STATE_PREPARATION:
                // Create preparation task
                xTaskCreatePinnedToCore(
                    preparationTask,
                    "PreparationTask",
                    4096,
                    NULL,
                    1,
                    &preparationTaskHandle,
                    1
                );
                
                // Wait for preparation to complete
                while (currentGameState == STATE_PREPARATION) {
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }
                Serial.println("Preparation phase completed");
                break;
                
            case STATE_QUEST:
                Serial.println("Starting quest phase...");
                // Create quest task
                xTaskCreatePinnedToCore(
                    questTask,
                    "QuestTask",
                    4096,
                    NULL,
                    1,
                    &questTaskHandle,
                    1
                );
                
                // Wait for quest to complete
                while (currentGameState == STATE_QUEST) {
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }
                Serial.println("Quest phase completed");
                break;
                
            case STATE_CONSEQUENCE:
                Serial.println("Starting consequence phase...");
                // Create consequence task
                xTaskCreatePinnedToCore(
                    consequenceTask,
                    "ConsequenceTask",
                    4096,
                    NULL,
                    1,
                    &consequenceTaskHandle,
                    1
                );
                
                // Wait for consequence to complete
                while (currentGameState == STATE_CONSEQUENCE) {
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }
                Serial.println("Consequence phase completed");
                break;
        }
        
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

void preparationTask(void *pvParameters) {
    Serial.println("Preparation task started");
    
    // Set all PCF8574 pins to input mode
    for (uint8_t i = 0; i < NUM_LASERS; i++) {
        pcf.write(i, HIGH);
    }
    pinMode(LED_BUILTIN, OUTPUT);

    // Turn on all lights and lasers immediately when prep starts
    setRedLighting(true);
    setGreenLighting(true);
    setLasers(true);
    Serial.println("All lights and lasers ON - Preparation started!");

    MainTaskMsg msg;
    
    // --- Time Selection Phase ---
    Serial.println("Select time mode:");
    Serial.println("RF1 (long press) = 30 seconds");
    Serial.println("RF2 (long press) = 1 minute");
    Serial.println("RF3 (long press) = 1.5 minutes");
    
    unsigned long selectedTimeLimit = 70000; // Default 70 seconds
    int blinkCount = 2; // Default for 70 seconds
    bool modeSelected = false;
    
    while (!modeSelected) {
        if (xQueueReceive(mainTaskQueue, &msg, portMAX_DELAY) == pdTRUE) {
            if (msg.type == LONG_PRESS) {
                switch (msg.channel) {
                    case 0: // RF1 - 40 seconds
                        selectedTimeLimit = 40000;
                        blinkCount = 1;
                        Serial.println("Mode selected: 40 seconds");
                        modeSelected = true;
                        break;
                    case 1: // RF2 - 70 seconds
                        selectedTimeLimit = 70000;
                        blinkCount = 2;
                        Serial.println("Mode selected: 70 seconds");
                        modeSelected = true;
                        break;
                    case 2: // RF3 - 90 seconds
                        selectedTimeLimit = 90000;
                        blinkCount = 3;
                        Serial.println("Mode selected: 90 seconds");
                        modeSelected = true;
                        break;
                    // RF4 is reserved for emergency restart, ignore other channels
                }
            }
        }
    }
    flushMainTaskQueue();
    // Confirmation blinks - according to selected time
    Serial.printf("Confirming selection with %d blinks...\n", blinkCount);
    for (int i = 0; i < blinkCount; i++) {
        setRedLighting(false);
        setGreenLighting(false);
        vTaskDelay(300 / portTICK_PERIOD_MS);
        setRedLighting(true);
        setGreenLighting(true);
        vTaskDelay(300 / portTICK_PERIOD_MS);
    }
    
    // Turn off all lights and lasers after confirmation
    setRedLighting(false);
    setGreenLighting(false);
    setLasers(false);
    
    Serial.println("Time mode confirmed! All lights and lasers OFF.");
    Serial.println("Preparation complete! Waiting for RF1 long press to start quest...");
    
    // Set global variables for main task
    gameTimeLimit = selectedTimeLimit;
    systemReady = true;
    
    // Wait for RF1 long press to transition to quest
    while (1) {
        if (xQueueReceive(mainTaskQueue, &msg, portMAX_DELAY) == pdTRUE) {
            if (msg.channel == 0 && msg.type == LONG_PRESS) {
                Serial.println("RF1 long press detected - Moving to quest phase!");
                currentGameState = STATE_QUEST;
                break;
            }
        }
    }
    
    // Task completes here - preparation is done
    vTaskDelete(NULL);
}

void questTask(void *pvParameters) {
    Serial.println("Quest task started - Game phase");
    
    // --- Instructions Phase at start of quest ---
    Serial.println("Playing instructions automatically...");
    playAudioInterrupt(1); // Play instructions immediately
    
    Serial.println("Instructions playing. Press RF1 (short press) to replay instructions, RF1 (long press) to start game...");
    
    MainTaskMsg msg;
    // Instructions loop
    while (1) {
        if (xQueueReceive(mainTaskQueue, &msg, portMAX_DELAY) == pdTRUE) {
            if (msg.channel == 0 && msg.type == SHORT_PRESS) {
                Serial.println("Replaying instructions...");
                playAudioInterrupt(1); // Replay instructions
            }
            if (msg.channel == 0 && msg.type == LONG_PRESS) {
                Serial.println("Instructions finished - Starting game!");
                break; // Exit instructions loop and start game
            }
        }
    }
    flushMainTaskQueue();
    // --- Game Phase starts here ---
    
    // Turn on lasers for the first game setup
    setLasers(true);
    Serial.println("Lasers turned ON - Game ready to start");
    
    // Perform laser check after preparation is complete
    bool laserWorking[NUM_LASERS];
    uint8_t pcfState = pcf.read8();
    for (uint8_t i = 0; i < NUM_LASERS; i++) {
        laserWorking[i] = !(pcfState & (1 << i));
    }

    Serial.print("Final laser working state: ");
    for (uint8_t i = 0; i < NUM_LASERS; i++) {
        Serial.print(laserWorking[i] ? "OK " : "BROKEN ");
    }
    Serial.println();

    const int LIVES_PER_PLAYER = 3;
    const unsigned long PLAYER_TIME_LIMIT = gameTimeLimit;

    int playerNumber = 1;
    flushMainTaskQueue();
    while (1) { // Infinite player loop
        // Reset lighting and lasers for new player
        setLasers(true);
        playAudioInterrupt(11);
        // For first player, wait for RF1 to start. For subsequent players, start automatically
        if (playerNumber == 1) {
            flushMainTaskQueue();
            Serial.printf("Waiting for player %d to start (short press RF1)...\n", playerNumber);
            while (1) {
                if (xQueueReceive(mainTaskQueue, &msg, portMAX_DELAY) == pdTRUE) {
                    if (msg.channel == 0 && msg.type == SHORT_PRESS) {
                        break;
                    } else if (msg.channel == 2 && msg.type == LONG_PRESS) {
                        // RF3 can end game even while waiting for player to start
                        Serial.println("RF3 long press detected - Ending game!");
                        currentGameState = STATE_CONSEQUENCE;
                        vTaskDelete(NULL); // Exit quest task
                        return;
                    }
                }
            }
        } else {
            // For subsequent players, they start automatically after decision
            Serial.printf("Player %d starting automatically...\n", playerNumber);
        }
        flushMainTaskQueue();
        setRedLighting(false);
        setGreenLighting(false);
        int lives = LIVES_PER_PLAYER;
        unsigned long startTime = millis();
        bool playerWon = false;
        bool gameEnded = false; // Track if game was ended early with RF3

        // Play countdown audio for player start (audio 7: start turn)
        Serial.printf("Player %d get ready! Playing countdown...\n", playerNumber);
        playAudioInterrupt(7); // Audio 07 - start turn
        
        // Wait 6 seconds for the countdown audio to complete
        vTaskDelay(6000 / portTICK_PERIOD_MS);
        playAudioInterrupt(13); // Audio 13 - all for now
        Serial.printf("Player %d started!\n", playerNumber);
        while (lives > 0 && (millis() - startTime) < PLAYER_TIME_LIMIT && !playerWon && !gameEnded) {
            // Check for laser interruption
            pcfState = pcf.read8();
            bool anyInterrupted = false;
            for (uint8_t i = 0; i < NUM_LASERS; i++) {
                if (laserWorking[i] && (pcfState & (1 << i))) {
                    anyInterrupted = true;
                    break;
                }
            }

            // Check for RF2 events (lose life or win) and RF3 events (end game)
            bool rf2Event = false;
            // Note: gameEnded is already declared in outer scope - don't redeclare it
            // Note: Don't override anyInterrupted here - it was calculated above
            MainTaskMsg rfMsg;
            // Process RF button events one at a time to avoid accumulation
            if (uxQueueMessagesWaiting(mainTaskQueue) > 0) {
                if (xQueueReceive(mainTaskQueue, &rfMsg, 0) == pdTRUE) {
                    if (rfMsg.channel == 1) { // RF2
                        if (rfMsg.type == SHORT_PRESS) {
                            // Lose a life by RF2 short press
                            rf2Event = true;
                        } else if (rfMsg.type == LONG_PRESS) {
                            // Win by RF2 long press
                            playerWon = true;
                            Serial.printf("Player %d wins!\n", playerNumber);
                            playAudioInterrupt(5); // Audio 05 - won
                        }
                    } else if (rfMsg.channel == 2) { // RF3 - End game
                        if (rfMsg.type == LONG_PRESS) {
                            Serial.println("RF3 long press detected - Ending game!");
                            gameEnded = true;
                        }
                    }
                    // Note: Process only one event per game loop iteration to prevent accumulation
                }
            }
            if (playerWon) break;
            if (gameEnded) break; // Exit if RF3 end game was pressed

            // Lose a life by laser interruption or RF2 short press
            if (anyInterrupted || rf2Event) {
                lives--;
                Serial.printf("Player %d lost a life! Lives left: %d\n", playerNumber, lives);

                blinkLasers(3); // Blink lasers 3 times

                if (lives == 2){
                    playAudioInterrupt(2);
                    vTaskDelay(7000 / portTICK_PERIOD_MS);
                    playAudioInterrupt(12);
                } else if (lives == 1) {
                    playAudioInterrupt(3);
                    vTaskDelay(7000 / portTICK_PERIOD_MS);
                    playAudioInterrupt(10);
                } else {
                    playAudioInterrupt(4);
                    vTaskDelay(5000 / portTICK_PERIOD_MS);
                }
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
                // Don't play timeout audio here - handle it in results section
                break;
            }

            vTaskDelay(50 / portTICK_PERIOD_MS);
        }

        // After game ends, turn off lasers
        setLasers(false);
        
        // Store game result and handle audio/lighting
        if (gameEnded) {
            // Game ended by RF3 - go directly to consequence phase
            Serial.printf("Player %d ended the game early with RF3.\n", playerNumber);
            setRedLighting(false);
            setGreenLighting(false);
            
            // Move directly to consequence phase (no audio here)
            Serial.println("Moving to consequence phase...");
            currentGameState = STATE_CONSEQUENCE;
            vTaskDelete(NULL); // Exit quest task
            return;
            
        } else if (playerWon) {
            setGreenLighting(true);
            setRedLighting(false);
            
            // Wait for win audio to complete (audio 5 = 12 seconds) plus buffer
            vTaskDelay(13000 / portTICK_PERIOD_MS); // Wait 13 seconds for audio to complete
            Serial.println("Playing next player preparation audio...");
            playAudioInterrupt(8); // Audio 08 - after turn
            
        } else if (lives == 0) {
            // Player lost all lives - red lighting already set during life loss
            setRedLighting(true);
            setGreenLighting(false);
            
            // Wait for life loss audio to complete (audio 4 = 9 seconds) plus buffer
            vTaskDelay(10000 / portTICK_PERIOD_MS); // Wait 10 seconds for audio to complete
            Serial.println("Playing next player preparation audio...");
            playAudioInterrupt(8); // Audio 08 - after turn
            
        } else if ((millis() - startTime) >= PLAYER_TIME_LIMIT) {
            // Timeout case - red lighting and timeout audio
            setRedLighting(true);
            setGreenLighting(false);
            
            Serial.println("Playing timeout audio...");
            playAudioInterrupt(6); // Audio 06 - timeout
            
            // Wait for timeout audio to complete (audio 6 = 11 seconds) plus buffer
            vTaskDelay(12000 / portTICK_PERIOD_MS); // Wait 12 seconds for audio to complete
            Serial.println("Playing next player preparation audio...");
            playAudioInterrupt(8); // Audio 08 - after turn
        }

        Serial.printf("Player %d's turn is over.\n", playerNumber);
        
        // Wait for after-turn audio to complete (audio 8 = 12 seconds) plus buffer
        vTaskDelay(15000 / portTICK_PERIOD_MS);
        
        // Automatic labyrinth restart - turn off all lights after restart audio
        setRedLighting(false);
        setGreenLighting(false);
        Serial.println("Labyrinth restarted automatically - Lights turned OFF");
        
        // Turn on lasers for next player
        setLasers(true);
        Serial.println("Lasers turned ON - Ready for next player");
        
        // Check if user wants to end the game or continue with next player
        Serial.println("Options:");
        Serial.println("RF1 (short press) - Next player");
        Serial.println("RF3 (long press) - End game and go to consequence phase");
        myDFPlayer.play(12);
        bool nextPlayerDecided = false;
        
        while (!nextPlayerDecided) {
            if (xQueueReceive(mainTaskQueue, &msg, portMAX_DELAY) == pdTRUE) {
                if (msg.channel == 0 && msg.type == SHORT_PRESS) {
                    // Continue with next player - automatically start their turn
                    playerNumber++;
                    Serial.printf("Starting player %d automatically...\n", playerNumber);
                    nextPlayerDecided = true;
                    // No need to wait for another RF1 press - go directly to game sequence
                } else if (msg.channel == 2 && msg.type == LONG_PRESS) {
                    // End game and go to consequence
                    Serial.println("Ending game session - Moving to consequence phase...");
                    currentGameState = STATE_CONSEQUENCE;
                    vTaskDelete(NULL); // Exit quest task
                    return;
                }
            }
        }
        
        // Continue the loop for next player (don't move to consequence yet)
    }
}

void consequenceTask(void *pvParameters) {
    Serial.println("Consequence task started - Game ending phase");
    
    // Turn off lasers immediately
    setLasers(false);
    Serial.println("Lasers turned OFF");
    
    // Turn on both lights (red and green)
    setRedLighting(true);
    setGreenLighting(true);
    Serial.println("Both red and green lights turned ON");
    
    // Force stop any ongoing audio by playing a working track first, then play goodbye
    Serial.println("Ensuring audio is ready...");
    playAudioInterrupt(9); // Track 9 - goodbye audio (confirmed working)
    Serial.println("Playing goodbye audio (track 9)...");
    
    unsigned long lastAudioTime = millis();
    const unsigned long AUDIO_REPEAT_INTERVAL = 20000; // 20 seconds
    
    Serial.println("Game ended. Press RF1 (long press) to restart preparation phase...");
    
    MainTaskMsg msg;
    while (1) {
        // Check for RF1 long press to restart preparation
        if (xQueueReceive(mainTaskQueue, &msg, 100 / portTICK_PERIOD_MS) == pdTRUE) {
            if (msg.channel == 0 && msg.type == LONG_PRESS) {
                // RF1 long press - restart entire game (back to preparation)
                Serial.println("RF1 long press detected - Restarting preparation phase...");
                currentGameState = STATE_PREPARATION;
                break;
            }
        }
        /*
        // Check if 20 seconds have passed since last audio play
        if ((millis() - lastAudioTime) >= AUDIO_REPEAT_INTERVAL) {
            Serial.println("Replaying ending audio (track 09)...");
            playAudioInterrupt(9); // Audio 09 - goodbye (confirmed working)
            lastAudioTime = millis();
        }
        */
        // Small delay to prevent busy waiting
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
    
    Serial.println("Consequence phase completed - Restarting game");
    
    // Task completes here - consequence is done
    vTaskDelete(NULL);
}