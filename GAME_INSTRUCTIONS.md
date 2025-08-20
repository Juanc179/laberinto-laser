# Laser Maze Game - Complete User Instructions

## Overview
The Laser Maze Game is an interactive challenge where players must navigate through a maze of laser beams without interrupting them. The game features multiple difficulty levels, audio feedback, visual lighting effects, and supports multiple players.

## Hardware Components
- **Main Unit**: ESP32-based controller with lighting and laser systems
- **RF Remote Control**: 4-button wireless remote (RF1, RF2, RF3, RF4)
- **Laser Grid**: 8 laser beams creating the maze
- **Lighting System**: Red and green LED strips for visual feedback
- **Audio System**: Speaker for instructions and game feedback
- **Emergency Button**: RF4 (Red button) for emergency system restart

---

## Game Phases Overview

The game operates in 4 main phases:
1. **🔴 IDLE**: System startup and initialization
2. **🟡 PREPARATION**: Time selection and system setup
3. **🟢 QUEST**: Instructions and active gameplay
4. **🔵 CONSEQUENCE**: Results display and next actions

---

## Complete Step-by-Step Instructions

### Phase 1: 🔴 System Startup (IDLE)

1. **Power On**
   - Connect power to the main unit
   - Wait for the system to initialize (you'll hear startup sounds)
   - All lights should be OFF initially

2. **Start the Game**
   - Press **RF1 (short press)** to begin preparation
   - The system will move to the PREPARATION phase

---

### Phase 2: 🟡 Preparation Phase (SETUP)

When preparation starts:
- **ALL lights turn ON** (red and green LED strips)
- **ALL lasers turn ON** 
- Audio prompt will guide you through time selection

#### Time Selection
Choose your difficulty level by pressing and holding one of these buttons:

| Button | Duration | Difficulty | Confirmation |
|--------|----------|------------|--------------|
| **RF1 (long press)** | 30 seconds | Hard | 1 blink |
| **RF2 (long press)** | 1 minute | Medium | 2 blinks |
| **RF3 (long press)** | 1.5 minutes | Easy | 3 blinks |

#### After Time Selection:
1. The system will blink the lights according to your selection (1, 2, or 3 times)
2. **ALL lights and lasers turn OFF**
3. Wait for the system prompt
4. Press **RF1 (long press)** to start the QUEST phase

---

### Phase 3: 🟢 Quest Phase (INSTRUCTIONS & GAMEPLAY)

#### Part A: Instructions
1. **Automatic Instructions**: The game automatically plays welcome/instruction audio
2. **Replay Instructions**: Press **RF1 (short press)** to replay instructions anytime
3. **Start Game**: Press **RF1 (long press)** to finish instructions and begin gameplay

#### Part B: Player Gameplay Loop

**For Each Player:**

1. **Wait for Player Ready**
   - System announces "Waiting for player X to start"
   - Press **RF1 (long press)** when the player is ready

2. **Countdown Phase**
   - System plays countdown audio (9 seconds)
   - Player should get into position
   - Lasers are ON during this time

3. **Active Game Phase**
   - Timer starts counting down
   - Player must navigate through the laser maze
   - **Goal**: Reach the end without interrupting any laser beams

#### During Gameplay:

**Ways to Lose a Life:**
- Interrupt any laser beam (automatic detection)
- Press **RF2 (short press)** (manual life loss)

**Ways to Win:**
- Press **RF2 (long press)** when reaching the end successfully

**Ways to End Game Early:**
- Press **RF3 (long press)** to quit the current game

#### Player Status:
- **3 Lives per player**
- Visual feedback when losing lives (laser blinking, audio alerts)
- Different audio for each life lost (1st, 2nd, 3rd life)

#### Game End Conditions:
- ✅ **WIN**: Player reaches end and presses RF2 (long press)
- ❌ **LOSE**: Player loses all 3 lives
- ⏰ **TIMEOUT**: Time runs out
- 🚪 **QUIT**: Player presses RF3 (long press)

---

### Phase 4: 🔵 Consequence Phase (RESULTS)

After each player's turn:

#### Visual Results Display:
- **🟢 Green Lights**: Player won
- **🔴 Red Lights**: Player lost or timed out
- **⚫ No Lights**: Player quit early
- Both lights turn ON for final display (3 seconds)

#### Audio Feedback:
- Appropriate audio plays based on game outcome
- Ending message plays every 20 seconds

#### Next Actions:
Choose what to do next:

| Button | Action | Result |
|--------|--------|---------|
| **RF1 (long press)** | Restart Game | Goes back to PREPARATION phase |
| **Wait** | Continue | System cycles through consequence audio |

**Note**: The game supports unlimited players in sequence. After each player, the system automatically prepares for the next player.

---

## RF Remote Button Reference

### 🔴 RF1 (Primary Control)
- **Short Press**: 
  - IDLE → Start preparation
  - QUEST → Replay instructions
- **Long Press**: 
  - PREPARATION → Select 30 seconds OR start quest (context dependent)
  - QUEST → Exit instructions and start game OR start player turn
  - CONSEQUENCE → Restart entire game

### 🟡 RF2 (Game Actions)
- **Short Press**: 
  - QUEST → Lose a life (during gameplay)
- **Long Press**: 
  - PREPARATION → Select 1 minute
  - QUEST → Win the game (during gameplay)

### 🟢 RF3 (Advanced Options)
- **Long Press**: 
  - PREPARATION → Select 1.5 minutes
  - QUEST → End game early (quit)

### 🔴 RF4 (EMERGENCY - RED BUTTON)
- **Long Press**: **EMERGENCY RESTART**
  - Immediately stops ALL game activity
  - Resets system to IDLE state
  - Turns OFF all lights and lasers
  - Use only if system becomes unresponsive

---

## Audio Feedback System

The game provides rich audio feedback:

| Audio Track | When It Plays | Duration |
|-------------|---------------|----------|
| **Instructions** | Quest start, RF1 short press | 28 seconds |
| **1 Life Lost** | First laser hit | 6 seconds |
| **2 Lives Lost** | Second laser hit | 10 seconds |
| **3 Lives Lost** | Third laser hit (game over) | 9 seconds |
| **Winner** | Player wins | 12 seconds |
| **Timeout** | Time runs out | 11 seconds |
| **Start Turn** | Player countdown | 9 seconds |
| **After Turn** | Between players | 12 seconds |
| **Goodbye** | Game ending | 9 seconds |

---

## Visual Lighting System

### Status Indicators:
- **All Lights OFF**: System idle or game in progress
- **Red + Green ON**: Preparation phase active
- **Lights Blinking**: Time selection confirmation
- **Green ON**: Player won
- **Red ON**: Player lost or timed out
- **Laser Blinking**: Life lost (3 rapid blinks)

---

## Troubleshooting

### Common Issues:

**System Not Responding:**
- Press and hold **RF4 (long press)** for emergency restart
- Check power connections
- Ensure RF remote has batteries

**Audio Not Playing:**
- Check SD card is properly inserted
- Verify audio files are present
- System will show error if DFPlayer Mini is disconnected

**Lasers Not Working:**
- System automatically detects broken laser beams
- Check laser beam alignments
- Ensure receivers are properly positioned

**Remote Not Working:**
- Replace remote batteries
- Check RF signal range
- Verify button presses (short vs long press timing)

---

## Safety Notes

⚠️ **Important Safety Information:**
- Never look directly into laser beams
- Use only the provided RF4 emergency button if needed
- Ensure playing area is clear of obstacles
- Adult supervision recommended for children
- Keep the main unit dry and away from water

---

## Quick Reference Card

| Phase | What You See | What to Press | What Happens |
|-------|--------------|---------------|--------------|
| 🔴 **IDLE** | All lights OFF | RF1 short | → PREPARATION |
| 🟡 **PREPARATION** | All lights ON | RF1/RF2/RF3 long | Select time → RF1 long → QUEST |
| 🟢 **QUEST** | Instructions playing | RF1 short (replay) / RF1 long (start) | → Gameplay |
| 🎮 **GAMEPLAY** | Lasers ON, timer running | RF2 long (win) / avoid lasers | → CONSEQUENCE |
| 🔵 **CONSEQUENCE** | Result lights | RF1 long (restart) | → PREPARATION or continue |
| 🚨 **EMERGENCY** | Any time | RF4 long | → IDLE (full reset) |

---

## Tips for Players

1. **Study the Maze**: Use the preparation time to observe laser beam positions
2. **Move Slowly**: Rushing increases chance of hitting beams
3. **Use Audio Cues**: Listen for life loss warnings
4. **Practice Timing**: Learn the difference between short and long button presses
5. **Stay Calm**: Time pressure can lead to mistakes
6. **Communication**: Call out when you've reached the end safely before pressing RF2

---

Enjoy the Laser Maze Challenge! 🎯✨
