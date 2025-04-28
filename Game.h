#pragma once
#ifndef GAME_H
#define GAME_H

// Include fundamental SFML headers needed for declarations
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/CircleShape.hpp> // For m_wheelBg
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/Music.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>

// Include project headers
#include "RoundedRectangleShape.h" // Must be included BEFORE it's used as a member type
#include "Theme.h"
#include "GameData.h"
#include "DecorLayer.h"
#include "Constants.h" // Include constants used in the header (like GRID_TOP_MARGIN default)

// Standard Library Headers needed for declarations
#include <vector>
#include <string>
#include <set>
#include <unordered_set> // If used in header (doesn't look like it is)
#include <optional>
#include <memory> // For std::unique_ptr

enum class DifficultyLevel {
    None, // Default state or for modes without difficulty
    Easy,
    Medium,
    Hard
};


//--------------------------------------------------------------------
//  Game Class Declaration
//--------------------------------------------------------------------
class Game {
public:
    Game(); // Constructor
    void run(); // Main game loop function

private:
    bool m_needsLayoutUpdate;
    sf::Vector2u m_lastKnownSize; // Store the last size used for layout

    std::set<std::string> m_usedBaseWordsThisSession;

    // --- Session and Difficulty State ---
    DifficultyLevel m_selectedDifficulty;
    int m_puzzlesPerSession;
    int m_currentPuzzleIndex; 
    bool m_isInSession;      

    std::vector<WordInfo> m_allPotentialSolutions; // Stores ALL sub-words before filtering
    std::set<std::string> m_foundBonusWords;      // Tracks found bonus words this puzzle

    // Core SFML Objects
    sf::RenderWindow m_window;
    sf::Font m_font;
    sf::Clock m_clock; // For delta time calculation

    // Game State
    GameScreen m_currentScreen;
    GState m_gameState; // For Playing/Solved state within game screen
    unsigned int m_hintsAvailable;
    unsigned int m_wordsSolvedSinceHint;
    unsigned int m_currentScore;
    bool m_dragging;
    std::vector<int> m_path;
    std::string m_currentGuess;

    // Word Data
    std::vector<WordInfo> m_fullWordList;
    std::vector<WordInfo> m_roots;
    std::string m_base;
    std::vector<WordInfo> m_solutions;
    std::vector<WordInfo> m_sorted;
    std::vector<std::vector<char>> m_grid;
    std::set<std::string> m_found;

    // Animations & Effects
    std::vector<LetterAnim> m_anims;
    std::vector<ScoreParticleAnim> m_scoreAnims;
	std::vector< ScoreParticleAnim> bonusAnim; // For bonus word animations
    DecorLayer m_decor;

    // --- Celebration Effect Storage ---
    std::vector<ConfettiParticle> m_confetti;
    std::vector<Balloon> m_balloons;
    float m_celebrationEffectTimer;

    // Score animation
    float m_scoreFlourishTimer; // Timer for how long the score stays big
    const float SCORE_FLOURISH_DURATION = 0.4f; // How long flourish lasts (seconds)
    const float SCORE_FLOURISH_SCALE = 1.3f;    // How much to scale score text

    // Resources (Textures, Sound Buffers - loaded once)
    sf::Texture m_scrambleTex;
    sf::Texture m_hintTex;
    sf::Texture m_sapphireTex;
    sf::Texture m_rubyTex;
    sf::Texture m_diamondTex;

    sf::SoundBuffer m_selectBuffer;
    sf::SoundBuffer m_placeBuffer;
    sf::SoundBuffer m_winBuffer;
    sf::SoundBuffer m_clickBuffer;
    sf::SoundBuffer m_hintUsedBuffer;
    sf::SoundBuffer m_errorWordBuffer;

    std::vector<std::string> m_musicFiles;
    sf::Music m_backgroundMusic;

    // --- Sounds & Music (Use unique_ptr) ---
    std::unique_ptr<sf::Sound> m_selectSound;
    std::unique_ptr<sf::Sound> m_placeSound;
    std::unique_ptr<sf::Sound> m_winSound;
    std::unique_ptr<sf::Sound> m_clickSound;
    std::unique_ptr<sf::Sound> m_hintUsedSound;
    std::unique_ptr<sf::Sound> m_errorWordSound;

    // --- Sprites (Use unique_ptr) ---
    std::unique_ptr<sf::Sprite> m_scrambleSpr;
    std::unique_ptr<sf::Sprite> m_hintSpr;
    std::unique_ptr<sf::Sprite> m_sapphireSpr;
    std::unique_ptr<sf::Sprite> m_rubySpr;
    std::unique_ptr<sf::Sprite> m_diamondSpr;

