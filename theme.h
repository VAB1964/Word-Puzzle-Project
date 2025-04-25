#ifndef THEME_H // Start of include guard
#define THEME_H

#include <SFML/Graphics/Color.hpp>
#include <algorithm> // For std::max/min
#include <cstdint>   // For uint8_t

//--------------------------------------------------------------------
//  Color Theme Definition
//--------------------------------------------------------------------
struct ColorTheme {
    sf::Color winBg = sf::Color::Black;
    // Decor Layer (Base + 2 Accents, alpha will be randomized in spawn)
    sf::Color decorBase = sf::Color(50, 50, 50);
    sf::Color decorAccent1 = sf::Color(100, 100, 100);
    sf::Color decorAccent2 = sf::Color(150, 150, 150);
    // Grid
    sf::Color gridEmptyTile = sf::Color(200, 200, 200);
    sf::Color gridFilledTile = sf::Color(0, 100, 0);
    sf::Color gridLetter = sf::Color::White;
    // Wheel
    sf::Color wheelBg = sf::Color(180, 180, 180);
    sf::Color wheelOutline = sf::Color(80, 80, 80);
    sf::Color letterCircleNormal = sf::Color(240, 240, 240);
    sf::Color letterCircleHighlight = sf::Color(100, 180, 100);
    sf::Color letterTextNormal = sf::Color::Black;
    sf::Color letterTextHighlight = sf::Color::White;
    // Dragging
    sf::Color dragLine = sf::Color(80, 80, 200);
    // UI
    sf::Color continueButton = sf::Color(100, 200, 100);
    sf::Color hudTextGuess = sf::Color(255, 120, 0);
    sf::Color hudTextFound = sf::Color(255, 120, 0);
    sf::Color hudTextSolved = sf::Color(0, 200, 0);    // Color for "Puzzle Solved!"
    sf::Color solvedOverlayBg = sf::Color(50, 50, 70, 220); // Default overlay
    sf::Color scoreBarBg = sf::Color(50, 50, 50, 220);      // Default score bar BG
    sf::Color scoreTextLabel = sf::Color(200, 200, 200);    // Default score label
    sf::Color scoreTextValue = sf::Color::White;            // Default score value
    // Menu Colors
    sf::Color menuBg = sf::Color(40, 40, 60, 230);
    sf::Color menuTitleText = sf::Color::White;
    sf::Color menuButtonNormal = sf::Color(80, 80, 100);
    sf::Color menuButtonHover = sf::Color(110, 110, 140);
    sf::Color menuButtonText = sf::Color::White;
};

// Helper to slightly adjust color brightness (for hover effects)
// (Okay to keep definition in header as it's simple and templated-like usage)
inline sf::Color adjustColorBrightness(sf::Color color, float factor) {
    factor = std::max(0.f, factor);
    return sf::Color(
        static_cast<uint8_t>(std::min(255.f, static_cast<float>(color.r) * factor)),
        static_cast<uint8_t>(std::min(255.f, static_cast<float>(color.g) * factor)),
        static_cast<uint8_t>(std::min(255.f, static_cast<float>(color.b) * factor)),
        color.a
    );
}

#endif // THEME_H // End of include guard#pragma once
