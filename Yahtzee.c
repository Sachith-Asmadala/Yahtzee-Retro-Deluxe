#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#define NUM_DICE 5
#define NUM_CATEGORIES 13
#define LOG_LINES 8
#define STATUS_LEN 160

enum {
    CP_DEFAULT = 1,
    CP_TITLE,
    CP_HOLD,
    CP_SELECTED,
    CP_HIGHLIGHT,
    CP_PLAYER,
    CP_COMPUTER,
    CP_WARNING,
    CP_PANEL
};

typedef enum {
    AI_PLAN_UPPER,
    AI_PLAN_THREE_KIND,
    AI_PLAN_FOUR_KIND,
    AI_PLAN_FULL_HOUSE,
    AI_PLAN_SMALL_STRAIGHT,
    AI_PLAN_LARGE_STRAIGHT,
    AI_PLAN_YAHTZEE,
    AI_PLAN_CHANCE
} ai_plan_t;

static const char *categories[NUM_CATEGORIES] = {
    "Ones", "Twos", "Threes", "Fours", "Fives", "Sixes",
    "Three of a Kind", "Four of a Kind", "Full House",
    "Small Straight", "Large Straight", "Yahtzee", "Chance"
};

static int g_player_scores[NUM_CATEGORIES];
static int g_computer_scores[NUM_CATEGORIES];
static int g_round = 1;
static bool g_quit_requested = false;

static WINDOW *w_header, *w_dice, *w_preview, *w_score, *w_log, *w_help;
static bool g_has_color = false;

static char g_logs[LOG_LINES][120];
static int g_log_count = 0;
static char g_status[STATUS_LEN] = "Welcome to Retro Yahtzee.";

static const char *dice_faces[7][5] = {
    {"", "", "", "", ""},
    {"+-----+", "|     |", "|  o  |", "|     |", "+-----+"},
    {"+-----+", "|o    |", "|     |", "|    o|", "+-----+"},
    {"+-----+", "|o    |", "|  o  |", "|    o|", "+-----+"},
    {"+-----+", "|o   o|", "|     |", "|o   o|", "+-----+"},
    {"+-----+", "|o   o|", "|  o  |", "|o   o|", "+-----+"},
    {"+-----+", "|o   o|", "|o   o|", "|o   o|", "+-----+"}
};

static void set_status(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(g_status, sizeof(g_status), fmt, ap);
    va_end(ap);
}

static void add_log(const char *fmt, ...) {
    va_list ap;
    char line[120];
    va_start(ap, fmt);
    vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);

    if (g_log_count < LOG_LINES) {
        strcpy(g_logs[g_log_count++], line);
    } else {
        for (int i = 1; i < LOG_LINES; i++) {
            strcpy(g_logs[i - 1], g_logs[i]);
        }
        strcpy(g_logs[LOG_LINES - 1], line);
    }
}

static int calculate_running_total(int scores[]) {
    int total = 0;
    for (int i = 0; i < NUM_CATEGORIES; i++) {
        if (scores[i] != -1) total += scores[i];
    }
    return total;
}

static void reset_game_state(void) {
    for (int i = 0; i < NUM_CATEGORIES; i++) {
        g_player_scores[i] = -1;
        g_computer_scores[i] = -1;
    }
    g_round = 1;
    g_quit_requested = false;
    g_log_count = 0;
    set_status("New game started.");
    add_log("Retro Yahtzee initialized.");
    add_log("Use arrow keys, SPACE, R, C, P.");
    add_log("Quick score keys: 1-9, 0, -, =, ]");
}

static void sort_dice(int dice[]) {
    for (int i = 0; i < NUM_DICE - 1; i++) {
        for (int j = 0; j < NUM_DICE - i - 1; j++) {
            if (dice[j] > dice[j + 1]) {
                int t = dice[j];
                dice[j] = dice[j + 1];
                dice[j + 1] = t;
            }
        }
    }
}

static int score_upper(int dice[], int number) {
    int score = 0;
    for (int i = 0; i < NUM_DICE; i++) if (dice[i] == number) score += number;
    return score;
}

static int score_upper_ones(int dice[])   { return score_upper(dice, 1); }
static int score_upper_twos(int dice[])   { return score_upper(dice, 2); }
static int score_upper_threes(int dice[]) { return score_upper(dice, 3); }
static int score_upper_fours(int dice[])  { return score_upper(dice, 4); }
static int score_upper_fives(int dice[])  { return score_upper(dice, 5); }
static int score_upper_sixes(int dice[])  { return score_upper(dice, 6); }

static int score_three_of_a_kind(int dice[]) {
    int counts[6] = {0}, sum = 0;
    for (int i = 0; i < NUM_DICE; i++) {
        counts[dice[i] - 1]++;
        sum += dice[i];
    }
    for (int i = 0; i < 6; i++) if (counts[i] >= 3) return sum;
    return 0;
}

static int score_four_of_a_kind(int dice[]) {
    int counts[6] = {0}, sum = 0;
    for (int i = 0; i < NUM_DICE; i++) {
        counts[dice[i] - 1]++;
        sum += dice[i];
    }
    for (int i = 0; i < 6; i++) if (counts[i] >= 4) return sum;
    return 0;
}

static int score_full_house(int dice[]) {
    int counts[6] = {0};
    bool has_three = false, has_two = false;
    for (int i = 0; i < NUM_DICE; i++) counts[dice[i] - 1]++;
    for (int i = 0; i < 6; i++) {
        if (counts[i] == 3) has_three = true;
        if (counts[i] == 2) has_two = true;
    }
    return (has_three && has_two) ? 25 : 0;
}

