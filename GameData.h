#pragma once
#ifndef GAMEDATA_H
#define GAMEDATA_H

#include <SFML/System/Vector2.hpp> // For sf::Vector2f
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/CircleShape.hpp> // For ScoreParticleAnim particle
#include <SFML/Graphics/Text.hpp> 
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

enum class AnimTarget {
    Grid,
    Score
};

struct ConfettiParticle {
    sf::RectangleShape shape; // Small rectangle for confetti
    sf::Vector2f velocity;
    float angularVelocity;
    float lifetime; // How long it lasts
    float initialLifetime; // Store initial for fading
};


struct Balloon {
    sf::CircleShape shape;
    sf::RectangleShape stringShape;
    sf::Vector2f position = { 0.f, 0.f };
    // sf::Vector2f velocity = {0.f, 0.f}; // We'll use riseSpeed and handle sway separately
    float initialX; 
    float swayAmount = 50.f;     // <<< Kept from original plan
    float swaySpeed = 1.5f;      // <<< Kept from original plan
    float swayTimer = 0.f;       // <<< Kept from original plan

    // --- ADD THESE MISSING MEMBERS ---
    float riseSpeed = -40.f;     // Speed moving upwards (negative Y)
    float swayTargetX = 0.f;     // Current target X for simpler sway (if needed, might remove)
    float timeToDisappear = 8.f; // Lifetime in seconds
    // --- END ADDED MEMBERS ---
};


//--------------------------------------------------------------------
//  Word Data Structure
//--------------------------------------------------------------------
// Structure to hold word data including rarity and pre-calculated metrics
struct WordInfo {
    std::string text = "";
    int rarity = 0;

    // --- NEW Pre-calculated Metrics ---
    float avgSubLen = 0.0f;         // Average length of sub-words (>= MIN_SUB_WORD_LEN)
    int countGE3 = 0;               // Count of sub-words with length >= 3
    int countGE4 = 0;               // Count of sub-words with length >= 4
    int countGE5 = 0;               // Count of sub-words with length >= 5
    int easyValidCount = 0;         // Count valid for Easy difficulty criteria
    int mediumValidCount = 0;       // Count valid for Medium difficulty criteria
    int hardValidCount = 0;         // Count valid for Hard difficulty criteria

    // Optional: Constructor for easier initialization if needed
    WordInfo() = default; // Keep default constructor

    // Example constructor if you want to set everything at once
    WordInfo(std::string t, int r, float avgSL = 0.0f, int c3 = 0, int c4 = 0, int c5 = 0, int ec = 0, int mc = 0, int hc = 0)
        : text(std::move(t)), rarity(r), avgSubLen(avgSL), countGE3(c3), countGE4(c4),
        countGE5(c5), easyValidCount(ec), mediumValidCount(mc), hardValidCount(hc) {
    }

};

struct ScoreFlourishParticle {
    std::string textString;     // e.g., "+40"
    sf::Vector2f position;      // Current position for rendering
    sf::Color color;            // Current color (for fading)
    // Potentially: unsigned int characterSize;
    // Potentially: sf::Text::Style style;

    sf::Vector2f velocity;
    float lifetime;
    float initialLifetime;

    // Explicit default constructor
    ScoreFlourishParticle()
        : textString(""),
        position(0.f, 0.f),
        color(sf::Color::White), // Default color
        velocity(0.f, 0.f),
        lifetime(0.f),
        initialLifetime(0.f) {
    }
};

struct HintPointAnimParticle {
    std::string textString;     // Will be "+1"
    sf::Vector2f currentPosition;
    sf::Vector2f startPosition;
    sf::Vector2f targetPosition; // Position of the "Points:" text
    sf::Color color;
    float t;                    // Animation progress (0.0 to 1.0)
    float speed;                // Speed of animation

    HintPointAnimParticle()
        : textString("+1"),
        currentPosition(0.f, 0.f),
        startPosition(0.f, 0.f),
        targetPosition(0.f, 0.f),
        color(sf::Color::Yellow), // A distinct color for hint points
        t(0.f),
        speed(0.5f) // Adjust as needed (higher is faster for t to reach 1.0)
    {
    }
};


enum class GState { Playing, Solved }; // Internal game state
enum class GameScreen { MainMenu, CasualMenu, CompetitiveMenu, Playing, GameOver, SessionComplete }; // Overall screen state

//--------------------------------------------------------------------
//  Animation Structures
//--------------------------------------------------------------------
struct LetterAnim {
    char ch = '?';
    sf::Vector2f start = { 0,0 };
    sf::Vector2f end = { 0,0 };
    float t = 0.f;
    int wordIdx = -1;   
    int charIdx = -1;   
    AnimTarget target = AnimTarget::Grid; 
};

// Keep ScoreParticleAnim as is for now, maybe remove later if unused
struct ScoreParticleAnim {
    sf::Vector2f startPos = { 0,0 };
    sf::Vector2f endPos = { 0,0 };
    sf::Text particle;
    float t = 0.f;
    float speed = 3.0f;
    int points = 0;
};

#endif // GAMEDATA_H