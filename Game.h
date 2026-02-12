//#pragma once
#ifndef GAME_H
#define GAME_H

// Include fundamental SFML headers needed for declarations

#include <SFML/Graphics.hpp>
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
#include "theme.h"
#include "ThemeData.h"
#include "GameData.h"
#include "DecorLayer.h"
#include "Constants.h" // Include constants used in the header (like GRID_TOP_MARGIN default)

// Standard Library Headers needed for declarations
#include <vector>
#include <string>
#include <set>
#include <memory> 

enum class DifficultyLevel {
    None, // Default state or for modes without difficulty
    Easy,
    Medium,
    Hard
};

// Add an enum for Hint Types
enum class HintType { RevealFirst, RevealRandom, RevealLast, RevealFirstOfEach };


//--------------------------------------------------------------------
//  Game Class Declaration
//--------------------------------------------------------------------
class Game {
public:
    // ... (public methods like Game(), run(), etc.) ...
    void m_updateView(sf::Vector2u win); // Already exists
    Game(); // Constructor
    void run(); // Main game loop function
    friend float S(const Game* g, float du);


private:

    const std::string HINT_DESC_REVEAL_FIRST = "\nThis hint will reveal the next\navailable letter.";
    const std::string HINT_DESC_REVEAL_RANDOM = "\nThis hint will reveal one\n random letter for each word.";
    const std::string HINT_DESC_REVEAL_LAST = "\nThis hint will reveal the last word\n that hasn't been revealed.";
    const std::string HINT_DESC_REVEAL_FIRST_OF_EACH = "\nThis hint will reveal the first\n unrevealed letter for every word.";

    std::unique_ptr<sf::Text> m_bonusWordsInHintZoneText;

    // --- NEW: Hint Hover Pop-up Elements ---
    int m_hoveredHintIndex;
    std::unique_ptr<sf::Sprite> m_hintPopupBgSpr;   // menu background art for hint description popup
    RoundedRectangleShape m_hintPopupBackground;
    std::unique_ptr<sf::Text> m_popupAvailablePointsText;
    std::unique_ptr<sf::Text> m_popupHintCostText;
    std::unique_ptr<sf::Text> m_popupHintDescriptionText;

    // --- Solved Word Info Pop-up Elements ---
    int m_hoveredSolvedWordIndex;
    std::unique_ptr<sf::Sprite> m_genericPopupBgSpr;  // menu background art for definition & bonus-words popups (dynamic size)
    RoundedRectangleShape m_wordInfoPopupBackground;
    std::unique_ptr<sf::Text> m_popupWordText;
    std::unique_ptr<sf::Text> m_popupPosText;
    std::unique_ptr<sf::Text> m_popupDefinitionText;
    std::unique_ptr<sf::Text> m_popupSentenceText;

    // --- Hint UI New Assets ---
    sf::Texture m_hintFrameTexture; // Texture for the individual hint frame art
    std::vector<std::unique_ptr<sf::Sprite>> m_hintFrameSprites; // Vector of 4 sprites, one for each hint frame

    sf::Texture m_hintIndicatorLightTex;
    std::vector<std::unique_ptr<sf::Sprite>> m_hintIndicatorLightSprs;

    // Hint Text elements (These are kept but will be repositioned ON the individual frames)
    std::unique_ptr<sf::Text> m_hintPointsText;
    std::unique_ptr<sf::Text> m_hintRevealFirstCostText;
    std::unique_ptr<sf::Text> m_hintRevealRandomButtonText; 
    std::unique_ptr<sf::Text> m_hintRevealRandomCostText;
    std::unique_ptr<sf::Text> m_hintRevealLastButtonText;   
    std::unique_ptr<sf::Text> m_hintRevealLastCostText;
    std::unique_ptr<sf::Text> m_hintRevealFirstButtonText;  
    std::unique_ptr<sf::Text> m_hintRevealFirstOfEachButtonText; 
    std::unique_ptr<sf::Text> m_hintRevealFirstOfEachCostText;

   
    // Clickable areas for new hint UI (will be calculated in m_updateLayout)
    std::vector<sf::FloatRect> m_hintClickableRegions; 

