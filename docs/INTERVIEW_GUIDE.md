# Yahtzee Retro Deluxe: code learning and interview guide

Reviewed snapshot: `12da10cac879ba8cfef2e7e39fdb4b1a38f256dd`. The walkthrough explains code behavior; this documentation change does not claim a new interactive test run.

## Problem and scope

This is a local terminal dice game written in C with ncurses. A player and a computer opponent fill 13 scoring categories over 13 rounds. It demonstrates arrays, function pointers, control flow, scoring rules, and an event-driven terminal interface. The computer uses hand-written heuristics, not a trained machine-learning model.

All game code is in [Yahtzee.c](https://github.com/Sachith-Asmadala/Yahtzee-Retro-Deluxe/blob/12da10cac879ba8cfef2e7e39fdb4b1a38f256dd/Yahtzee.c). The capital `Y` matters on case-sensitive filesystems. A source-level compilation command is:

```bash
gcc Yahtzee.c -o yahtzee -lncurses
```

The ncurses development library and compiler must be installed. This guide does not change the build instructions elsewhere in the repository.

## Startup and component communication

`main()` seeds `rand()` using the current time, initializes ncurses, configures keyboard input, and checks that the terminal is at least 110 columns by 30 rows. It creates windows for dice, scores, previews, help, and logs. The startup menu lets the user play, read controls, or quit.

Starting a game calls `reset_game_state()`. `main()` alternates `take_player_turn()` and `take_computer_turn()` while incrementing the round. Scoring functions return integers; rendering functions read arrays/state to draw the terminal. The program deletes windows and calls `endwin()` on its normal exit path. There is no web server, database, or network protocol.

| Functions | Responsibility |
| --- | --- |
| `take_player_turn()`, `animate_roll()` | Handle keys, held dice, reroll limits, and dice updates |
| `score_full_house()`, `score_small_straight()`, other `score_*()` functions | Calculate category-specific scores |
| `scoring_functions[]`, `select_best_category()` | Dispatch scoring and choose an available category |
| `choose_ai_plan()`, `select_dice_to_reroll_smart()` | Choose heuristic computer targets and reroll masks |
| `render_all()`, `draw_*()` | Present current state |
| `pause_menu()`, `handle_pause_menu()`, `confirm_dialog()` | Pause/restart/quit interactions |
| `calculate_running_total()`, `final_screen()` | Sum used categories and show the result |

## Walkthrough: hold dice, reroll, and score a full house

1. `take_player_turn()` creates `held[5]`, sets `rolls_used` to 1, and performs the initial roll through `animate_roll()`.
2. Left/right keys change the cursor. Space toggles `held[cursor]`. Holding a die only changes a boolean; the die value remains in `dice[]`.
3. Pressing R constructs `reroll_mask[i] = !held[i]`. If three rolls have already been used, the code displays a warning. If every die is held, it asks the player to release a die or score.
4. `animate_roll()` animates temporary dice values, then copies final values into only the positions selected by the mask. The underlying random expression is `rand() % 6 + 1`.
5. Suppose the final dice are `[2, 2, 3, 3, 3]`. The category popup calls scoring functions to preview possibilities. `score_full_house()` builds a six-entry face-frequency array and sees one count of 2 and one of 3, so it returns 25.
6. After category selection, the player-turn function dispatches through `scoring_functions[category]`, animates assignment, and stores 25 in `g_player_scores[8]` (zero-based full-house index).
7. That category is now used; zero and `-1` have different meanings. The computer then takes its turn, and the round advances.

The example is a hand trace of the scoring code, not evidence that those dice were produced in a live game.

## Data structures and decisions

- `dice[5]` holds five face values; a parallel boolean array holds the keep/reroll decision per die.
- Player/computer score arrays have 13 entries. `-1` means unused; 0 is a valid recorded score.
- Frequency arrays count each face for three/four of a kind and full house. Straight scoring copies/sorts the dice, preserving the original display order; small-straight scoring removes duplicates before checking a run.
- An array of function pointers maps category indexes to scoring functions. Preview, player scoring, and computer selection reuse the same calculations.
- `ai_plan_t` identifies the computer's target pattern. It makes heuristic decisions inspectable, but does not optimize expected value over every possible future roll.
- The eight-line log is a fixed buffer that shifts older entries out when full. This favors simplicity for a tiny display history.

## Errors, limitations, and improvements

The game rejects an undersized terminal at startup, prevents reusing a category, limits rerolls, and asks for confirmation before destructive game actions. ncurses `ERR` input means no key arrived and the player loop continues. Resource handling is manual; `newwin()` return values are not consistently checked before use.

The repository has no automated scoring test suite. Mutable global state and UI/scoring code in one file make isolated testing harder. Terminal resizing during play is not explicitly handled. Scores/state are not saved. `calculate_running_total()` sums used categories; an upper-section bonus is not implemented there, and the code does not implement the complete official Yahtzee bonus/joker rules. Describe it as this game's ruleset.

Useful future work: isolate pure scoring tests (including duplicate-containing straights and all-identical dice), allow deterministic random seeds for replay, check window-allocation failures, and test pause/restart paths. A small scoring module can help if accompanied by behavior-preserving checks; a broad rewrite is unnecessary.

## Contribution evidence

GitHub attributes [commit 2d54b15](https://github.com/Sachith-Asmadala/Yahtzee-Retro-Deluxe/commit/2d54b1598833a117190be5ed5e8692ed47091620), which added `Yahtzee.c`, to `Sachith-Asmadala`. That is account attribution. Confirm independent implementation and any assistance before describing the complete program as solely your work. A documentation improvement is distinct from implementing the gameplay.

## Five interview questions and model answers

1. **Why use -1 for unused categories?** Zero is a valid score when sacrificing a category. A separate sentinel distinguishes an unused category from a used category worth zero.
2. **Why use function pointers?** The category index selects one scoring function without duplicating a large switch across previews, player turns, and computer selection.
3. **How are held dice preserved?** The player builds a reroll mask from the inverse of `held[]`. Animation commits new values only to selected positions.
4. **How does small-straight detection handle duplicates?** It sorts a copy, removes repeated values, and searches for four consecutive unique values. Thus a duplicate need not break an otherwise valid run.
5. **Is the opponent optimal AI?** No. It uses explicit pattern and score heuristics. An optimal expected-value search would be a different implementation and is not claimed here.

## Practice

Calculate every category's result for `[2, 2, 3, 3, 3]` by reading the functions. Explain the difference between changing a die value and changing its held flag. Trace what happens if a player presses R after the third roll.
