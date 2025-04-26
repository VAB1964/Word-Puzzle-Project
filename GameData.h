#pragma once
#ifndef GAMEDATA_H
#define GAMEDATA_H

#include <SFML/System/Vector2.hpp> // For sf::Vector2f
#include <SFML/Graphics/CircleShape.hpp> // For ScoreParticleAnim particle
#include <SFML/Graphics/Text.hpp> 
#include <string>
#include <vector>

enum class AnimTarget {
    Grid,
    Score
};

//--------------------------------------------------------------------
//  Word Data Structure
//--------------------------------------------------------------------
struct WordInfo {
    std::string text;
    int rarity = 4; // Default to Very Rare if not found or error
};

//--------------------------------------------------------------------
//  Game?wide state types (Enums moved here for better organization)
//--------------------------------------------------------------------
enum class GState { Playing, Solved }; // Internal game state
enum class GameScreen { MainMenu, CasualMenu, CompetitiveMenu, Playing, GameOver }; // Overall screen state

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