    // --- UI Elements (Shapes can stay, Text needs unique_ptr) ---
    RoundedRectangleShape m_contBtn;
    RoundedRectangleShape m_solvedOverlay;
    RoundedRectangleShape m_scoreBar;
    // Use unique_ptr
    std::unique_ptr<sf::Text> m_contTxt;
    std::unique_ptr<sf::Text> m_scoreLabelText; // Use unique_ptr
    std::unique_ptr<sf::Text> m_scoreValueText; // Use unique_ptr
    std::unique_ptr<sf::Text> m_hintCountTxt; // Use unique_ptr
    sf::CircleShape m_wheelBg;
    std::unique_ptr<sf::Text> m_guessDisplay_Text;   // Text object for the guess itself
    RoundedRectangleShape     m_guessDisplay_Bg;     // Background shape for the guess

    // Main Menu Elements
    RoundedRectangleShape m_mainMenuBg;
    RoundedRectangleShape m_casualButtonShape;
    RoundedRectangleShape m_competitiveButtonShape;
    RoundedRectangleShape m_quitButtonShape;
    std::unique_ptr<sf::Text> m_mainMenuTitle; // Use unique_ptr
	std::unique_ptr<sf::Text> m_casualMenuTitle; // Use unique_ptr
    std::unique_ptr<sf::Text> m_casualButtonText; // Use unique_ptrGame::m_updateLayer
    std::unique_ptr<sf::Text> m_competitiveButtonText; // Use unique_ptr
    std::unique_ptr<sf::Text> m_quitButtonText;
    
    // --- Casual Menu Elements ---
    RoundedRectangleShape m_casualMenuBg;
    RoundedRectangleShape m_easyButtonShape; 
    RoundedRectangleShape m_mediumButtonShape;
    RoundedRectangleShape m_hardButtonShape;
    RoundedRectangleShape m_returnButtonShape; // Can reuse shape settings
    // Use unique_ptr
    std::unique_ptr<sf::Text> m_easyButtonText;
    std::unique_ptr<sf::Text> m_mediumButtonText;
    std::unique_ptr<sf::Text> m_hardButtonText;
    std::unique_ptr<sf::Text> m_returnButtonText;

    // Layout Variables
    float m_wheelX = 0.f;
    float m_wheelY = 0.f;
    std::vector<sf::Vector2f> m_wheelCentres;
    std::vector<int>   m_wordCol;
    std::vector<int>   m_wordRow;
    std::vector<int>   m_colMaxLen;
    std::vector<float> m_colXOffset;
    float m_gridStartX = 0.f;
    float m_gridStartY = GRID_TOP_MARGIN; // Use constant for initial default
    float m_totalGridW = 0.f;

    // Themes
    std::vector<ColorTheme> m_themes;
    ColorTheme m_currentTheme;

    // Helper to get criteria based on current state (optional but cleans up rebuild)
    struct PuzzleCriteria {
        std::vector<int> allowedLengths;
        std::vector<int> allowedRarities;
    };
    PuzzleCriteria m_getCriteriaForCurrentPuzzle() const;

    // Progress Meter Elements
    sf::RectangleShape m_progressMeterBg;     // Background/border
    sf::RectangleShape m_progressMeterFill;   // The filled part showing progress
    std::unique_ptr<sf::Text> m_progressMeterText; // Optional: Text overlay "X/Y"


    // --- Private Helper Methods (Declarations only) ---
    void m_loadResources();
    void m_processEvents();
    void m_update(sf::Time dt);
    void m_render();

    void m_rebuild();
    void m_updateLayout(sf::Vector2u windowSize);
    void m_updateAnims(float dt); // Takes dt
    void m_updateScoreAnims(float dt);
    sf::Vector2f m_tilePos(int wordIdx, int charIdx);
    void m_clearDragState();

    // Screen Specific Helpers
    void m_handleMainMenuEvents(const sf::Event& event);
    void m_renderMainMenu(const sf::Vector2f& mousePos);
    void m_handleCasualMenuEvents(const sf::Event& event); 
    void m_renderCasualMenu(const sf::Vector2f& mousePos); 
    // void m_handleCompetitiveMenuEvents(const sf::Event& event); // Declare later
    // void m_renderCompetitiveMenu(const sf::Vector2f& mousePos); // Declare later
    void m_handlePlayingEvents(const sf::Event& event);
    void m_handleGameOverEvents(const sf::Event& event);
    void m_renderGameScreen(const sf::Vector2f& mousePos);

    //celebration functions
    void m_startCelebrationEffects(); 
    void m_updateCelebrationEffects(float dt); 
    void m_renderCelebrationEffects(sf::RenderTarget& target);
    void m_renderSessionComplete(const sf::Vector2f& mousePos);

    void m_handleSessionCompleteEvents(const sf::Event& event);
};

#endif // GAME_H