static int score_small_straight(int dice[]) {
    int sorted[NUM_DICE], unique[NUM_DICE], uniq = 0, consecutive = 1;
    for (int i = 0; i < NUM_DICE; i++) sorted[i] = dice[i];
    sort_dice(sorted);

    unique[uniq++] = sorted[0];
    for (int i = 1; i < NUM_DICE; i++) {
        if (sorted[i] != sorted[i - 1]) unique[uniq++] = sorted[i];
    }

    for (int i = 1; i < uniq; i++) {
        if (unique[i] == unique[i - 1] + 1) {
            consecutive++;
            if (consecutive >= 4) return 30;
        } else {
            consecutive = 1;
        }
    }
    return 0;
}

static int score_large_straight(int dice[]) {
    int sorted[NUM_DICE];
    for (int i = 0; i < NUM_DICE; i++) sorted[i] = dice[i];
    sort_dice(sorted);
    for (int i = 1; i < NUM_DICE; i++) {
        if (sorted[i] != sorted[i - 1] + 1) return 0;
    }
    return 40;
}

static int score_yahtzee(int dice[]) {
    for (int i = 1; i < NUM_DICE; i++) if (dice[i] != dice[0]) return 0;
    return 50;
}

static int score_chance(int dice[]) {
    int sum = 0;
    for (int i = 0; i < NUM_DICE; i++) sum += dice[i];
    return sum;
}

static int (*scoring_functions[NUM_CATEGORIES])(int dice[]) = {
    score_upper_ones, score_upper_twos, score_upper_threes,
    score_upper_fours, score_upper_fives, score_upper_sixes,
    score_three_of_a_kind, score_four_of_a_kind, score_full_house,
    score_small_straight, score_large_straight, score_yahtzee, score_chance
};

static int select_best_category(int dice[], int scores[]) {
    int best_score = -1, best_category = -1;
    for (int i = 0; i < NUM_CATEGORIES; i++) {
        if (scores[i] == -1) {
            int s = scoring_functions[i](dice);
            if (s > best_score) {
                best_score = s;
                best_category = i;
            }
        }
    }
    return best_category;
}

/* ---------- smarter AI helpers ---------- */

static int compare_ints(const void *a, const void *b) {
    return (*(const int *)a - *(const int *)b);
}

static int build_unique_sorted(int dice[], int out[]) {
    int tmp[NUM_DICE];
    for (int i = 0; i < NUM_DICE; i++) tmp[i] = dice[i];
    qsort(tmp, NUM_DICE, sizeof(int), compare_ints);

    int n = 0;
    for (int i = 0; i < NUM_DICE; i++) {
        if (i == 0 || tmp[i] != tmp[i - 1]) out[n++] = tmp[i];
    }
    return n;
}

static int longest_straight_run(int dice[], int *best_start, int *best_len) {
    int u[NUM_DICE];
    int n = build_unique_sorted(dice, u);

    int current_start = u[0];
    int current_len = 1;
    *best_start = u[0];
    *best_len = 1;

    for (int i = 1; i < n; i++) {
        if (u[i] == u[i - 1] + 1) {
            current_len++;
        } else {
            if (current_len > *best_len) {
                *best_len = current_len;
                *best_start = current_start;
            }
            current_start = u[i];
            current_len = 1;
        }
    }

    if (current_len > *best_len) {
        *best_len = current_len;
        *best_start = current_start;
    }
    return *best_len;
}

static int count_value(int dice[], int value) {
    int c = 0;
    for (int i = 0; i < NUM_DICE; i++) if (dice[i] == value) c++;
    return c;
}

static void counts_of_faces(int dice[], int counts[7]) {
    for (int i = 0; i < 7; i++) counts[i] = 0;
    for (int i = 0; i < NUM_DICE; i++) counts[dice[i]]++;
}

static int most_common_face(int dice[]) {
    int counts[7];
    counts_of_faces(dice, counts);

    int best_face = 1;
    int best_count = counts[1];
    for (int v = 2; v <= 6; v++) {
        if (counts[v] > best_count || (counts[v] == best_count && v > best_face)) {
            best_count = counts[v];
            best_face = v;
        }
    }
    return best_face;
}

static bool has_two_pairs(int dice[]) {
    int counts[7], pairs = 0;
    counts_of_faces(dice, counts);
    for (int v = 1; v <= 6; v++) if (counts[v] >= 2) pairs++;
    return pairs >= 2;
}

static ai_plan_t choose_ai_plan(int dice[], int scores[]) {
    int best_cat = select_best_category(dice, scores);
    int best_score = (best_cat >= 0) ? scoring_functions[best_cat](dice) : 0;

    int run_start, run_len;
    longest_straight_run(dice, &run_start, &run_len);

    if (scores[10] == -1 && score_large_straight(dice) == 40) return AI_PLAN_LARGE_STRAIGHT;
    if (scores[9]  == -1 && score_small_straight(dice) == 30) return AI_PLAN_SMALL_STRAIGHT;
    if (scores[11] == -1 && score_yahtzee(dice) == 50) return AI_PLAN_YAHTZEE;
    if (scores[8]  == -1 && score_full_house(dice) == 25) return AI_PLAN_FULL_HOUSE;

    if (scores[10] == -1 && run_len >= 4) return AI_PLAN_LARGE_STRAIGHT;
    if (scores[9]  == -1 && run_len >= 3) return AI_PLAN_SMALL_STRAIGHT;
    if (scores[8] == -1 && has_two_pairs(dice)) return AI_PLAN_FULL_HOUSE;

    if (best_cat >= 0 && best_cat <= 5) return AI_PLAN_UPPER;
    if (best_cat == 6) return AI_PLAN_THREE_KIND;
    if (best_cat == 7) return AI_PLAN_FOUR_KIND;
    if (best_cat == 8) return AI_PLAN_FULL_HOUSE;
    if (best_cat == 9) return AI_PLAN_SMALL_STRAIGHT;
    if (best_cat == 10) return AI_PLAN_LARGE_STRAIGHT;
    if (best_cat == 11) return AI_PLAN_YAHTZEE;

    if (best_score >= 20) return AI_PLAN_CHANCE;
    return AI_PLAN_UPPER;
}

