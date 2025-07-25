# Laberinto Laser - Updated Task Architecture

## Overview
The project has been simplified to a more streamlined design with emergency restart functionality and optimized button usage.

## Architecture Components

### 1. Main Task (Coordinator)
- **Role**: Central coordinator that manages the game flow
- **Function**: `mainTask()`
- **Responsibility**: 
  - Manages state transitions between different game phases
  - Creates and destroys phase-specific tasks as needed
  - Monitors task completion and coordinates the overall game flow
  - Handles system initialization and reset

### 2. Game States
The game operates in four distinct states:
- `STATE_IDLE`: Waiting for initial RF1 short press to start
- `STATE_PREPARATION`: Time selection and system configuration
- `STATE_QUEST`: Active gameplay phase
- `STATE_CONSEQUENCE`: Post-game results and player decisions

### 3. Emergency Restart System
- **RF4 (Red Button)**: Emergency kill switch
- **Function**: Long press on RF4 at any time during operation
- **Complete System Reset**:
  1. Kills ALL active tasks (main, preparation, quest, consequence)
  2. Resets all hardware to safe state (lights OFF, lasers OFF)
  3. Resets all global variables to default values
  4. Clears message queue of any pending commands
  5. Automatically restarts main task in STATE_IDLE
- **Purpose**: Complete system recovery if any task becomes unresponsive or system enters invalid state
- **Safety**: Ensures no orphaned tasks or inconsistent state remains

### 4. Phase-Specific Tasks

#### Preparation Task
- **Function**: `preparationTask()`
- **Purpose**: Handles system setup and time mode selection
- **Lifecycle**: Created when RF1 short press is detected, deleted when preparation completes
- **Flow**:
  1. Turn ON all lights and lasers immediately
  2. Time selection via RF1-RF3 long presses:
     - RF1: 30 seconds (1 blink confirmation)
     - RF2: 1 minute (2 blinks confirmation)  
     - RF3: 1.5 minutes (3 blinks confirmation)
  3. Confirmation blinks according to selected time
  4. Turn OFF all lights and lasers
  5. Wait for RF1 long press to start quest

#### Quest Task
- **Function**: `questTask()`
- **Purpose**: Manages instructions and actual laser maze gameplay
- **Lifecycle**: Created when RF1 long press is detected after preparation, deleted when game ends
- **Flow**:
  1. **Instructions Phase**:
     - Automatically plays instructions when quest starts
     - RF1 short press: Replay instructions (indefinitely)
     - RF1 long press: Exit instructions and start game
  2. **Game Phase**:
     - Laser system check
     - Wait for player to start (RF1 long press)
     - Game loop with laser monitoring and life management
     - Win/lose detection
     - Transition to consequence phase

#### Consequence Task
- **Function**: `consequenceTask()`
- **Purpose**: Handles post-game results and next action decisions
- **Lifecycle**: Created when quest completes, deleted when decision is made
- **Flow**:
  1. Display game results (lighting for 3 seconds)
  2. Turn off all lighting
  3. Wait for player decision:
     - RF1 long press: Restart entire game (back to preparation)
     - RF2 long press: Continue with next player (back to quest)

### 5. RF Controller Task
- **Function**: `rfControllerTask()`
- **Purpose**: Handles RF remote input processing with emergency functionality
- **Lifecycle**: Runs continuously throughout system operation
- **Special Features**:
  - Monitors RF4 for emergency restart
  - Processes and forwards other RF events to main task queue

## Key Improvements

### 1. Simplified Button Scheme
- **RF1**: Start system (short press), time selection 30s (long press), start quest (long press), replay instructions (short press), start game/player turn (long press), restart game (long press in consequence)
- **RF2**: Time selection 1min (long press), win/lose life actions in game, next player (long press in consequence)
- **RF3**: Time selection 1.5min (long press), game actions
- **RF4**: Emergency restart only (long press) - RED BUTTON

### 2. Streamlined Startup Flow
```
ESP Power On → Setup → Main Task (STATE_IDLE)
     ↓
RF1 Short Press → Preparation Task → Lights/Lasers ON
     ↓
Time Selection (RF1/RF2/RF3 long press) → Confirmation Blinks
     ↓
Lights/Lasers OFF → Wait for RF1 Long Press → Quest Task
     ↓
Instructions Phase → RF1 Short Press (replay) / RF1 Long Press (start game)
     ↓
Game Phase → Player gameplay → Consequence Phase
```

### 3. Visual Feedback System
- **Preparation Start**: All lights and lasers turn ON
- **Time Confirmation**: Blink count matches selected time (1, 2, or 3 blinks)
- **Preparation End**: All lights and lasers turn OFF
- **Game Results**: Green for win, Red for lose/timeout

### 4. Emergency Safety
- **RF4 Emergency**: Immediately kills main task and restarts system
- **Works at any time**: During any phase of operation
- **Complete Reset**: Returns system to initial STATE_IDLE

## Communication Flow

```
RF Input → RF Controller Task → Emergency Check → Main Task Queue
                                      ↓ (if RF4)
                              Kill & Restart Main Task
                                      ↓
Main Task (Coordinator) ← State Change ← Phase Task Completion
     ↓
Create Next Phase Task
```

## Button Reference

### Startup Phase
- **RF1 Short Press**: Start preparation

### Preparation Phase
- **RF1 Long Press**: Select 30 seconds (1 blink)
- **RF2 Long Press**: Select 1 minute (2 blinks)
- **RF3 Long Press**: Select 1.5 minutes (3 blinks)
- **RF1 Long Press** (after selection): Start quest

### Quest Phase
- **RF1 Short Press**: Replay instructions (during instructions phase)
- **RF1 Long Press**: Exit instructions and start game / Start player turn
- **RF2 Short Press**: Lose a life
- **RF2 Long Press**: Win the game
- **Laser Interruption**: Lose a life

### Consequence Phase
- **RF1 Long Press**: Restart entire game
- **RF2 Long Press**: Continue with next player

### Emergency (Any Time)
- **RF4 Long Press**: Emergency restart system

## Global Variables

- `currentGameState`: Current phase of the game (IDLE, PREPARATION, QUEST, CONSEQUENCE)
- `gameTimeLimit`: Selected time limit from preparation phase  
- `systemReady`: Flag indicating system readiness
- `emergencyRestart`: Flag for emergency restart detection
- `mainTaskHandle`: Handle to main task for emergency restart
- Task handles for dynamic task management

This streamlined architecture provides a clean, safe, and efficient structure for the laser maze game system with proper emergency handling.
