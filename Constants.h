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
constexpr float MIN_WINDOW_HEIGHT = 550.f;
constexpr float MIN_WINDOW_WIDTH = 600.f;
constexpr float SCORE_BAR_HEIGHT = 50.f;
constexpr float SCORE_BAR_TOP_MARGIN = 15.f;
constexpr float SCRAMBLE_BTN_HEIGHT = 40.f;
constexpr float HINT_BTN_HEIGHT = SCRAMBLE_BTN_HEIGHT;

// Note: rarityBonusPoints is not constexpr by default with std::vector
// Keep its definition in Game.cpp or Game::Game() for now.
// Or use std::array<int, 4> if you want it constexpr.
// const std::vector<int> rarityBonusPoints = { 0, 1, 5, 15 }; // Keep in .cpp

#endif // CONSTANTS_H