    // ... (rest of your private members) ...

    // Core SFML Objects
    sf::RenderWindow m_window;
    sf::Font m_font;
    sf::Clock m_clock;

    // Game State
    GameScreen m_currentScreen;
    GState m_gameState;
    int m_hintPoints;
    

    // Example of keeping existing relevant members:
    float m_letterPositionRadius;
    float m_visualBgRadius;

    struct GridLetterFlourish {
        int wordIdx;
        int charIdx;
        float timer;
    };
    std::vector<GridLetterFlourish> m_gridFlourishes;

    float m_bonusTextFlourishTimer = 0.f;

    const float GRID_FLOURISH_DURATION = 0.6f;
    const float BONUS_TEXT_FLOURISH_DURATION = 0.6f;

    bool m_debugDrawCircleMode;
    float m_currentGridLayoutScale = 1.0f;
    std::set<std::string> m_usedBaseWordsThisSession;
    std::set<std::string> m_usedLetterSetsThisSession;
    float m_uiScale = 1.f;
    bool m_needsLayoutUpdate;
    sf::Vector2u m_lastKnownSize;

    DifficultyLevel m_selectedDifficulty;
    int m_puzzlesPerSession;
    int m_currentPuzzleIndex;
    bool m_isInSession;

    std::vector<WordInfo> m_allPotentialSolutions;
    std::set<std::string> m_foundBonusWords;
    std::vector<HintPointAnimParticle> m_hintPointAnims;
    float m_hintPointsTextFlourishTimer;

    unsigned int m_hintsAvailable; // Keep if used elsewhere, otherwise points are prime
    unsigned int m_wordsSolvedSinceHint;
    unsigned int m_currentScore;

    std::unique_ptr<sf::Sprite> m_mainBackgroundSpr;
    sf::Texture m_mainBackgroundTex;

    std::vector<sf::Vector2f> m_wheelLetterRenderPos;
    float m_currentLetterRenderRadius;
    bool m_firstFrame = true;
    bool m_dragging;
    std::vector<int> m_path;
    std::string m_currentGuess;

    std::vector<WordInfo> m_fullWordList;
    std::vector<WordInfo> m_roots;
    std::string m_base;
    std::vector<WordInfo> m_solutions;
    std::vector<WordInfo> m_sorted;
    std::vector<std::vector<char>> m_grid;
    std::set<std::string> m_found;

    std::vector<LetterAnim> m_anims;
    std::vector<ScoreParticleAnim> m_scoreAnims;
    std::vector< ScoreParticleAnim> bonusAnim;
    DecorLayer m_decor;

    // --- Hint Button Click Feedback ---
    std::vector<float> m_hintFrameClickAnimTimers; 
    const float HINT_FRAME_CLICK_DURATION = 0.15f; 
    sf::Color m_hintFrameClickColor;               
    sf::Color m_hintFrameNormalColor;

    std::vector<ConfettiParticle> m_confetti;
    std::vector<Balloon> m_balloons;
    float m_celebrationEffectTimer;

    float m_scoreFlourishTimer;
    const float SCORE_FLOURISH_DURATION = 0.4f;
    const float SCORE_FLOURISH_SCALE = 1.3f;

    sf::Texture m_scrambleTex;
    sf::Texture m_sapphireTex;
    sf::Texture m_rubyTex;
    sf::Texture m_diamondTex;
    sf::Texture m_buttonTex;
    sf::Texture m_circularLetterFrameTex;

    sf::SoundBuffer m_selectBuffer;
    sf::SoundBuffer m_placeBuffer;
    sf::SoundBuffer m_winBuffer;
    sf::SoundBuffer m_clickBuffer;
    sf::SoundBuffer m_hintUsedBuffer;
    sf::SoundBuffer m_errorWordBuffer;

    std::vector<std::string> m_musicFiles;
    sf::Music m_backgroundMusic;