static void select_dice_for_straight_plan(int dice[], bool reroll_mask[]) {
    int best_start = 1, best_len = 1;
    longest_straight_run(dice, &best_start, &best_len);

    bool keep_value[7] = {false};
    bool used_value[7] = {false};

    for (int v = best_start; v < best_start + best_len && v <= 6; v++) keep_value[v] = true;
    for (int i = 0; i < NUM_DICE; i++) reroll_mask[i] = true;

    for (int i = 0; i < NUM_DICE; i++) {
        int v = dice[i];
        if (keep_value[v] && !used_value[v]) {
            reroll_mask[i] = false;
            used_value[v] = true;
        }
    }
}

static void select_dice_to_reroll_smart(int dice[], bool reroll_mask[], ai_plan_t plan) {
    int counts[7];
    counts_of_faces(dice, counts);

    for (int i = 0; i < NUM_DICE; i++) reroll_mask[i] = false;

    if (plan == AI_PLAN_LARGE_STRAIGHT || plan == AI_PLAN_SMALL_STRAIGHT) {
        select_dice_for_straight_plan(dice, reroll_mask);
        return;
    }

    if (plan == AI_PLAN_FULL_HOUSE) {
        int face_a = -1, face_b = -1;
        for (int v = 1; v <= 6; v++) {
            if (counts[v] >= 2) {
                if (face_a == -1) face_a = v;
                else if (face_b == -1) face_b = v;
            }
        }

        if (face_a != -1 && face_b != -1) {
            int kept_a = 0, kept_b = 0;
            for (int i = 0; i < NUM_DICE; i++) {
                if (dice[i] == face_a && kept_a < 3) kept_a++;
                else if (dice[i] == face_b && kept_b < 2) kept_b++;
                else reroll_mask[i] = true;
            }
            return;
        }
    }

    if (plan == AI_PLAN_YAHTZEE || plan == AI_PLAN_FOUR_KIND || plan == AI_PLAN_THREE_KIND || plan == AI_PLAN_UPPER) {
        int target = most_common_face(dice);
        for (int i = 0; i < NUM_DICE; i++) {
            if (dice[i] != target) reroll_mask[i] = true;
        }
        return;
    }

    if (plan == AI_PLAN_CHANCE) {
        for (int i = 0; i < NUM_DICE; i++) {
            if (dice[i] <= 3) reroll_mask[i] = true;
        }
    }
}

static bool should_computer_reroll_smart(int dice[], int scores[], ai_plan_t *plan_out) {
    *plan_out = choose_ai_plan(dice, scores);

    if (score_yahtzee(dice) == 50) return false;
    if (score_large_straight(dice) == 40) return false;
    if (score_full_house(dice) == 25 && scores[8] == -1) return false;

    switch (*plan_out) {
        case AI_PLAN_LARGE_STRAIGHT: {
            int s, l;
            longest_straight_run(dice, &s, &l);
            return l < 5;
        }
        case AI_PLAN_SMALL_STRAIGHT: {
            int s, l;
            longest_straight_run(dice, &s, &l);
            return l < 4;
        }
        case AI_PLAN_FULL_HOUSE:
            return score_full_house(dice) == 0;
        case AI_PLAN_YAHTZEE:
            return score_yahtzee(dice) == 0;
        case AI_PLAN_FOUR_KIND:
            return score_four_of_a_kind(dice) == 0;
        case AI_PLAN_THREE_KIND:
            return score_three_of_a_kind(dice) == 0;
        case AI_PLAN_UPPER: {
            int target = most_common_face(dice);
            return count_value(dice, target) < 4;
        }
        case AI_PLAN_CHANCE:
            return score_chance(dice) < 22;
        default:
            return false;
    }
}

static const char *ai_plan_name(ai_plan_t plan) {
    switch (plan) {
        case AI_PLAN_UPPER: return "Upper Section";
        case AI_PLAN_THREE_KIND: return "Three of a Kind";
        case AI_PLAN_FOUR_KIND: return "Four of a Kind";
        case AI_PLAN_FULL_HOUSE: return "Full House";
        case AI_PLAN_SMALL_STRAIGHT: return "Small Straight";
        case AI_PLAN_LARGE_STRAIGHT: return "Large Straight";
        case AI_PLAN_YAHTZEE: return "Yahtzee";
        case AI_PLAN_CHANCE: return "Chance";
        default: return "Unknown";
    }
}

/* ---------- UI ---------- */

static void event_fx(int category, int score) {
    if (category == 11 || score >= 50) {
        beep();
        flash();
        beep();
    } else if (category == 10 || category == 8 || score >= 25) {
        beep();
    } else if (score == 0) {
        flash();
    }
}

static void init_colors_ui(void) {
    if (has_colors()) {
        g_has_color = true;
        start_color();
        use_default_colors();

        init_pair(CP_DEFAULT,   COLOR_WHITE,  -1);
        init_pair(CP_TITLE,     COLOR_CYAN,   -1);
        init_pair(CP_HOLD,      COLOR_GREEN,  -1);
        init_pair(CP_SELECTED,  COLOR_YELLOW, -1);
        init_pair(CP_HIGHLIGHT, COLOR_BLACK,  COLOR_CYAN);
        init_pair(CP_PLAYER,    COLOR_GREEN,  -1);
        init_pair(CP_COMPUTER,  COLOR_RED,    -1);
        init_pair(CP_WARNING,   COLOR_YELLOW, -1);
        init_pair(CP_PANEL,     COLOR_MAGENTA, -1);
    }
}

static void draw_box_title(WINDOW *w, const char *title, int pair) {
    box(w, 0, 0);
    if (g_has_color) wattron(w, COLOR_PAIR(pair) | A_BOLD);
    else wattron(w, A_BOLD);
    mvwprintw(w, 0, 2, " %s ", title);
    if (g_has_color) wattroff(w, COLOR_PAIR(pair) | A_BOLD);
    else wattroff(w, A_BOLD);
}

