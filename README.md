# Yahtzee Retro Deluxe

A retro-style terminal Yahtzee game written entirely in **C** using the **ncurses** library.
This project brings the classic Yahtzee experience to the command line with animated dice rolls, colored UI panels, score previews, pause menus, event logs, and a computer opponent with smarter decision-making.

## Features

- Built using **pure C**
- Terminal-based interface powered by **ncurses**
- Retro-style animated gameplay
- Full **Yahtzee scoring system**
- Interactive dice holding and rerolling
- Category score preview before selecting
- Keyboard shortcuts for fast scoring
- Pause menu with resume, restart, and quit options
- Event log for turn-by-turn updates
- Smarter computer AI with strategy-based rerolls
- Colored terminal UI (when supported)

## Gameplay

The game is played over **13 rounds**.
In each round:

1. Roll 5 dice
2. Choose which dice to hold
3. Reroll the remaining dice up to 2 times
4. Select a scoring category

At the end of 13 rounds, the player with the highest total score wins.

## Scoring Categories

The game includes all standard Yahtzee categories:

- Ones
- Twos
- Threes
- Fours
- Fives
- Sixes
- Three of a Kind
- Four of a Kind
- Full House
- Small Straight
- Large Straight
- Yahtzee
- Chance

## Controls

### In-game controls

- `LEFT / RIGHT` — Move between dice
- `SPACE` — Hold or unhold selected die
- `R` — Reroll unheld dice
- `C` — Open category selection menu
- `P` — Open pause menu

### Quick score keys

You can directly choose categories using shortcut keys:

- `1` → Ones
- `2` → Twos
- `3` → Threes
- `4` → Fours
- `5` → Fives
- `6` → Sixes
- `7` → Three of a Kind
- `8` → Four of a Kind
- `9` → Full House
- `0` → Small Straight
- `-` → Large Straight
- `=` → Yahtzee
- `]` → Chance

## Requirements

- C compiler such as **gcc**
- **ncurses** library installed
- Terminal size of at least **110x30**

## Compilation

Compile the program with:

```bash
gcc yahtzee.c -o yahtzee -lncurses
Running the Game

After compiling, run:

./yahtzee
Example

When the game starts, you will see a retro-style menu with options to:

Start Game
View How to Play
Quit

During gameplay, the interface includes:

Dice display panel
Category preview panel
Scoreboard
Event log
Control/help section
Project Structure

This project is currently implemented in a single C source file and includes:

Game state management
Dice rolling and holding system
Score calculation functions
Computer AI logic
Terminal UI rendering
Menus, dialogs, and animations
AI Behavior

The computer opponent does more than choose random rerolls.
It uses strategy-based planning to aim for categories such as:

Upper section scores
Three/Four of a Kind
Full House
Small Straight
Large Straight
Yahtzee
Chance

This makes the gameplay more dynamic and competitive.

Notes
The game uses ncurses, so it should be run in a compatible terminal.
Colored output depends on terminal color support.
If your terminal window is too small, the game will ask you to resize it before running.
Future Improvements

Possible enhancements for this project:

Upper section bonus support
Yahtzee bonus rules
Save/load game feature
Two-player mode
Sound improvements
Better end-game statistics
Cross-platform build instructions
Learning Goals

This project is a good example of practicing:

Procedural programming in C
Terminal UI development with ncurses
Array and function pointer usage
Game loop design
Input handling
Basic AI decision-making
License

This project is open-source and available under the MIT License.

Author

Developed as a retro Yahtzee game project in C.