    std::unique_ptr<sf::Sound> m_selectSound;
    std::unique_ptr<sf::Sound> m_placeSound;
    std::unique_ptr<sf::Sound> m_winSound;
    std::unique_ptr<sf::Sound> m_clickSound;
    std::unique_ptr<sf::Sound> m_hintUsedSound;
    std::unique_ptr<sf::Sound> m_errorWordSound;

    std::unique_ptr<sf::Sprite> m_scrambleSpr;
    std::unique_ptr<sf::Sprite> m_sapphireSpr;
    std::unique_ptr<sf::Sprite> m_rubySpr;
    std::unique_ptr<sf::Sprite> m_diamondSpr;
    std::unique_ptr<sf::Sprite> m_buttonSpr;

    RoundedRectangleShape m_contBtn;
    RoundedRectangleShape m_solvedOverlay;
    RoundedRectangleShape m_scoreBar; // If this is for the "SCORE" text background keep it
    std::unique_ptr<sf::Text> m_contTxt;
    std::unique_ptr<sf::Text> m_scoreLabelText;
    std::unique_ptr<sf::Text> m_scoreValueText;
    std::unique_ptr<sf::Text> m_hintCountTxt; // This might be redundant if m_hintPointsText is used primarily
    sf::CircleShape m_wheelBg;
    std::unique_ptr<sf::Text> m_guessDisplay_Text;
    RoundedRectangleShape     m_guessDisplay_Bg;

    sf::Texture m_menuBgTexture;
    std::unique_ptr<sf::Sprite> m_mainMenuBgSpr;
    std::unique_ptr<sf::Sprite> m_casualMenuBgSpr;

    sf::Texture m_menuButtonTexture;
    std::unique_ptr<sf::Sprite> m_casualButtonSpr;
    std::unique_ptr<sf::Sprite> m_competitiveButtonSpr;
    std::unique_ptr<sf::Sprite> m_quitButtonSpr;
    std::unique_ptr<sf::Sprite> m_easyButtonSpr;
    std::unique_ptr<sf::Sprite> m_mediumButtonSpr;
    std::unique_ptr<sf::Sprite> m_hardButtonSpr;
    std::unique_ptr<sf::Sprite> m_returnButtonSpr;
    std::unique_ptr<sf::Sprite> m_returnToMenuButtonSpr;
    std::unique_ptr<sf::Sprite> m_continueButtonSpr;

    RoundedRectangleShape m_mainMenuBg;
    RoundedRectangleShape m_casualButtonShape;
    RoundedRectangleShape m_competitiveButtonShape;
    RoundedRectangleShape m_quitButtonShape;
    RoundedRectangleShape m_returnToMenuButtonShape;
    std::unique_ptr<sf::Text> m_returnToMenuButtonText;
    std::unique_ptr<sf::Text> m_mainMenuTitle;
    std::unique_ptr<sf::Text> m_casualMenuTitle;
    std::unique_ptr<sf::Text> m_casualButtonText;
    std::unique_ptr<sf::Text> m_competitiveButtonText;
    std::unique_ptr<sf::Text> m_quitButtonText;

    RoundedRectangleShape m_casualMenuBg;
    RoundedRectangleShape m_easyButtonShape;
    RoundedRectangleShape m_mediumButtonShape;
    RoundedRectangleShape m_hardButtonShape;
    RoundedRectangleShape m_returnButtonShape;
    std::unique_ptr<sf::Text> m_easyButtonText;
    std::unique_ptr<sf::Text> m_mediumButtonText;
    std::unique_ptr<sf::Text> m_hardButtonText;
    std::unique_ptr<sf::Text> m_returnButtonText;

    float m_wheelX = 0.f;
    float m_wheelY = 0.f;
    std::vector<sf::Vector2f> m_wheelCentres;
    std::vector<int>   m_wordCol;
    std::vector<int>   m_wordRow;
    std::vector<int>   m_colMaxLen;
    std::vector<float> m_colXOffset;
    float m_gridStartX = 0.f;
    float m_gridStartY = GRID_TOP_MARGIN;
    float m_totalGridW = 0.f;
    float m_currentWheelRadius;
    int tempCount = 0;

    sf::Vector2u m_lastLayoutSize;