static void draw_header(void) {
    werase(w_header);
    draw_box_title(w_header, "YAHTZEE RETRO", CP_TITLE);
    mvwprintw(w_header, 1, 2, "Round %d / 13", g_round);
    mvwprintw(w_header, 1, 24, "Player: %d", calculate_running_total(g_player_scores));
    mvwprintw(w_header, 1, 42, "Computer: %d", calculate_running_total(g_computer_scores));
    wrefresh(w_header);
}

static void draw_log_window(void) {
    werase(w_log);
    draw_box_title(w_log, "EVENT LOG", CP_DEFAULT);
    int start = (g_log_count > getmaxy(w_log) - 2) ? g_log_count - (getmaxy(w_log) - 2) : 0;
    int row = 1;
    for (int i = start; i < g_log_count && row < getmaxy(w_log) - 1; i++, row++) {
        mvwprintw(w_log, row, 2, "%s", g_logs[i]);
    }
    wrefresh(w_log);
}

static void draw_help_window(bool player_turn, int rolls_used) {
    werase(w_help);
    draw_box_title(w_help, "CONTROLS", CP_DEFAULT);

    if (player_turn) {
        mvwprintw(w_help, 1, 2, "LEFT/RIGHT move  SPACE hold  R reroll  C menu  P pause");
        mvwprintw(w_help, 2, 2, "Quick score: 1-9, 0, -, =, ]   Rolls used: %d/3", rolls_used);
    } else {
        mvwprintw(w_help, 1, 2, "Computer is taking its turn...");
        mvwprintw(w_help, 2, 2, "Sit back and watch the retro drama.");
    }

    if (g_has_color) wattron(w_help, COLOR_PAIR(CP_WARNING) | A_BOLD);
    mvwprintw(w_help, 1, getmaxx(w_help) - (int)strlen(g_status) - 4, "%s", g_status);
    if (g_has_color) wattroff(w_help, COLOR_PAIR(CP_WARNING) | A_BOLD);

    wrefresh(w_help);
}

static void draw_scoreboard_custom(int p[], int c[], int highlight_row, int highlight_side) {
    werase(w_score);
    draw_box_title(w_score, "SCOREBOARD", CP_DEFAULT);

    mvwprintw(w_score, 1, 2, "%-14s %-5s %-5s", "Category", "You", "CPU");

    int max_rows = getmaxy(w_score) - 6;
    for (int i = 0; i < NUM_CATEGORIES && i < max_rows; i++) {
        int y = i + 2;

        if (i == highlight_row) {
            if (g_has_color) wattron(w_score, COLOR_PAIR(CP_HIGHLIGHT) | A_BOLD);
            else wattron(w_score, A_REVERSE | A_BOLD);
        }

        mvwprintw(w_score, y, 2, "%-14.14s", categories[i]);

        if (p[i] == -1) mvwprintw(w_score, y, 17, "%-5s", "---");
        else            mvwprintw(w_score, y, 17, "%-5d", p[i]);

        if (c[i] == -1) mvwprintw(w_score, y, 23, "%-5s", "---");
        else            mvwprintw(w_score, y, 23, "%-5d", c[i]);

        if (i == highlight_row) {
            if (g_has_color) wattroff(w_score, COLOR_PAIR(CP_HIGHLIGHT) | A_BOLD);
            else wattroff(w_score, A_REVERSE | A_BOLD);
        }
    }

    int y = getmaxy(w_score) - 3;

    if (g_has_color) wattron(w_score, COLOR_PAIR(CP_PLAYER) | A_BOLD);
    mvwprintw(w_score, y, 2, "P:%d", calculate_running_total(p));
    if (g_has_color) wattroff(w_score, COLOR_PAIR(CP_PLAYER) | A_BOLD);

    if (g_has_color) wattron(w_score, COLOR_PAIR(CP_COMPUTER) | A_BOLD);
    mvwprintw(w_score, y, 14, "C:%d", calculate_running_total(c));
    if (g_has_color) wattroff(w_score, COLOR_PAIR(CP_COMPUTER) | A_BOLD);

    mvwprintw(w_score, y + 1, 2, "Hi:%s",
              (highlight_side == 0) ? "Player" :
              (highlight_side == 1) ? "CPU" : "None");

    wrefresh(w_score);
}

static void draw_scoreboard(void) {
    draw_scoreboard_custom(g_player_scores, g_computer_scores, -1, -1);
}

static void draw_preview_window(int dice[]) {
    werase(w_preview);
    draw_box_title(w_preview, "CATEGORY PREVIEW", CP_DEFAULT);

    mvwprintw(w_preview, 1, 2, "%-18s %-6s", "Category", "Score");

    int row = 2;
    for (int i = 0; i < NUM_CATEGORIES && row < getmaxy(w_preview) - 1; i++, row++) {
        int preview = scoring_functions[i](dice);

        if (g_player_scores[i] == -1) mvwprintw(w_preview, row, 2, "%-18s %3d", categories[i], preview);
        else                          mvwprintw(w_preview, row, 2, "%-18s USED", categories[i]);
    }

    wrefresh(w_preview);
}

