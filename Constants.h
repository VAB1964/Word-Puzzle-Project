// In Constants.h (or equivalent file)

#pragma once
#ifndef CONSTANTS_H
#define CONSTANTS_H

// Gameplay constants
constexpr unsigned int REF_W = 1000;
constexpr unsigned int REF_H = 800;
constexpr float GUESS_DISPLAY_GAP = 40.0f;
constexpr float GUESS_DISPLAY_OFFSET_Y = 20.f;   // Extra pixels to move guess row up (increase to move up)
constexpr float GUESS_TILE_SCALE = 1.25f;       // Guess tile size multiplier (larger = bigger boxes, less cramped)
constexpr float GUESS_LETTER_FONT_SCALE = 0.65f; // Letter height as fraction of guess tile size (adjust if cramped)
constexpr float HINT_BTN_OFFSET_X = 10; // Keep as is
constexpr float HINT_BTN_OFFSET_Y = 5;  // Keep as is
constexpr float TILE_SIZE = 40.f;
constexpr float TILE_PAD = 5.f;
constexpr float GRID_TOP_MARGIN = 15.f;
constexpr float GRID_WHEEL_GAP = 75.f;       // Keep reduced
constexpr float COL_PAD = 10.f;              // Keep reduced
constexpr float GRID_COLUMN_DIVIDER_WIDTH = 4.f;  // Width of vertical divider between columns (0 = off)
constexpr int GRID_COLUMN_DIVIDER_ROWS = 5;       // Divider extends to this many rows (so full length on Easy/Medium)
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

const int HINT_COST_REVEAL_FIRST = 2; 
const int HINT_COST_REVEAL_RANDOM = 3; 
const int HINT_COST_REVEAL_LAST = 5;
const int HINT_COST_REVEAL_FIRST_OF_EACH = 7;

constexpr unsigned int WORDS_PER_HINT = 5;

const float SCORE_BAR_BOTTOM_MARGIN = 10.f;
const float METER_SCORE_GAP = 8.f;

// --- Progress Meter ---
const float PROGRESS_METER_WIDTH = 300.0f;
const float PROGRESS_METER_HEIGHT = 20.0f;
const float PROGRESS_METER_TOP_MARGIN = 15.0f;
const float PROGRESS_METER_OUTLINE = 2.0f;

// For Progress Meter
const float PROGRESS_METER_HEIGHT_DESIGN = 10.f; // Example: 20 design units high


// For Return to Menu Button in Top Bar
const float RETURN_BTN_WIDTH_DESIGN = 120.f;
const float RETURN_BTN_HEIGHT_DESIGN = 35.f; // Adjusted for typical button height
const float TOP_BAR_PADDING_X_DESIGN = 20.f;    // Example padding from left edge of top bar
const unsigned int RETURN_BTN_FONT_SIZE_DESIGN = 16; // Example font size

// Menu line-item buttons (Casual, Competitive, Quit, Easy, Medium, Hard, Return) — tweak button size only
const float MENU_BUTTON_WIDTH_DESIGN = 250.f;   // Width of each menu button (design units)
const float MENU_BUTTON_HEIGHT_DESIGN = 75.f;   // Height of each menu button (design units)

// Menu background panel (wooden framed plaque) — tweak panel size/layout separately from buttons
const float MENU_PANEL_PADDING_DESIGN = 70.f;   // Padding inside panel around title and buttons (design units)
const float MENU_BUTTON_SPACING_DESIGN = 10.f;  // Vertical gap between menu buttons (design units)
const float MENU_PANEL_EXTRA_WIDTH_DESIGN = 0.f;   // Extra width added to panel (0 = fit content; increase for wider frame)
const float MENU_PANEL_EXTRA_HEIGHT_DESIGN = 0.f;  // Extra height added to panel (0 = fit content; increase for taller frame)

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

// Hint description pop-up (hovering a hint — "Available Points", "Cost", description)
const float HINT_POPUP_WIDTH_DESIGN = 300.f;
const float HINT_POPUP_HEIGHT_DESIGN = 180.f;
const float HINT_POPUP_PADDING_DESIGN = 40.f;
const float HINT_POPUP_LINE_SPACING_DESIGN = 7.f;

