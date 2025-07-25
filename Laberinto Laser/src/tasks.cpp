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
TaskHandle_t mainTaskHandle = NULL;
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
                
                // Clear any pending messages in the queue
                MainTaskMsg dummyMsg;
                while (uxQueueMessagesWaiting(mainTaskQueue) > 0) {
                    xQueueReceive(mainTaskQueue, &dummyMsg, 0);
                }
                
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
    
    unsigned long selectedTimeLimit = 60000; // Default 1 minute
    int blinkCount = 2; // Default for 1 minute
    bool modeSelected = false;
    
    while (!modeSelected) {
        if (xQueueReceive(mainTaskQueue, &msg, portMAX_DELAY) == pdTRUE) {
            if (msg.type == LONG_PRESS) {
                switch (msg.channel) {
                    case 0: // RF1 - 30 seconds
                        selectedTimeLimit = 30000;
                        blinkCount = 1;
                        Serial.println("Mode selected: 30 seconds");
                        modeSelected = true;
                        break;
                    case 1: // RF2 - 1 minute
                        selectedTimeLimit = 60000;
                        blinkCount = 2;
                        Serial.println("Mode selected: 1 minute");
                        modeSelected = true;
                        break;
                    case 2: // RF3 - 1.5 minutes
                        selectedTimeLimit = 90000;
                        blinkCount = 3;
                        Serial.println("Mode selected: 1.5 minutes");
                        modeSelected = true;
                        break;
                    // RF4 is reserved for emergency restart, ignore other channels
                }
            }
        }
    }
    
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
    
    // --- Game Phase starts here ---
    
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
    
    while (1) { // Infinite player loop
        // Reset lighting and lasers for new player
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
        
        setRedLighting(false);
        setGreenLighting(false);
        int lives = LIVES_PER_PLAYER;
        unsigned long startTime = millis();
        bool playerWon = false;
        bool gameEnded = false; // Track if game was ended early with RF3

        // Play countdown audio for player start (audio 6: "el laberinto esta listo...")
        Serial.printf("Player %d get ready! Playing countdown...\n", playerNumber);
        playAudioInterrupt(6); // Audio 7 - 1 = 6
        
        // Wait 8 seconds for the countdown audio to complete
        vTaskDelay(8000 / portTICK_PERIOD_MS);
        
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
            bool gameEnded = false;
            MainTaskMsg rfMsg;
            while (uxQueueMessagesWaiting(mainTaskQueue) > 0) {
                if (xQueueReceive(mainTaskQueue, &rfMsg, 0) == pdTRUE) {
                    if (rfMsg.channel == 1) { // RF2
                        if (rfMsg.type == SHORT_PRESS) {
                            // Lose a life by RF2 short press
                            rf2Event = true;
                            break;
                        } else if (rfMsg.type == LONG_PRESS) {
                            // Win by RF2 long press
                            playerWon = true;
                            Serial.printf("Player %d wins!\n", playerNumber);
                            playAudioInterrupt(6); // Game ended, player won
                            break;
                        }
                    } else if (rfMsg.channel == 2) { // RF3 - End game
                        if (rfMsg.type == LONG_PRESS) {
                            Serial.println("RF3 long press detected - Ending game!");
                            gameEnded = true;
                            break;
                        }
                    }
                }
            }
            if (playerWon) break;
            if (gameEnded) break; // Exit if RF3 end game was pressed

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
                playAudioInterrupt(8); // Audio 9 - 1 = 8 ("se acabo el tiempo...")
                break;
            }

            vTaskDelay(50 / portTICK_PERIOD_MS);
        }

        // After game ends, turn off lasers
        setLasers(false);

        // Store game result for consequence task
        if (gameEnded) {
            // Game ended by RF3 - neutral state
            Serial.printf("Player %d ended the game early with RF3.\n", playerNumber);
            setRedLighting(false);
            setGreenLighting(false);
        } else if (playerWon) {
            setGreenLighting(true);
            setRedLighting(false);
        } else if (lives == 0) {
            // Player lost all lives - red lighting already set during life loss
            setRedLighting(true);
            setGreenLighting(false);
            
            // Play next player audio after life loss (audio 7: "el laberinto se reiniciara...")
            vTaskDelay(2000 / portTICK_PERIOD_MS); // Give time for lighting effects
            Serial.println("Playing next player preparation audio...");
            playAudioInterrupt(7); // Audio 8 - 1 = 7
            
        } else {
            // Timeout case - red lighting
            setRedLighting(true);
            setGreenLighting(false);
            
            // Play next player audio after timeout (audio 7: "el laberinto se reiniciara...")
            vTaskDelay(2000 / portTICK_PERIOD_MS); // Give time for lighting effects  
            Serial.println("Playing next player preparation audio...");
            playAudioInterrupt(7); // Audio 8 - 1 = 7
        }

        Serial.printf("Player %d's turn is over. Moving to consequence phase.\n", playerNumber);
        
        // Move to consequence phase
        currentGameState = STATE_CONSEQUENCE;
        
        // Task completes here - quest is done
        vTaskDelete(NULL);
    }
}

void consequenceTask(void *pvParameters) {
    Serial.println("Consequence task started - Game ending phase");
    
    // Turn off lasers immediately
    setLasers(false);
    Serial.println("Lasers turned OFF");
    
    // Turn on both lights
    setRedLighting(true);
    setGreenLighting(true);
    Serial.println("Both red and green lights turned ON");
    
    // Display results for a while
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    
    // Play ending audio (audio 9)
    Serial.println("Playing ending audio (track 9)...");
    playAudioInterrupt(9); // Audio 10 - 1 = 9 ("el juego ha terminado...")
    
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
        
        // Check if 20 seconds have passed since last audio play
        if ((millis() - lastAudioTime) >= AUDIO_REPEAT_INTERVAL) {
            Serial.println("Replaying ending audio (track 9)...");
            playAudioInterrupt(9); // Audio 10 - 1 = 9 ("el juego ha terminado...")
            lastAudioTime = millis();
        }
        
        // Small delay to prevent busy waiting
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
    
    Serial.println("Consequence phase completed - Restarting game");
    
    // Task completes here - consequence is done
    vTaskDelete(NULL);
}