static void draw_dice_window(int dice[], bool held[], int cursor, bool blink_on, bool player_turn) {
    werase(w_dice);
    draw_box_title(w_dice, player_turn ? "PLAYER DICE" : "COMPUTER DICE", CP_DEFAULT);

    int start_x = 2;
    int top_y = 2;
    int gap = 10;

    for (int i = 0; i < NUM_DICE; i++) {
        int x = start_x + i * gap;
        bool selected = (i == cursor);
        bool is_held = held ? held[i] : false;

        if (selected && blink_on) {
            if (g_has_color) wattron(w_dice, COLOR_PAIR(CP_SELECTED) | A_BOLD);
            else wattron(w_dice, A_REVERSE | A_BOLD);
        } else if (is_held) {
            if (g_has_color) wattron(w_dice, COLOR_PAIR(CP_HOLD) | A_BOLD);
            else wattron(w_dice, A_BOLD);
        }

        for (int row = 0; row < 5; row++) {
            mvwprintw(w_dice, top_y + row, x, "%s", dice_faces[dice[i]][row]);
        }

        mvwprintw(w_dice, top_y + 6, x + 2, "%d", i + 1);
        mvwprintw(w_dice, top_y + 7, x + 2, "%d", dice[i]);

        if (is_held) mvwprintw(w_dice, top_y + 8, x + 1, "HOLD");

        if (selected && blink_on) {
            if (g_has_color) wattroff(w_dice, COLOR_PAIR(CP_SELECTED) | A_BOLD);
            else wattroff(w_dice, A_REVERSE | A_BOLD);
        } else if (is_held) {
            if (g_has_color) wattroff(w_dice, COLOR_PAIR(CP_HOLD) | A_BOLD);
            else wattroff(w_dice, A_BOLD);
        }
    }

    mvwprintw(w_dice, 10, 2, "Selected die blinks. Held dice stay highlighted.");
    wrefresh(w_dice);
}

static void render_all(int dice[], bool held[], int cursor, bool blink_on, bool player_turn, int rolls_used) {
    draw_header();
    draw_dice_window(dice, held, cursor, blink_on, player_turn);
    draw_preview_window(dice);
    draw_scoreboard();
    draw_log_window();
    draw_help_window(player_turn, rolls_used);
}

static void animate_score_assignment(int side, int category, int final_score,
                                     int dice[], bool held[], int cursor, bool player_turn, int rolls_used) {
    int p[NUM_CATEGORIES], c[NUM_CATEGORIES];
    memcpy(p, g_player_scores, sizeof(p));
    memcpy(c, g_computer_scores, sizeof(c));

    int step = (final_score <= 12) ? 1 : final_score / 8;
    if (step < 1) step = 1;

    render_all(dice, held, cursor, true, player_turn, rolls_used);

    if (final_score == 0) {
        if (side == 0) p[category] = 0;
        else c[category] = 0;
        draw_scoreboard_custom(p, c, category, side);
        napms(250);
        return;
    }

    for (int v = 0; v <= final_score; v += step) {
        if (side == 0) p[category] = v;
        else c[category] = v;
        draw_scoreboard_custom(p, c, category, side);
        napms(45);
    }

    if (side == 0) p[category] = final_score;
    else c[category] = final_score;

    draw_scoreboard_custom(p, c, category, side);
    napms(180);
}

static void animate_roll(int dice[], bool reroll_mask[], bool held_view[],
                         int cursor, bool player_turn, int rolls_used, const char *status) {
    int frames = 10;
    int temp[NUM_DICE];

    for (int i = 0; i < NUM_DICE; i++) temp[i] = dice[i];
    set_status("%s", status);

    for (int f = 0; f < frames; f++) {
        for (int i = 0; i < NUM_DICE; i++) {
            if (!reroll_mask || reroll_mask[i]) temp[i] = rand() % 6 + 1;
        }
        render_all(temp, held_view, cursor, (f % 2 == 0), player_turn, rolls_used);
        napms(70 + f * 6);
    }

    for (int i = 0; i < NUM_DICE; i++) {
        if (!reroll_mask || reroll_mask[i]) dice[i] = temp[i];
    }

    render_all(dice, held_view, cursor, true, player_turn, rolls_used);
}

static int category_popup(int dice[]) {
    int h = 18, w = 44;
    int y = (LINES - h) / 2;
    int x = (COLS - w) / 2;
    WINDOW *pop = newwin(h, w, y, x);
    keypad(pop, TRUE);

    int current = 0;
    while (g_player_scores[current] != -1) current = (current + 1) % NUM_CATEGORIES;

    while (1) {
        werase(pop);
        box(pop, 0, 0);
        if (g_has_color) wattron(pop, COLOR_PAIR(CP_TITLE) | A_BOLD);
        mvwprintw(pop, 0, 2, " Choose Category ");
        if (g_has_color) wattroff(pop, COLOR_PAIR(CP_TITLE) | A_BOLD);

        for (int i = 0; i < NUM_CATEGORIES; i++) {
            int preview = scoring_functions[i](dice);

            if (i == current) {
                if (g_has_color) wattron(pop, COLOR_PAIR(CP_HIGHLIGHT) | A_BOLD);
                else wattron(pop, A_REVERSE | A_BOLD);
            }

            if (g_player_scores[i] == -1) mvwprintw(pop, i + 1, 2, "%2d. %-18s %3d", i + 1, categories[i], preview);
            else                          mvwprintw(pop, i + 1, 2, "%2d. %-18s USED", i + 1, categories[i]);

            if (i == current) {
                if (g_has_color) wattroff(pop, COLOR_PAIR(CP_HIGHLIGHT) | A_BOLD);
                else wattroff(pop, A_REVERSE | A_BOLD);
            }
        }

        mvwprintw(pop, h - 2, 2, "UP/DOWN move, ENTER confirm");
        wrefresh(pop);

        int ch = wgetch(pop);
        if (ch == KEY_UP) {
            do { current = (current - 1 + NUM_CATEGORIES) % NUM_CATEGORIES; }
            while (g_player_scores[current] != -1);
        } else if (ch == KEY_DOWN) {
            do { current = (current + 1) % NUM_CATEGORIES; }
            while (g_player_scores[current] != -1);
        } else if (ch == '\n' || ch == KEY_ENTER || ch == 10 || ch == 13) {
            delwin(pop);
            touchwin(stdscr);
            refresh();
            return current;
        }
    }
}

static int quick_key_to_category(int ch) {
    switch (ch) {
        case '1': return 0;
        case '2': return 1;
        case '3': return 2;
        case '4': return 3;
        case '5': return 4;
        case '6': return 5;
        case '7': return 6;
        case '8': return 7;
        case '9': return 8;
        case '0': return 9;
        case '-': return 10;
        case '=': return 11;
        case ']': return 12;
        default: return -1;
    }
}

