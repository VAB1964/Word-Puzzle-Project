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

constexpr float SCRAMBLE_BTN_OFFSET_X = 80.f;// Keep as is
constexpr float SCRAMBLE_BTN_OFFSET_Y = 125.f;// Keep as is
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
const int HINT_COST_REVEAL_LAST = 7;
const int HINT_COST_REVEAL_FIRST_OF_EACH = 10;

constexpr unsigned int WORDS_PER_HINT = 5;

const float SCORE_BAR_BOTTOM_MARGIN = 10.f;
const float METER_SCORE_GAP = 8.f;

// --- Progress Meter ---
const float PROGRESS_METER_WIDTH = 300.0f;
const float PROGRESS_METER_HEIGHT = 20.0f;
const float PROGRESS_METER_TOP_MARGIN = 15.0f;
const float PROGRESS_METER_OUTLINE = 1.0f;

// For Progress Meter
const float PROGRESS_METER_HEIGHT_DESIGN = 20.f; // Example: 20 design units high


// For Return to Menu Button in Top Bar
const float RETURN_BTN_WIDTH_DESIGN = 120.f;
const float RETURN_BTN_HEIGHT_DESIGN = 35.f; // Adjusted for typical button height
const float TOP_BAR_PADDING_X_DESIGN = 20.f;    // Example padding from left edge of top bar
const unsigned int RETURN_BTN_FONT_SIZE_DESIGN = 16; // Example font size


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


// DEBUG: ---- Assuming REF_W = 1000, REF_H = 800 for these example values:
const sf::FloatRect GRID_ZONE_RECT_DESIGN = { { 90.f,  122.f}, {815.f, 273.f} }; // Large central area for the word grid
const sf::FloatRect HINT_ZONE_RECT_DESIGN = { { 110.f, 475.f}, {245.f, 245.f} }; // Bottom-left area for hints
const sf::FloatRect WHEEL_ZONE_RECT_DESIGN = { {374.f, 450.f}, {285.f, 285.f} }; // Bottom-middle for letter wheel
const sf::FloatRect SCORE_ZONE_RECT_DESIGN = { {660.f, 500.f}, {243.f, 195.f} }; // Bottom-right for score/bonus
const sf::FloatRect TOP_BAR_ZONE_DESIGN = { { 50.f,  10.f}, {900.f,  30.f} }; // Area for current "Score: XXX" and "1/5"


const float HINT_BG_PADDING_X = 5;
const float HINT_BG_PADDING_Y = 2;
const float GRID_ZONE_PADDING_X_DESIGN = 2.f; 
const float GRID_ZONE_PADDING_Y_DESIGN = 2.f;
const float WHEEL_ZONE_PADDING_DESIGN = 10.f;
const float SCALED_WHEEL_PADDING_DESIGN = 15.f;
const float LETTER_VISUAL_GAP_DESIGN = 15.f;
const float LETTER_INWARD_OFFSET_DESIGN = 2.f;
const float WHEEL_BG_EXTRA_PADDING_DESIGN = 5.f;
const float WHEEL_BG_PADDING_AROUND_LETTERS_DESIGN = 3.f;

constexpr float LETTER_R = 15.f;
const float LETTER_R_BASE_DESIGN = 18.f;
const float MIN_LETTER_RADIUS_FACTOR = 0.08f;
const float MAX_LETTER_RADIUS_FACTOR = 0.20f;
const float WHEEL_LETTER_FONT_SIZE_BASE_DESIGN = 24.f;
constexpr float WHEEL_R = 68.f;

// --- Score Zone Constants ---
const float SCORE_ZONE_PADDING_X_DESIGN = 15.f;       
const float SCORE_ZONE_PADDING_Y_DESIGN = 26.f;       
const float SCORE_LABEL_VALUE_GAP_DESIGN = 10.f;       
const float SCORE_VALUE_BONUS_GAP_DESIGN = 10.f; 

// Font Sizes for Score Zone (in design units, will be scaled by S())
const unsigned int SCORE_ZONE_LABEL_FONT_SIZE = 18;   
const unsigned int SCORE_ZONE_VALUE_FONT_SIZE = 28;   
const unsigned int SCORE_ZONE_BONUS_FONT_SIZE = 10;   

// New Color (Bluish-Green from tubes - you'll need to fine-tune this RGB)
// Example: A bright cyan/turquoise. Adjust R,G,B to match your tube art.
const sf::Color GLOWING_TUBE_TEXT_COLOR = sf::Color(60, 220, 200); // Brighter variant
// const sf::Color GLOWING_TUBE_TEXT_COLOR = sf::Color(40, 180, 160); // Darker variant



#endif // CONSTANTS_H