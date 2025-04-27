#pragma once
#ifndef CONSTANTS_H
#define CONSTANTS_H

// Gameplay constants
constexpr float TILE_SIZE = 32.f;
constexpr float TILE_PAD = 5.f;
constexpr float LETTER_R = 22.f;
constexpr float WHEEL_R = 80.f;
constexpr float GRID_TOP_MARGIN = 40.f; // Used for layout logic
constexpr float GRID_WHEEL_GAP = 40.f;
constexpr float COL_PAD = 30.f;
constexpr float WHEEL_BOTTOM_MARGIN = 200.f;
constexpr float HUD_TEXT_OFFSET_Y = 60.f;
constexpr float HUD_LINE_SPACING = 30.f;
constexpr float SCRAMBLE_BTN_OFFSET_X = 40.f;
constexpr float SCRAMBLE_BTN_OFFSET_Y = 10.f;
constexpr float CONTINUE_BTN_OFFSET_Y = 80.f;
constexpr float WORD_LENGTH = 5.f; // Or 6.f or 7.f as needed
constexpr unsigned int INITIAL_HINTS = 3;
constexpr unsigned int WORDS_PER_HINT = 5;
constexpr float GRID_SIDE_MARGIN = 50.f;
constexpr float MIN_WINDOW_HEIGHT = 700.f;
constexpr float MIN_WINDOW_WIDTH = 800.f;
constexpr float SCORE_BAR_HEIGHT = 50.f;
constexpr float SCORE_BAR_TOP_MARGIN = 15.f;
constexpr float SCRAMBLE_BTN_HEIGHT = 40.f;
constexpr float HINT_BTN_HEIGHT = SCRAMBLE_BTN_HEIGHT;
const int MIN_WORD_LENGTH = 3; // Minimum word length for any game mode
const int HARD_MIN_WORD_LENGTH = 4; // Minimum word length for Hard mode

const int EASY_MAX_SOLUTIONS = 7;
const int MEDIUM_MAX_SOLUTIONS = 12;
const int HARD_MAX_SOLUTIONS = 15;
const int MIN_DESIRED_GRID_WORDS = 5;

// --- Progress Meter ---
const float PROGRESS_METER_WIDTH = 300.0f;  // Or calculate based on window width?
const float PROGRESS_METER_HEIGHT = 20.0f;
const float PROGRESS_METER_TOP_MARGIN = 15.0f; // Margin below top of window
const float PROGRESS_METER_OUTLINE = 1.0f;

// Difficulty Session Size
const int EASY_PUZZLE_COUNT = 1;   // <<< SET TO 3 FOR TESTING (was 10)
const int MEDIUM_PUZZLE_COUNT = 7;
const int HARD_PUZZLE_COUNT = 10;

#endif // CONSTANTS_H