static int pause_menu(void) {
    int h = 12, w = 40;
    int y = (LINES - h) / 2;
    int x = (COLS - w) / 2;
    WINDOW *pop = newwin(h, w, y, x);
    keypad(pop, TRUE);

    const char *items[] = {
        "Resume",
        "Restart Game",
        "Quit"
    };
    int count = 3;
    int current = 0;

    while (1) {
        werase(pop);
        box(pop, 0, 0);
        if (g_has_color) wattron(pop, COLOR_PAIR(CP_PANEL) | A_BOLD);
        mvwprintw(pop, 0, 2, " Pause Menu ");
        if (g_has_color) wattroff(pop, COLOR_PAIR(CP_PANEL) | A_BOLD);

        mvwprintw(pop, 2, 2, "Game paused.");
        for (int i = 0; i < count; i++) {
            if (i == current) {
                if (g_has_color) wattron(pop, COLOR_PAIR(CP_HIGHLIGHT) | A_BOLD);
                else wattron(pop, A_REVERSE | A_BOLD);
            }
            mvwprintw(pop, 4 + i, 4, "%s", items[i]);
            if (i == current) {
                if (g_has_color) wattroff(pop, COLOR_PAIR(CP_HIGHLIGHT) | A_BOLD);
                else wattroff(pop, A_REVERSE | A_BOLD);
            }
        }
        mvwprintw(pop, h - 2, 2, "UP/DOWN move, ENTER select");
        wrefresh(pop);

        int ch = wgetch(pop);
        if (ch == KEY_UP) current = (current - 1 + count) % count;
        else if (ch == KEY_DOWN) current = (current + 1) % count;
        else if (ch == '\n' || ch == KEY_ENTER || ch == 10 || ch == 13) {
            delwin(pop);
            touchwin(stdscr);
            refresh();
            return current;
        } else if (ch == 'p' || ch == 'P' || ch == 27) {
            delwin(pop);
            touchwin(stdscr);
            refresh();
            return 0;
        }
    }
}

static bool confirm_dialog(const char *title, const char *message) {
    int h = 9, w = 50;
    int y = (LINES - h) / 2;
    int x = (COLS - w) / 2;
    WINDOW *pop = newwin(h, w, y, x);
    keypad(pop, TRUE);

    int current = 0; /* 0 yes, 1 no */

    while (1) {
        werase(pop);
        box(pop, 0, 0);
        if (g_has_color) wattron(pop, COLOR_PAIR(CP_PANEL) | A_BOLD);
        mvwprintw(pop, 0, 2, " %s ", title);
        if (g_has_color) wattroff(pop, COLOR_PAIR(CP_PANEL) | A_BOLD);

        mvwprintw(pop, 2, 2, "%s", message);

        if (current == 0) {
            if (g_has_color) wattron(pop, COLOR_PAIR(CP_HIGHLIGHT) | A_BOLD);
            else wattron(pop, A_REVERSE | A_BOLD);
        }
        mvwprintw(pop, 5, 12, "Yes");
        if (current == 0) {
            if (g_has_color) wattroff(pop, COLOR_PAIR(CP_HIGHLIGHT) | A_BOLD);
            else wattroff(pop, A_REVERSE | A_BOLD);
        }

        if (current == 1) {
            if (g_has_color) wattron(pop, COLOR_PAIR(CP_HIGHLIGHT) | A_BOLD);
            else wattron(pop, A_REVERSE | A_BOLD);
        }
        mvwprintw(pop, 5, 26, "No");
        if (current == 1) {
            if (g_has_color) wattroff(pop, COLOR_PAIR(CP_HIGHLIGHT) | A_BOLD);
            else wattroff(pop, A_REVERSE | A_BOLD);
        }

        mvwprintw(pop, 7, 2, "LEFT/RIGHT move, ENTER confirm");
        wrefresh(pop);

        int ch = wgetch(pop);
        if (ch == KEY_LEFT || ch == KEY_RIGHT || ch == '\t') current = 1 - current;
        else if (ch == '\n' || ch == KEY_ENTER || ch == 10 || ch == 13) {
            bool result = (current == 0);
            delwin(pop);
            touchwin(stdscr);
            refresh();
            return result;
        } else if (ch == 27) {
            delwin(pop);
            touchwin(stdscr);
            refresh();
            return false;
        }
    }
}

static int startup_screen(void) {
    int current = 0;
    const char *items[] = {
        "Start Game",
        "How to Play",
        "Quit"
    };
    int count = 3;
    keypad(stdscr, TRUE);
    timeout(-1);

    while (1) {
        werase(stdscr);
        box(stdscr, 0, 0);

        if (g_has_color) attron(COLOR_PAIR(CP_TITLE) | A_BOLD);
        mvprintw(2, (COLS - 30) / 2, "YAHTZEE RETRO DELUXE");
        attroff(A_BOLD);
        if (g_has_color) attroff(COLOR_PAIR(CP_TITLE));

        mvprintw(4, (COLS - 42) / 2, "Animated terminal Yahtzee for your Mac");
        mvprintw(6, (COLS - 54) / 2, "Hold dice, chase straights, and beat the computer.");

        int art_y = 8;
        mvprintw(art_y + 0, (COLS - 39) / 2, "+-----+  +-----+  +-----+");
        mvprintw(art_y + 1, (COLS - 39) / 2, "|o   o|  |  o  |  |o    |");
        mvprintw(art_y + 2, (COLS - 39) / 2, "|  o  |  |     |  |  o  |");
        mvprintw(art_y + 3, (COLS - 39) / 2, "|o   o|  |  o  |  |    o|");
        mvprintw(art_y + 4, (COLS - 39) / 2, "+-----+  +-----+  +-----+");

        for (int i = 0; i < count; i++) {
            int y = 15 + i;
            int x = (COLS - 16) / 2;
            if (i == current) {
                if (g_has_color) attron(COLOR_PAIR(CP_HIGHLIGHT) | A_BOLD);
                else attron(A_REVERSE | A_BOLD);
            }
            mvprintw(y, x, "%s", items[i]);
            if (i == current) {
                if (g_has_color) attroff(COLOR_PAIR(CP_HIGHLIGHT) | A_BOLD);
                else attroff(A_REVERSE | A_BOLD);
            }
        }

        mvprintw(LINES - 4, 4, "UP/DOWN move   ENTER select");
        mvprintw(LINES - 3, 4, "Controls in game: arrows, SPACE, R, C, P");
        refresh();

        int ch = getch();
        if (ch == KEY_UP) current = (current - 1 + count) % count;
        else if (ch == KEY_DOWN) current = (current + 1) % count;
        else if (ch == '\n' || ch == KEY_ENTER || ch == 10 || ch == 13) {
            return current;
        }
    }
}