// Word info / definition pop-up (hovering a solved word — word, POS, definition, sentence)
const float WORD_INFO_POPUP_MAX_WIDTH_DESIGN = 420.f;
const float WORD_INFO_POPUP_PADDING_DESIGN = 50.f;
const float WORD_INFO_POPUP_LINE_SPACING_DESIGN = 6.f;
const float WORD_INFO_POPUP_OFFSET_FROM_MOUSE_DESIGN = 12.f;  // offset from cursor

// All popups: keep this far from window edge when clamping position
const float POPUP_SCREEN_MARGIN_DESIGN = 5.f;

// Bonus words list pop-up (hovering "Bonus Words: X/Y" — list of bonus words by length)
const float POPUP_PADDING_BASE = 50.f;    // inset from frame so text sits inside the panel, not on the border
const float MAJOR_COL_SPACING_BASE = 10.f;
const float MINOR_COL_SPACING_BASE = 8.f;
const float TITLE_BOTTOM_MARGIN_BASE = 6.f; 
const float WORD_LINE_SPACING_BASE = 5.f;   
const unsigned int POPUP_WORD_FONT_SIZE_BASE = 22;   // larger base so text is readable in the large panel
const unsigned int POPUP_TITLE_FONT_SIZE_BASE = 26; 

const float POPUP_MIN_TEXT_SCALE = 0.70f;  // minimum text scale when there are many words
const float POPUP_MAX_TEXT_SCALE = 1.35f;  // allow scaling up when few words so text fills the panel       
const float POPUP_CORNER_RADIUS_BASE = 10.f;
const int MAX_MINOR_COLS_PER_GROUP = 3;

const float POPUP_MAX_WIDTH_DESIGN_RATIO = 0.95f;  // use most of grid zone width for bonus words popup
const float POPUP_MAX_HEIGHT_DESIGN_RATIO = 0.95f; // use most of grid zone height for bonus words popup
const float BONUS_POPUP_SCROLL_SPEED = 35.f;       // design units per mouse wheel tick (scrollable list)
const float POPUP_SCROLL_TOP_BUFFER = 0.f;         // extra design units above scroll area (0 = use header row height)
const float POPUP_SCROLL_BOTTOM_BUFFER = 20.f;    // design units below scroll area so text never touches bottom frame


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
const float WHEEL_LETTER_FONT_SIZE_BASE_DESIGN = 16.f;
/** Scale factor for wheel letter frames and text. Increase (e.g. 1.3f, 1.5f) to make them bigger. */
const float WHEEL_LETTER_VISUAL_SCALE = 1.8f;
constexpr float WHEEL_R = 68.f;

// --- Score Zone Constants ---
const float SCORE_ZONE_PADDING_X_DESIGN = 15.f;       
const float SCORE_ZONE_PADDING_Y_DESIGN = 26.f;       
const float SCORE_LABEL_VALUE_GAP_DESIGN = 25.f;       
const float SCORE_VALUE_BONUS_GAP_DESIGN = 10.f; 

// Font size for "Puzzle Solved!" pop-up title (design units, scaled by S()). Reduce to fit frame.
const float PUZZLE_SOLVED_TITLE_FONT_SIZE_DESIGN = 18.f;

// Font Sizes for Score Zone (in design units, will be scaled by S())
const unsigned int SCORE_ZONE_LABEL_FONT_SIZE = 18;   
const unsigned int SCORE_ZONE_VALUE_FONT_SIZE = 28;   
const unsigned int SCORE_ZONE_BONUS_FONT_SIZE = 10;   

// New Color (Bluish-Green from tubes - you'll need to fine-tune this RGB)
// Example: A bright cyan/turquoise. Adjust R,G,B to match your tube art.
const sf::Color GLOWING_TUBE_TEXT_COLOR = sf::Color(255, 190, 70); // Orange glowing (matches grid letter)
// const sf::Color GLOWING_TUBE_TEXT_COLOR = sf::Color(60, 220, 200); // Brighter variant



#endif // CONSTANTS_H