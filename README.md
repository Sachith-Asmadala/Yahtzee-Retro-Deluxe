# Yahtzee Retro Deluxe

A retro-style **Yahtzee terminal game written in C** using the **ncurses** library.  
This project recreates the classic dice game in a colorful command-line interface with animated rolls, score previews, category selection, and a smarter AI opponent.

## Overview

**Yahtzee Retro Deluxe** is a terminal-based implementation of Yahtzee designed to showcase core C programming concepts through an interactive game. The project combines classic gameplay rules with a retro user interface, keyboard-driven controls, and simple AI strategy to create a fun and polished command-line experience.

This project was built to practice:

- procedural programming in C
- terminal UI development with ncurses
- game loop design
- scoring logic and state management
- input handling and menu systems
- basic AI decision-making

## Features

- Full **Yahtzee gameplay** across 13 rounds
- Built entirely in **C**
- **ncurses-based** retro terminal interface
- Animated dice roll effects
- Dice hold/unhold system
- Real-time score preview for available categories
- Full scoreboard for player and computer
- Event log for gameplay updates
- Pause menu with resume, restart, and quit
- Keyboard shortcuts for faster category selection
- Smarter AI opponent with category-based reroll planning
- Color support for compatible terminals

## Gameplay

The game is played over **13 rounds**.  
Each round consists of:

1. Rolling 5 dice
2. Choosing which dice to hold
3. Rerolling the remaining dice up to 2 times
4. Selecting a scoring category

Once a category is used, it cannot be selected again.  
After all 13 categories are filled, the game ends and the final scores are compared.

## Scoring Categories

### Upper Section
- Ones
- Twos
- Threes
- Fours
- Fives
- Sixes

### Lower Section
- Three of a Kind
- Four of a Kind
- Full House
- Small Straight
- Large Straight
- Yahtzee
- Chance

## Controls

### Main Controls
- `LEFT / RIGHT` — move between dice
- `SPACE` — hold or unhold selected die
- `R` — reroll unheld dice
- `C` — open category selection menu
- `P` — open pause menu

### Quick Category Shortcuts
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

To build and run this project, you need:

- a C compiler such as `gcc`
- the `ncurses` library
- a terminal window with a minimum size of **110 x 30**

## Build

Compile the program with:

```bash
gcc yahtzee.c -o yahtzee -lncurses

Run

After compilation, run:
./yahtzee

Project Structure

The current version is implemented as a single C source file and includes:

game state initialization and round handling
scoring functions for all categories
player interaction and reroll logic
AI reroll planning and category selection
ncurses-based window rendering
popups, menus, dialogs, and animations
AI Opponent

The computer player uses simple decision logic instead of random choices.
It evaluates the current dice and attempts to aim for useful scoring categories such as:

upper section matches
three of a kind
four of a kind
full house
small straight
large straight
yahtzee
chance

This makes gameplay more interesting and gives the project a stronger game-logic component.

Interface Panels

The terminal UI is divided into multiple sections:

Header — round number and total scores
Dice Window — current dice values and held dice
Preview Panel — possible category scores for the current roll
Scoreboard — player and computer score tracking
Event Log — recent gameplay actions
Help Panel — controls and status messages
Why This Project

This project is a good demonstration of how traditional C programming can be used to create a complete interactive application without relying on modern game engines or graphical frameworks.

It highlights:

clean function-based design
real-time terminal rendering
rule-based scoring systems
interactive gameplay in a console environment
Future Improvements

Planned or possible future enhancements:

upper-section bonus support
yahtzee bonus rules
save/load functionality
two-player mode
improved end-game statistics
modular multi-file project structure
cross-platform build instructions
Repository Description

Retro-style Yahtzee game in C using ncurses, featuring animated terminal gameplay, score previews, and a smarter AI opponent.

License

This project is available under the MIT License.
Add a LICENSE file if you want to officially publish it under MIT.

Author

Created by Sachith as a C programming project focused on terminal UI, game logic, and interactive gameplay.