static void show_how_to_play(void) {
    timeout(-1);
    while (1) {
        werase(stdscr);
        box(stdscr, 0, 0);

        if (g_has_color) attron(COLOR_PAIR(CP_PANEL) | A_BOLD);
        mvprintw(2, 3, "HOW TO PLAY");
        if (g_has_color) attroff(COLOR_PAIR(CP_PANEL) | A_BOLD);

        mvprintw(4, 3, "Goal:");
        mvprintw(5, 5, "Score more points than the computer after 13 rounds.");

        mvprintw(7, 3, "Turn flow:");
        mvprintw(8, 5, "1. Roll five dice.");
        mvprintw(9, 5, "2. Hold any dice you want to keep.");
        mvprintw(10, 5, "3. Reroll the rest, up to 2 rerolls.");
        mvprintw(11, 5, "4. Choose a category to score.");

        mvprintw(13, 3, "Controls:");
        mvprintw(14, 5, "LEFT/RIGHT  move between dice");
        mvprintw(15, 5, "SPACE       hold/unhold die");
        mvprintw(16, 5, "R           reroll unheld dice");
        mvprintw(17, 5, "C           category popup");
        mvprintw(18, 5, "P           pause menu");
        mvprintw(19, 5, "1-9,0,-,=,] quick-score categories 1-13");

        mvprintw(LINES - 3, 3, "Press any key to go back.");
        refresh();
        getch();
        return;
    }
}

static int handle_pause_menu(void) {
    int action = pause_menu();
    if (action == 0) {
        set_status("Resumed.");
        return 0;
    }
    if (action == 1) {
        if (confirm_dialog("Restart Game", "Restart the whole game? Current progress will be lost.")) {
            reset_game_state();
            return 1;
        }
        return 0;
    }
    if (action == 2) {
        if (confirm_dialog("Quit Game", "Quit to terminal? Unsaved progress will be lost.")) {
            g_quit_requested = true;
            return 2;
        }
        return 0;
    }
    return 0;
}

static int take_player_turn(int dice[]) {
    bool held[NUM_DICE] = {false, false, false, false, false};
    bool all_mask[NUM_DICE] = {true, true, true, true, true};
    int cursor = 0;
    int rolls_used = 1;
    bool blink_on = true;

    add_log("Round %d: player turn begins.", g_round);
    animate_roll(dice, all_mask, held, cursor, true, rolls_used, "Initial roll");

    timeout(140);

    while (1) {
        render_all(dice, held, cursor, blink_on, true, rolls_used);
        blink_on = !blink_on;

        int ch = getch();
        if (ch == ERR) continue;

        if (ch == 'p' || ch == 'P') {
            timeout(-1);
            int pause_result = handle_pause_menu();
            if (pause_result == 1 || pause_result == 2) {
                timeout(140);
                return -2;
            }
            timeout(140);
            continue;
        }

        int quick_cat = quick_key_to_category(ch);
        if (quick_cat != -1) {
            if (g_player_scores[quick_cat] != -1) {
                set_status("Category already used.");
                flash();
                continue;
            }

            timeout(-1);
            int score = scoring_functions[quick_cat](dice);
            add_log("Player chooses %s for %d points.", categories[quick_cat], score);
            set_status("Scoring %s", categories[quick_cat]);
            animate_score_assignment(0, quick_cat, score, dice, held, cursor, true, rolls_used);
            g_player_scores[quick_cat] = score;
            event_fx(quick_cat, score);
            timeout(140);
            return quick_cat;
        }

        if (ch == KEY_LEFT) {
            cursor = (cursor - 1 + NUM_DICE) % NUM_DICE;
        } else if (ch == KEY_RIGHT) {
            cursor = (cursor + 1) % NUM_DICE;
        } else if (ch == ' ' || ch == 'h' || ch == 'H') {
            held[cursor] = !held[cursor];
            if (held[cursor]) add_log("Held die %d.", cursor + 1);
            else add_log("Released die %d.", cursor + 1);
        } else if (ch == 'r' || ch == 'R') {
            if (rolls_used >= 3) {
                set_status("No rerolls left.");
                flash();
                continue;
            }

            bool reroll_mask[NUM_DICE];
            int count = 0;
            for (int i = 0; i < NUM_DICE; i++) {
                reroll_mask[i] = !held[i];
                if (reroll_mask[i]) count++;
            }

            if (count == 0) {
                set_status("All dice are held. Release one or score.");
                beep();
                continue;
            }

            rolls_used++;
            add_log("Player rerolls %d dice.", count);
            animate_roll(dice, reroll_mask, held, cursor, true, rolls_used, "Rerolling selected dice");
        } else if (ch == 'c' || ch == 'C') {
            timeout(-1);
            int cat = category_popup(dice);
            int score = scoring_functions[cat](dice);
            add_log("Player chooses %s for %d points.", categories[cat], score);
            set_status("Scoring %s", categories[cat]);
            animate_score_assignment(0, cat, score, dice, held, cursor, true, rolls_used);
            g_player_scores[cat] = score;
            event_fx(cat, score);
            timeout(140);
            return cat;
        }
    }
}

