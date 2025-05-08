// In Constants.h (or equivalent file)

#pragma once
#ifndef CONSTANTS_H
#define CONSTANTS_H

// Gameplay constants
constexpr unsigned int REF_W = 1000;
constexpr unsigned int REF_H = 800;
constexpr float GUESS_DISPLAY_GAP = 20.0f;
constexpr float HINT_BTN_OFFSET_X = 10; // Keep as is
constexpr float HINT_BTN_OFFSET_Y = 5;  // Keep as is
constexpr float TILE_SIZE = 32.f;
constexpr float TILE_PAD = 5.f;
constexpr float LETTER_R = 22.f;
constexpr float WHEEL_R = 80.f;
constexpr float GRID_TOP_MARGIN = 15.f;
constexpr float GRID_WHEEL_GAP = 75.f;       // Keep reduced
constexpr float COL_PAD = 10.f;              // Keep reduced
constexpr float WHEEL_BOTTOM_MARGIN = 70.f;  // Keep reduced

// *** Adjust these HUD values ***
const float HUD_TEXT_OFFSET_Y = 15.f;       // Increased slightly from 8.f (Gives tiny bit more gap above HUD)
const float HUD_LINE_SPACING = 20.f;        // Increased from 10.f/15.f (More space BETWEEN HUD lines)
const float HUD_AREA_MIN_HEIGHT = 25.f;     // Increased from 15.f (Reserves more space FOR HUD)
constexpr float UI_SCALE_MODIFIER = 0.75f;
constexpr float GRID_TILE_RELATIVE_SCALE_WHEN_SHRUNK = 0.95f;
// ******************************

constexpr float SCRAMBLE_BTN_OFFSET_X = 10.f;// Keep as is
constexpr float SCRAMBLE_BTN_OFFSET_Y = 10.f;// Keep as is
constexpr float CONTINUE_BTN_OFFSET_Y = 80.f;
constexpr float WORD_LENGTH = 5.f; // Or 6.f or 7.f as needed
constexpr float GRID_SIDE_MARGIN = 5.f;
constexpr float MIN_WINDOW_HEIGHT = 800.f;
constexpr float MIN_WINDOW_WIDTH = 1000.f;
constexpr float SCORE_BAR_WIDTH = 300.f;
constexpr float SCORE_BAR_HEIGHT = 30.f;
constexpr float SCORE_BAR_TOP_MARGIN = 15.f;
constexpr float SCRAMBLE_BTN_HEIGHT = 40.f;
constexpr float HINT_BTN_HEIGHT = SCRAMBLE_BTN_HEIGHT;
const int MIN_WORD_LENGTH = 3;
const int HARD_MIN_WORD_LENGTH = 4;

const int EASY_MAX_SOLUTIONS = 7;
const int MEDIUM_MAX_SOLUTIONS = 12;
const int HARD_MAX_SOLUTIONS = 15;
const int MIN_DESIRED_GRID_WORDS = 5;

const int HINT_COST_REVEAL_FIRST = 3; 
const int HINT_COST_REVEAL_RANDOM = 5; 
const int HINT_COST_REVEAL_LAST = 10;
constexpr unsigned int INITIAL_HINTS = 999;
constexpr unsigned int WORDS_PER_HINT = 5;

const float SCORE_BAR_BOTTOM_MARGIN = 10.f;
const float METER_SCORE_GAP = 8.f;

// --- Progress Meter ---
const float PROGRESS_METER_WIDTH = 300.0f;
const float PROGRESS_METER_HEIGHT = 20.0f;
const float PROGRESS_METER_TOP_MARGIN = 15.0f;
const float PROGRESS_METER_OUTLINE = 1.0f;

// Difficulty Session Size
const int EASY_PUZZLE_COUNT = 5;
const int MEDIUM_PUZZLE_COUNT = 7;
const int HARD_PUZZLE_COUNT = 10;

// Points Flourish
const float SCORE_FLOURISH_FONT_SIZE_BASE_DESIGN = 20.f; // Base font size in design units
const float SCORE_FLOURISH_LIFETIME_MIN_SEC = 1.8f;    // Minimum duration of the flourish
const float SCORE_FLOURISH_LIFETIME_MAX_SEC = 2.8f;    // Maximum duration
const float SCORE_FLOURISH_VEL_Y_MIN_DESIGN = -50.f;   // Min upward speed (design units/sec)
const float SCORE_FLOURISH_VEL_Y_MAX_DESIGN = -90.f;   // Max upward speed
const float SCORE_FLOURISH_VEL_X_RANGE_DESIGN = 20.f;    // Horizontal drift range (+/- this value)
const float HINT_POINT_ANIM_FONT_SIZE_DESIGN = 20.f;
const float HINT_POINT_TEXT_FLOURISH_DURATION = 0.5f; // Duration for "Points:" text flourish
const float HINT_POINT_ANIM_SPEED = 1.0f;

#endif // CONSTANTS_H