    std::vector<ColorTheme> m_themes;
    ColorTheme m_currentTheme;

    struct PuzzleCriteria {
        std::vector<int> allowedLengths;
        std::vector<int> allowedRarities;
    };
    PuzzleCriteria m_getCriteriaForCurrentPuzzle() const;

    RoundedRectangleShape m_progressMeterBg;
    RoundedRectangleShape m_progressMeterFill;
    std::unique_ptr<sf::Text> m_progressMeterText;

    std::vector<ScoreFlourishParticle> m_scoreFlourishes;

    sf::RectangleShape m_debugGridZoneShape;
    sf::RectangleShape m_debugHintZoneShape;
    sf::RectangleShape m_debugWheelZoneShape;
    sf::RectangleShape m_debugScoreZoneShape;
    sf::RectangleShape m_debugTopBarZoneShape;
    bool m_showDebugZones;

    bool m_isHoveringHintPointsText;
    std::vector<WordInfo> m_cachedBonusWords;
    bool m_bonusWordsCacheIsValid;
    float m_bonusWordsPopupScrollOffset;   // current scroll position (design units) for bonus words popup
    float m_bonusWordsPopupMaxScrollOffset; // max scroll (set during render when content is taller than popup)

    // --- Bonus List Complete Effect ---
    bool m_bonusListCompleteEffectActive;
    float m_bonusListCompleteAnimTimer;
    float m_bonusListCompletePopupDisplayTimer; // How long the main popup stays
    int m_bonusListCompletePointsAwarded;
    sf::Text m_bonusListCompletePopupText; // For "Bonus List Complete: +XXXX"
    sf::Text m_bonusListCompleteAnimatingPointsText; // For the points flying to score
    sf::Vector2f m_bonusListCompleteAnimStartPos;
    sf::Vector2f m_bonusListCompleteAnimEndPos;

    int m_calculateTotalPossibleBonusWords() const; // Renamed for clarity
    void m_triggerBonusListCompleteEffect(int pointsAwarded);
    void m_updateBonusListCompleteEffect(float dt);
    void m_renderBonusListCompleteEffect(sf::RenderTarget& target);

    void m_renderBonusWordsPopup(sf::RenderTarget& target);
    bool isGridSolution(const std::string& wordText) const;

    void m_loadResources();
    void m_processEvents();
    void m_update(sf::Time dt);
    void m_render();

    void m_rebuild();
    void m_updateLayout(sf::Vector2u windowSize);
    void m_updateAnims(float dt);
    void m_updateScoreAnims(float dt);
    sf::Vector2f m_tilePos(int wordIdx, int charIdx);
    void m_clearDragState();

    void m_handleMainMenuEvents(const sf::Event& event);
    void m_renderMainMenu(const sf::Vector2f& mousePos);
    void m_handleCasualMenuEvents(const sf::Event& event);
    void m_renderCasualMenu(const sf::Vector2f& mousePos);
    void m_handlePlayingEvents(const sf::Event& event);
    void m_handleGameOverEvents(const sf::Event& event);
    void m_renderGameScreen(const sf::Vector2f& mousePos);
    void m_activateHint(HintType type);
    void m_checkWordCompletion(int wordIdx);

    void m_startCelebrationEffects();
    void m_updateCelebrationEffects(float dt);
    void m_renderCelebrationEffects(sf::RenderTarget& target);
    void m_renderSessionComplete(const sf::Vector2f& mousePos);

    void m_spawnScoreFlourish(int points, int wordIdxOnGrid);
    void m_updateScoreFlourishes(float dt);
    void m_renderScoreFlourishes(sf::RenderTarget& target);

    void m_handleSessionCompleteEvents(const sf::Event& event);
    void m_renderDebugCircle();

    void m_spawnHintPointAnimation(const sf::Vector2f& bonusWordTextCenterPos, int pointsAwarded);
    void m_updateHintPointAnims(float dt);
    void m_renderHintPointAnims(sf::RenderTarget& target);

    static void centerTextOnShape(sf::Text& text, const sf::Shape& shape);

};

#endif // GAME_H