static void take_computer_turn(int dice[]) {
    bool kept_view[NUM_DICE] = {false, false, false, false, false};
    bool all_mask[NUM_DICE] = {true, true, true, true, true};
    int rolls_used = 1;

    add_log("Computer turn begins.");
    animate_roll(dice, all_mask, kept_view, -1, false, rolls_used, "Computer rolling");

    for (int reroll = 0; reroll < 2; reroll++) {
        ai_plan_t plan;
        if (!should_computer_reroll_smart(dice, g_computer_scores, &plan)) {
            add_log("Computer keeps current dice.");
            break;
        }

        bool reroll_mask[NUM_DICE];
        select_dice_to_reroll_smart(dice, reroll_mask, plan);

        int reroll_count = 0;
        for (int i = 0; i < NUM_DICE; i++) {
            kept_view[i] = !reroll_mask[i];
            if (reroll_mask[i]) reroll_count++;
        }

        if (reroll_count == 0) {
            add_log("Computer keeps all dice for %s.", ai_plan_name(plan));
            break;
        }

        add_log("Computer aims for %s.", ai_plan_name(plan));
        add_log("Computer rerolls %d dice.", reroll_count);

        rolls_used++;
        set_status("Computer is thinking about %s...", ai_plan_name(plan));
        render_all(dice, kept_view, -1, true, false, rolls_used);
        napms(500);

        animate_roll(dice, reroll_mask, kept_view, -1, false, rolls_used, "Computer reroll");
    }

    memset(kept_view, 0, sizeof(kept_view));

    int cat = select_best_category(dice, g_computer_scores);
    int score = scoring_functions[cat](dice);

    add_log("Computer chooses %s for %d points.", categories[cat], score);
    set_status("Computer scores %s", categories[cat]);
    animate_score_assignment(1, cat, score, dice, kept_view, -1, false, rolls_used);
    g_computer_scores[cat] = score;
    event_fx(cat, score);
    napms(450);
}

static void final_screen(void) {
    int p = calculate_running_total(g_player_scores);
    int c = calculate_running_total(g_computer_scores);

    werase(stdscr);
    box(stdscr, 0, 0);

    if (g_has_color) attron(COLOR_PAIR(CP_TITLE) | A_BOLD);
    mvprintw(2, (COLS - 20) / 2, "FINAL RESULTS");
    if (g_has_color) attroff(COLOR_PAIR(CP_TITLE) | A_BOLD);

    mvprintw(5, (COLS - 24) / 2, "Player Total   : %d", p);
    mvprintw(6, (COLS - 24) / 2, "Computer Total : %d", c);

    if (p > c) {
        if (g_has_color) attron(COLOR_PAIR(CP_PLAYER) | A_BOLD);
        mvprintw(9, (COLS - 18) / 2, "You win. Nice.");
        if (g_has_color) attroff(COLOR_PAIR(CP_PLAYER) | A_BOLD);
        beep();
    } else if (c > p) {
        if (g_has_color) attron(COLOR_PAIR(CP_COMPUTER) | A_BOLD);
        mvprintw(9, (COLS - 26) / 2, "Computer wins this round.");
        if (g_has_color) attroff(COLOR_PAIR(CP_COMPUTER) | A_BOLD);
        flash();
    } else {
        if (g_has_color) attron(COLOR_PAIR(CP_WARNING) | A_BOLD);
        mvprintw(9, (COLS - 12) / 2, "It's a tie.");
        if (g_has_color) attroff(COLOR_PAIR(CP_WARNING) | A_BOLD);
        beep();
    }

    mvprintw(12, (COLS - 36) / 2, "Press any key to return to the menu.");
    refresh();
    timeout(-1);
    getch();
}

int main(void) {
    srand((unsigned int)time(NULL));

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    if (LINES < 30 || COLS < 110) {
        endwin();
        fprintf(stderr, "Please use a terminal of at least 110x30.\n");
        return 1;
    }

    init_colors_ui();

    int header_h = 3;
    int help_h = 4;
    int body_h = LINES - header_h - help_h;

    int right_w = 38;
    int left_w = COLS - right_w;

    int dice_h = 13;
    int log_h = body_h - dice_h;

    int preview_h = 16;
    int score_h = body_h - preview_h;

    w_header  = newwin(header_h, COLS, 0, 0);
    w_dice    = newwin(dice_h, left_w, header_h, 0);
    w_log     = newwin(log_h, left_w, header_h + dice_h, 0);
    w_preview = newwin(preview_h, right_w, header_h, left_w);
    w_score   = newwin(score_h, right_w, header_h + preview_h, left_w);
    w_help    = newwin(help_h, COLS, LINES - help_h, 0);

    int dice[NUM_DICE] = {1, 1, 1, 1, 1};

    while (1) {
        int menu_choice = startup_screen();

        if (menu_choice == 2) break;
        if (menu_choice == 1) {
            show_how_to_play();
            continue;
        }

        reset_game_state();

        while (g_round <= NUM_CATEGORIES) {
            set_status("Your turn");
            int turn_result = take_player_turn(dice);

            if (g_quit_requested) break;

            if (turn_result == -2) {
                if (g_quit_requested) break;
                if (g_round == 1 && calculate_running_total(g_player_scores) == 0 &&
                    calculate_running_total(g_computer_scores) == 0) {
                    continue;
                }
                if (g_player_scores[g_round - 1] == -1 && g_round <= NUM_CATEGORIES) {
                    continue;
                }
                if (g_round == 1 || g_player_scores[g_round - 1] == -1) continue;
            }

            if (g_quit_requested) break;

            set_status("Computer turn");
            take_computer_turn(dice);

            if (g_quit_requested) break;
            g_round++;
        }

        if (g_quit_requested) break;
        final_screen();
    }

    delwin(w_help);
    delwin(w_score);
    delwin(w_preview);
    delwin(w_log);
    delwin(w_dice);
    delwin(w_header);
    endwin();
    return 0;
}
