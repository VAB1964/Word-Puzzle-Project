
#include <utility>
#include <cstdint>
#include <fstream>
#include <sstream>

#include <string>
#include <iostream> // For error messages
#include <stdexcept> // For std::stof, std::stoi exceptions
#include <algorithm>
// <string> and <vector> were duplicated, ensure only one of each at the top level if not for specific reasons
#include <set>
#include <functional>
#include <cmath>
#include <numeric>
#include <random>
#include <ctime>
#include <memory> // For unique_ptr
#include <limits>
#include <cctype>
#include <map>

// 2. SFML Headers (Crucial: Before your project headers that use SFML)
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Window/VideoMode.hpp> // If directly used in Game.cpp, else SFML/Graphics.hpp might cover it via Window.hpp
#include <SFML/System/Time.hpp>     // If directly used in Game.cpp, else SFML/Graphics.hpp might cover it via System.hpp
#include <SFML/Window/Mouse.hpp>    // If directly used in Game.cpp

// 3. Your Project-Specific Headers
#include "theme.h"
#include "Constants.h"
#include "ThemeData.h"
#include "GameData.h" // GameData.h might use types from SFML/Graphics.hpp
#include "Game.h"     // Game.h definitely uses types from SFML/Graphics.hpp
#include "Words.h"
#include "Utils.h"
#include <vector>

//--------------------------------------------------------------------
//  Game Class Implementation
//--------------------------------------------------------------------

inline float S(const Game* g, float du) { return du * g->m_uiScale; }

// --- START: Anonymous Namespace for Helper Function ---
namespace { // Anonymous namespace for local helper
    // Helper function to get sorted, lowercase letters of a word
    std::string getCanonicalLetters(const std::string& word) {
        std::string temp = word;
        // Convert to lowercase
        std::transform(temp.begin(), temp.end(), temp.begin(),
            [](unsigned char c) { return std::tolower(c); });
        // Sort alphabetically
        std::sort(temp.begin(), temp.end());
        return temp;
    }
} // end anonymous namespace
// --- END: Anonymous Namespace for Helper Function ---

namespace { // Anonymous namespace for helper struct
    struct PopupDrawItem {
        std::string textDisplay; // Actual text to show ('word' or '***')
        bool isTitle;
        float height;
        float width;
        sf::Color color; // To handle found/unfound colors
    };
}


//--------------------------------------------------------------------
//  ★ SCALE BLOCK 2 –  view that letter‑boxes (no brace‑init lists)
//--------------------------------------------------------------------
void Game::m_updateView(sf::Vector2u ws)
{
    const float designW = static_cast<float>(REF_W);
    const float designH = static_cast<float>(REF_H);

    // create view over design‑space [0,0 – designW×designH]
    // Use the SFML 3 constructor: position vector, size vector
    sf::FloatRect initialViewRect{ {0.f, 0.f}, {designW, designH} }; // Pass {pos}, {size}
    sf::View v(initialViewRect);

    float winAspect = ws.x / static_cast<float>(ws.y);
    float designAspect = designW / designH;

    sf::FloatRect vp; // Declare the viewport rect (position/size members exist)

    if (winAspect > designAspect)
    {
        // Window is wider – pillar‑box
        // Calculate scale FIRST
        float scale = designAspect / winAspect;
        // Assign members using SFML 3 API: .position.x, .position.y, .size.x, .size.y
        vp.position.x = (1.f - scale) * 0.5f;
        vp.position.y = 0.f;
        vp.size.x = scale;
        vp.size.y = 1.f;
    }
    else
    {
        // Window is taller – letter‑box
        // Calculate scale FIRST
        float scale = winAspect / designAspect;
        // Assign members using SFML 3 API: .position.x, .position.y, .size.x, .size.y
        vp.position.x = 0.f;
        vp.position.y = (1.f - scale) * 0.5f;
        vp.size.x = 1.f;
        vp.size.y = scale;
    }

    v.setViewport(vp);
    m_window.setView(v);
}

Game::Game() :
    // --- Initialize members in the initializer list ---
    m_window(),                              // Default construct window
    m_font(),                                // Default construct font (will be loaded)
    m_clock(),
    m_lastLayoutSize({ 0, 0 }),
    m_celebrationEffectTimer(0.f),
    m_currentScreen(GameScreen::MainMenu),
    m_gameState(GState::Playing),         
    m_wordsSolvedSinceHint(0),
    m_currentScore(0),
    m_scoreFlourishTimer(0.f),
    m_bonusTextFlourishTimer(0.f),
    m_dragging(false),
    m_decor(10),                             
    m_selectedDifficulty(DifficultyLevel::None),
    m_puzzlesPerSession(0),
    m_currentPuzzleIndex(0),
    m_isInSession(false),
    m_uiScale(1.f),
    m_hintPoints(999),
    m_scoreFlourishes(),
    m_hintPointAnims(), 
    m_hintPointsTextFlourishTimer(0.f),

    // Resource Handles (default construct textures/buffers - loaded later)
    m_scrambleTex(), m_sapphireTex(), m_rubyTex(), m_diamondTex(),
    m_selectBuffer(), m_placeBuffer(), m_winBuffer(), m_clickBuffer(), m_hintUsedBuffer(), m_errorWordBuffer(),
    // Sounds (will be unique_ptr, initialize to nullptr or default construct)
    m_selectSound(nullptr), m_placeSound(nullptr), m_winSound(nullptr), m_clickSound(nullptr), m_hintUsedSound(nullptr), m_errorWordSound(nullptr),
    // Music (default construct - file loaded later)
    m_backgroundMusic(),
    // Sprites (will be unique_ptr, initialize to nullptr or default construct)
    m_scrambleSpr(nullptr), m_sapphireSpr(nullptr), m_rubySpr(nullptr), m_diamondSpr(nullptr),
    // Texts (will be unique_ptr, initialize to nullptr or default construct)
    m_contTxt(nullptr), m_scoreLabelText(nullptr), m_scoreValueText(nullptr), m_hintCountTxt(nullptr),
    m_mainMenuTitle(nullptr), m_casualButtonText(nullptr), m_competitiveButtonText(nullptr), m_quitButtonText(nullptr),
    m_casualMenuTitle(nullptr), m_easyButtonText(nullptr), m_mediumButtonText(nullptr), m_hardButtonText(nullptr), m_returnButtonText(nullptr),
    m_guessDisplay_Text(nullptr),
    m_progressMeterText(nullptr),
    m_mainBackgroundSpr(nullptr),
    m_returnToMenuButtonText(nullptr), // Added for return button
    // UI Shapes (can use constructor directly)
    m_contBtn({ 200.f, 50.f }, 10.f, 10),
    m_solvedOverlay({ 100.f, 50.f }, 10.f, 10),
    m_scoreBar({ 100.f, 30.f }, 10.f, 10),
    m_guessDisplay_Bg({ 50.f, 30.f }, 5.f, 10),
    m_debugDrawCircleMode(false),
    m_needsLayoutUpdate(false),
    m_lastKnownSize(0, 0),
    m_mainMenuBg({ 300.f, 300.f }, 15.f, 10),
    m_casualButtonShape({ 250.f, 50.f }, 10.f, 10),
    m_competitiveButtonShape({ 250.f, 50.f }, 10.f, 10),
    m_quitButtonShape({ 250.f, 50.f }, 10.f, 10),
    m_casualMenuBg({ 300.f, 400.f }, 15.f, 10),
    m_easyButtonShape({ 250.f, 50.f }, 10.f, 10),
    m_mediumButtonShape({ 250.f, 50.f }, 10.f, 10),
    m_hardButtonShape({ 250.f, 50.f }, 10.f, 10),
    m_returnButtonShape({ 250.f, 50.f }, 10.f, 10),
    m_progressMeterBg({ 100.f, 20.f }),
    m_progressMeterFill({ 100.f, 20.f }),
    m_returnToMenuButtonShape({ 100.f, 40.f }, 8.f, 10), 
    m_hintRevealRandomButtonShape({ 100.f, 30.f }, 5.f, 10),
    m_hintRevealLastButtonShape({ 100.f, 30.f }, 5.f, 10),
    m_hintRevealFirstButtonShape({ 100.f, 30.f }, 5.f, 10), 
    m_hintRevealFirstOfEachButtonShape({ 100.f, 30.f }, 5.f, 10),
    m_hintAreaBg({ 100.f, 200.f }, 10.f, 10),
    m_isHoveringHintPointsText(false),
    m_bonusWordsCacheIsValid(false),
    //debug
    m_showDebugZones(false),

    m_firstFrame(true) 
{ // --- Constructor Body Starts Here ---

    //----------------------------------------------------------------
    // ★★★ REVERTED: Create window at desired size, capped by desktop.
    //----------------------------------------------------------------
    const sf::Vector2u desiredInitialSize{ 1000u, 800u }; // Your original desired size
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();

    // Calculate initial size, ensuring it's not larger than the desktop
    unsigned int initialWidth = std::min(desiredInitialSize.x, desktop.size.x);
    unsigned int initialHeight = std::min(desiredInitialSize.y, desktop.size.y);

    // Create the window with the calculated initial size and default style
    m_window.create(sf::VideoMode({ initialWidth, initialHeight }),
        "Word Puzzle",
        sf::Style::Default); // Or Fullscreen if you prefer
    m_window.setFramerateLimit(60);
    m_window.setVerticalSyncEnabled(true);

    m_loadResources(); // Load resources

    // --- Resource Setup (Sounds, Sprites) ---
    // (Sounds)
    if (m_selectBuffer.getSampleCount() > 0) m_selectSound = std::make_unique<sf::Sound>(m_selectBuffer);
    if (m_placeBuffer.getSampleCount() > 0) m_placeSound = std::make_unique<sf::Sound>(m_placeBuffer);
    if (m_winBuffer.getSampleCount() > 0) m_winSound = std::make_unique<sf::Sound>(m_winBuffer);
    if (m_clickBuffer.getSampleCount() > 0) m_clickSound = std::make_unique<sf::Sound>(m_clickBuffer);
    if (m_hintUsedBuffer.getSampleCount() > 0) m_hintUsedSound = std::make_unique<sf::Sound>(m_hintUsedBuffer);
    if (m_errorWordBuffer.getSampleCount() > 0) m_errorWordSound = std::make_unique<sf::Sound>(m_errorWordBuffer);
    // (Sprites)
    if (m_scrambleTex.getSize().x > 0) m_scrambleSpr = std::make_unique<sf::Sprite>(m_scrambleTex);
    if (m_sapphireTex.getSize().x > 0) m_sapphireSpr = std::make_unique<sf::Sprite>(m_sapphireTex);
    if (m_rubyTex.getSize().x > 0) m_rubySpr = std::make_unique<sf::Sprite>(m_rubyTex);
    if (m_diamondTex.getSize().x > 0) m_diamondSpr = std::make_unique<sf::Sprite>(m_diamondTex);

    // --- Set initial sprite properties ---
    if (m_contTxt) m_contTxt->setFillColor(sf::Color::White);
    // (Gems)
    if (m_sapphireSpr && m_sapphireTex.getSize().y > 0) {
        float desiredGemHeight = TILE_SIZE * 0.60f; float gemScale = desiredGemHeight / m_sapphireTex.getSize().y;
        m_sapphireSpr->setScale({ gemScale, gemScale });
        m_sapphireSpr->setOrigin({ m_sapphireTex.getSize().x / 2.f, m_sapphireTex.getSize().y / 2.f });
    }
    if (m_rubySpr && m_rubyTex.getSize().y > 0) {
        float desiredGemHeight = TILE_SIZE * 0.60f; float gemScale = desiredGemHeight / m_rubyTex.getSize().y;
        m_rubySpr->setScale({ gemScale, gemScale });
        m_rubySpr->setOrigin({ m_rubyTex.getSize().x / 2.f, m_rubyTex.getSize().y / 2.f });
    }
    if (m_diamondSpr && m_diamondTex.getSize().y > 0) {
        float desiredGemHeight = TILE_SIZE * 0.60f; float gemScale = desiredGemHeight / m_diamondTex.getSize().y;
        m_diamondSpr->setScale({ gemScale, gemScale });
        m_diamondSpr->setOrigin({ m_diamondTex.getSize().x / 2.f, m_diamondTex.getSize().y / 2.f });
    }
    // (Buttons)
    if (m_scrambleSpr && m_scrambleTex.getSize().y > 0) {
        float scrambleScale = SCRAMBLE_BTN_HEIGHT / static_cast<float>(m_scrambleTex.getSize().y);
        m_scrambleSpr->setScale({ scrambleScale, scrambleScale });
    }

    //DEBUG:
    sf::Color debugColor = sf::Color(255, 0, 0, 100); // Semi-transparent red
    float outlineThickness = 5.0f; // In design units (will be scaled by m_uiScale via S())

    m_debugGridZoneShape.setFillColor(sf::Color::Transparent);
    m_debugGridZoneShape.setOutlineColor(debugColor);
   

    m_debugHintZoneShape.setFillColor(sf::Color::Transparent);
    m_debugHintZoneShape.setOutlineColor(debugColor);

    m_debugWheelZoneShape.setFillColor(sf::Color::Transparent);
    m_debugWheelZoneShape.setOutlineColor(debugColor);

    m_debugScoreZoneShape.setFillColor(sf::Color::Transparent);
    m_debugScoreZoneShape.setOutlineColor(debugColor);

    m_debugTopBarZoneShape.setFillColor(sf::Color::Transparent);
    m_debugTopBarZoneShape.setOutlineColor(debugColor);


    // --- Initial Setup Order ---
    // 1. Set the view based on the NEW window size.
    m_updateView(m_window.getSize());

    // 2. Rebuild game state (includes an internal call to m_updateLayout).
    m_rebuild();

    // 3. ★★★ ADDED: Explicitly update layout AGAIN after rebuild. ★★★
    // This ensures layout calculations use the finalized view and window state.
    m_updateLayout(m_window.getSize());

}

// --- Main Game Loop ---
    void Game::run() {
        while (m_window.isOpen()) {

            if (m_firstFrame) {
                std::cout << "DEBUG: Performing first frame initialization..." << std::endl;

                // 1. Set the view
                m_updateView(m_window.getSize());

                m_window.clear(m_currentTheme.winBg); // Clear with background color
                m_decor.draw(m_window);
                m_window.display();
                // --- END CLEAR/DISPLAY CYCLE ---

                // 2. Rebuild game state (includes an internal call to m_updateLayout)
                m_rebuild();

                // 3. Explicitly update layout AGAIN to be sure.
                m_updateLayout(m_window.getSize());

                m_firstFrame = false;
                std::cout << "DEBUG: First frame initialization complete." << std::endl;
            }

            // --- Existing game loop code ---
            sf::Time dt = m_clock.restart();
            if (dt.asSeconds() > 0.1f) dt = sf::seconds(0.1f); // Clamp dt

            m_processEvents();
            m_update(dt);
            m_render();
        }
    }

void Game::m_updateAnims(float dt)
{
    m_anims.erase(std::remove_if(m_anims.begin(), m_anims.end(),
        [&](LetterAnim& a) {
            a.t += dt * 3.0f; // Adjust speed if needed
            if (a.t >= 1.f) {
                a.t = 1.f; // Clamp

                // --- Action on Completion ---
                if (a.target == AnimTarget::Grid) {
                    if (a.wordIdx >= 0 && static_cast<size_t>(a.wordIdx) < m_grid.size() &&
                        a.charIdx >= 0 && static_cast<size_t>(a.charIdx) < m_grid[a.wordIdx].size())
                    {
                        // The double assignment was in your original code, likely a harmless typo
                        // m_grid[a.wordIdx][a.charIdx] = a.ch; 
                        m_grid[a.wordIdx][a.charIdx] = a.ch; // This updates the grid data model

                        std::cout << "DEBUG: Anim to grid: m_grid[" << a.wordIdx << "][" << a.charIdx << "] = " << a.ch << std::endl;
                        m_checkWordCompletion(a.wordIdx);
                    }
                    else {
                        std::cerr << "ERROR: Anim completion - word/char index out of bounds for grid. "
                            << "wordIdx=" << a.wordIdx << " (grid size=" << m_grid.size() << "), "
                            << "charIdx=" << a.charIdx;
                        if (static_cast<size_t>(a.wordIdx) < m_grid.size()) {
                            std::cerr << " (grid word size=" << m_grid[a.wordIdx].size() << ")";
                        }
                        std::cerr << std::endl;
                    }
                    if (m_placeSound) m_placeSound->play();
                }
                else if (a.target == AnimTarget::Score) {
                    bool isLastOfBatch = true;
                    for (const auto& other_a : m_anims) {
                        if (&other_a != &a && other_a.target == AnimTarget::Score && other_a.t < 1.0f) {
                            isLastOfBatch = false;
                            break;
                        }
                    }
                    if (isLastOfBatch) {
                        m_scoreFlourishTimer = SCORE_FLOURISH_DURATION;
                        if (m_placeSound) m_placeSound->play();
                        std::cout << "DEBUG: Score flourish triggered." << std::endl;
                    }
                }
                // --- End Action ---

                return true; // Mark for removal
            }
            return false; // Keep animation active
        }),
        m_anims.end());
}


// Remove 'static' and add 'Game::' prefix
void Game::m_updateScoreAnims(float dt) // No longer need to pass RenderTarget
{
    m_scoreAnims.erase(std::remove_if(m_scoreAnims.begin(), m_scoreAnims.end(),
        [&](ScoreParticleAnim& a) { // Capture 'this' implicitly or explicitly [&, this]
            a.t += dt * a.speed;
            if (a.t >= 1.f) {
                a.t = 1.f;
                // TODO: Update score text logic here
                return true;
            }

            sf::Vector2f p = a.startPos + (a.endPos - a.startPos) * a.t;
            a.particle.setPosition(p);
            //m_window.draw(a.particle); // Use m_window

            return false;
        }),
        m_scoreAnims.end());
}

// --- Resource Loading ---
void Game::m_loadResources() {
    // --- Font ---
    if (!m_font.openFromFile("fonts/arialbd.ttf")) {
        if (!m_font.openFromFile("E:/UdemyCoursesProjects/WordPuzzle/SFML_TestProject/fonts/arialbd.ttf")) {
            std::cerr << "FATAL Error loading font. Exiting.\n"; exit(1);
        }
    }
    // --- Textures ---
    
    // --- Load New Main Background Texture ---
    if (!m_mainBackgroundTex.loadFromFile("assets/BackgroundandFrame.png")) { 
        std::cerr << "CRITICAL ERROR: Could not load main background texture!" << std::endl;
        exit(1); 
    }
    m_mainBackgroundTex.setSmooth(true); // Optional, but good for detailed art if scaled    

    // Load scramble button texture (Example - adjust path as needed)
    if (!m_scrambleTex.loadFromFile("assets/scramble.png")) {
        std::cerr << "Error loading scramble texture!" << std::endl;
        // Consider exiting or using a fallback visual
    }
    else {
        m_scrambleTex.setSmooth(true);
    }
    // Return to menu button
    m_returnToMenuButtonText = std::make_unique<sf::Text>(m_font, "Menu", 20);

	//Progess Meter Elements
    m_progressMeterText = std::make_unique<sf::Text>(m_font, "", 16); // Smaller font size?
    m_progressMeterText->setFillColor(sf::Color::White); // Default color

    m_guessDisplay_Text = std::make_unique<sf::Text>(m_font, "", 30); 
    m_guessDisplay_Text->setFillColor(sf::Color::White); 

    // *** ADD GEM TEXTURE LOADING HERE ***
    // Make sure these paths are correct relative to your executable
    // or use absolute paths if necessary.
    if (!m_sapphireTex.loadFromFile("assets/emerald.png")) { // <-- ADJUST PATH
        std::cerr << "Error loading sapphire texture (assets/emerald.png)!" << std::endl;
        // Handle error (exit, default color, etc.)
    }
    else {
        m_sapphireTex.setSmooth(true); // Optional: Smooth scaling
    }

    if (!m_rubyTex.loadFromFile("assets/ruby.png")) { // <-- ADJUST PATH
        std::cerr << "Error loading ruby texture (assets/ruby.png)!" << std::endl;
        // Handle error
    }
    else {
        m_rubyTex.setSmooth(true);
    }

    if (!m_diamondTex.loadFromFile("assets/diamond.png")) { // <-- ADJUST PATH
        std::cerr << "Error loading diamond texture (assets/diamond.png)!" << std::endl;
        // Handle error
    }
    else {
        m_diamondTex.setSmooth(true);
    }
    // *** END OF GEM TEXTURE LOADING ***

    // --- Sound Buffers ---
    // Load directly into MEMBER buffers
    bool selectLoaded = m_selectBuffer.loadFromFile("assets/sounds/select_letter.wav");
    if (!selectLoaded) { std::cerr << "Error loading select_letter sound\n"; }

    bool placeLoaded = m_placeBuffer.loadFromFile("assets/sounds/place_letter.wav");
    if (!placeLoaded) { std::cerr << "Error loading place_letter sound\n"; }

    bool winLoaded = m_winBuffer.loadFromFile("assets/sounds/puzzle_solved.wav");
    if (!winLoaded) { std::cerr << "Error loading puzzle_solved sound\n"; }

    bool clickLoaded = m_clickBuffer.loadFromFile("assets/sounds/button_click.wav");
    if (!clickLoaded) { std::cerr << "Error loading button_click sound\n"; }

    bool hintUsedLoaded = m_hintUsedBuffer.loadFromFile("assets/sounds/button_click.wav"); 
    if (!hintUsedLoaded) { std::cerr << "Error loading hint_used sound\n"; }

    bool errorWordLoaded = m_errorWordBuffer.loadFromFile("assets/sounds/hint_used.mp3");
    if (!errorWordLoaded) { std::cerr << "Error loading hint_used sound\n"; }

    
    // --- Create Sounds (Link Buffers) ---
    // Only create sound object IF the corresponding MEMBER buffer loaded successfully
    if (selectLoaded) { m_selectSound = std::make_unique<sf::Sound>(m_selectBuffer); }
    if (placeLoaded) { m_placeSound = std::make_unique<sf::Sound>(m_placeBuffer); }
    if (winLoaded) { m_winSound = std::make_unique<sf::Sound>(m_winBuffer); }
    if (clickLoaded) { m_clickSound = std::make_unique<sf::Sound>(m_clickBuffer); }
    if (hintUsedLoaded) { m_hintUsedSound = std::make_unique<sf::Sound>(m_hintUsedBuffer); }
    if (errorWordLoaded) { m_errorWordSound = std::make_unique<sf::Sound>(m_errorWordBuffer); }
    // Note: If a buffer fails to load, the corresponding unique_ptr remains nullptr


    // --- Create Sprites (Link Textures) ---
    m_scrambleSpr = std::make_unique<sf::Sprite>(m_scrambleTex);
    m_sapphireSpr = std::make_unique<sf::Sprite>(m_sapphireTex);
    m_rubySpr = std::make_unique<sf::Sprite>(m_rubyTex);
    m_diamondSpr = std::make_unique<sf::Sprite>(m_diamondTex);

    // --- Create and set up the main background sprite ---
    m_mainBackgroundSpr = std::make_unique<sf::Sprite>(m_mainBackgroundTex);
    if (m_mainBackgroundSpr) {
        m_mainBackgroundSpr->setTexture(m_mainBackgroundTex);
        m_mainBackgroundSpr->setPosition({ 0.f, 0.f });
    }
    else {
        std::cerr << "ERROR: Failed to create m_mainBackgroundSpr unique_ptr!" << std::endl;
        exit(1); // Critical error
    }


    // --- Create Text Objects (Link Font, Set Properties) --- *** MOVED HERE ***
    m_contTxt = std::make_unique<sf::Text>(m_font, "Continue", 24);
    m_scoreLabelText = std::make_unique<sf::Text>(m_font, "SCORE:", SCORE_ZONE_LABEL_FONT_SIZE);
    m_scoreValueText = std::make_unique<sf::Text>(m_font, "0", SCORE_ZONE_VALUE_FONT_SIZE);
    m_hintCountTxt = std::make_unique<sf::Text>(m_font, "", 20);
    m_mainMenuTitle = std::make_unique<sf::Text>(m_font, "Main Menu", 36);
    m_casualButtonText = std::make_unique<sf::Text>(m_font, "Casual", 24);
    m_competitiveButtonText = std::make_unique<sf::Text>(m_font, "Competitive", 24);
    m_quitButtonText = std::make_unique<sf::Text>(m_font, "Quit", 24);

    m_hintRevealFirstButtonText = std::make_unique<sf::Text>(m_font, "Letter", 18); // Set text to "Letter"
    m_hintRevealFirstButtonText->setFillColor(sf::Color::White);
    m_hintPointsText = std::make_unique<sf::Text>(m_font, "Points: 0", 20); // Initial text
    m_hintPointsText->setFillColor(sf::Color::White); // Or use theme color

    // Cost text for the FIRST hint (using existing sprite button)
    m_hintRevealFirstCostText = std::make_unique<sf::Text>(m_font, "Cost: " + std::to_string(HINT_COST_REVEAL_FIRST), 16);
    m_hintRevealFirstCostText->setFillColor(sf::Color::White); // Or theme color

    // Button text and cost text for RANDOM hint
    m_hintRevealRandomButtonText = std::make_unique<sf::Text>(m_font, "Random", 18);
    m_hintRevealRandomButtonText->setFillColor(sf::Color::White);
    m_hintRevealRandomCostText = std::make_unique<sf::Text>(m_font, "Cost: " + std::to_string(HINT_COST_REVEAL_RANDOM), 16);
    m_hintRevealRandomCostText->setFillColor(sf::Color::White);

    // Button text and cost text for LAST WORD hint
    m_hintRevealLastButtonText = std::make_unique<sf::Text>(m_font, "Full Word", 18);
    m_hintRevealLastButtonText->setFillColor(sf::Color::White);
    m_hintRevealLastCostText = std::make_unique<sf::Text>(m_font, "Cost: " + std::to_string(HINT_COST_REVEAL_LAST), 16);
    m_hintRevealLastCostText->setFillColor(sf::Color::White);

    // Button text and cost text for FIRST OF EACH hint
    m_hintRevealFirstOfEachButtonText = std::make_unique<sf::Text>(m_font, "1st of Each", 18); // Shorter label
    m_hintRevealFirstOfEachButtonText->setFillColor(sf::Color::White);
    m_hintRevealFirstOfEachCostText = std::make_unique<sf::Text>(m_font, "Cost: " + std::to_string(HINT_COST_REVEAL_FIRST_OF_EACH), 16);
    m_hintRevealFirstOfEachCostText->setFillColor(sf::Color::White);

    // Create Casual Menu Text
    m_casualMenuTitle = std::make_unique<sf::Text>(m_font, "Casual", 36);
    m_easyButtonText = std::make_unique<sf::Text>(m_font, "Easy", 24);
    m_mediumButtonText = std::make_unique<sf::Text>(m_font, "Medium", 24);
    m_hardButtonText = std::make_unique<sf::Text>(m_font, "Hard", 24);
    m_returnButtonText = std::make_unique<sf::Text>(m_font, "Return", 24);

    // Set initial text colors (or do it based on theme later)
    m_contTxt->setFillColor(sf::Color::White);
    // Add ->setFillColor for other text elements if needed

    // --- Set initial Sprite properties ---
    float desiredGemHeight = TILE_SIZE * 0.60f; float gemScale = desiredGemHeight / m_sapphireTex.getSize().y;
    m_sapphireSpr->setScale({ gemScale, gemScale }); m_rubySpr->setScale({ gemScale, gemScale }); m_diamondSpr->setScale({ gemScale, gemScale });
    m_sapphireSpr->setOrigin({ m_sapphireTex.getSize().x / 2.f, m_sapphireTex.getSize().y / 2.f });
    m_rubySpr->setOrigin({ m_rubyTex.getSize().x / 2.f, m_rubyTex.getSize().y / 2.f });
    m_diamondSpr->setOrigin({ m_diamondTex.getSize().x / 2.f, m_diamondTex.getSize().y / 2.f });
    const float scrambleBtnHeight = SCRAMBLE_BTN_HEIGHT; // Use constant
    const float hintBtnHeight = HINT_BTN_HEIGHT;         // Use constant
    float scrambleScale = scrambleBtnHeight / static_cast<float>(m_scrambleTex.getSize().y); m_scrambleSpr->setScale({ scrambleScale, scrambleScale });



    // --- Load Music Files List ---
    m_musicFiles = { "assets/music/track1.mp3", "assets/music/track2.mp3", "assets/music/track3.mp3", "assets/music/track4.mp3", "assets/music/track5.mp3" };
    m_backgroundMusic.setVolume(40.f);



    // --- Load Word List ---
    m_fullWordList = Words::loadProcessedWordList("words_processed.csv");
    if (m_fullWordList.empty()) { std::cerr << "Failed to load word list or list is empty. Exiting." << std::endl; exit(1); }
    m_roots.clear(); // Start with an empty list
    std::vector<int> potentialBaseLengths = { 4, 5, 6, 7 }; // Define the lengths we might need

    for (int len : potentialBaseLengths) {
        std::vector<WordInfo> wordsOfLength = Words::withLength(m_fullWordList, len);
        // Append these words to the main m_roots list
        m_roots.insert(m_roots.end(), wordsOfLength.begin(), wordsOfLength.end());
    }
    if (m_roots.empty()) { std::cerr << "No suitable root words found in list. Exiting." << std::endl; exit(1); }
    std::cout << "DEBUG: Populated m_roots with " << m_roots.size() << " potential base words (lengths 4-7)." << std::endl;
    // --- End m_roots creation ---

    // --- Load Color Themes ---
    m_themes.clear();
    m_themes = loadThemes();
    //m_themes[0] = m_currentTheme;

    // Check if loading failed or returned empty
    if (m_themes.empty()) {
        std::cerr << "CRITICAL Warning: loadThemes() returned empty vector. Using fallback default theme.\n";
        m_themes.push_back({}); // Add a default-constructed theme as a fallback
    }
    if (m_returnToMenuButtonText) m_returnToMenuButtonText->setFillColor(sf::Color::White);

    

}


// --- Process Events (Placeholder) ---
void Game::m_processEvents()
{
    while (const std::optional evOpt = m_window.pollEvent())
    {
        const sf::Event& ev = *evOpt;

        if (ev.is<sf::Event::Closed>())
        {
            m_window.close();
            return;
        }
        else if (const auto* rs = ev.getIf<sf::Event::Resized>())
        {
            sf::Vector2u newSize{ rs->size.x, rs->size.y };
            m_updateView(newSize);     // <--- THIS IS THE CRUCIAL CALL
            m_updateLayout(newSize); // Layout isn't needed for the simple circle test
            continue;                 // no further per‑screen handling
        }

        //---------------- existing per‑screen event handling ----------
        if (m_currentScreen == GameScreen::MainMenu)          m_handleMainMenuEvents(ev);
        else if (m_currentScreen == GameScreen::CasualMenu)   m_handleCasualMenuEvents(ev);
        else if (m_currentScreen == GameScreen::Playing)      m_handlePlayingEvents(ev);
        else if (m_currentScreen == GameScreen::GameOver)     m_handleGameOverEvents(ev);
        else if (m_currentScreen == GameScreen::SessionComplete) m_handleSessionCompleteEvents(ev);
    } // --- End while pollEvent ---

    // --- Post-Event Updates ---

    // Update layout IF needed, using the last known valid size
    if (m_needsLayoutUpdate) {
        std::cout << "DEBUG: Updating layout based on final size " << m_lastKnownSize.x << "x" << m_lastKnownSize.y << std::endl;
        // View should have already been set correctly when m_needsLayoutUpdate was set true
        m_updateLayout(m_lastKnownSize);
        m_needsLayoutUpdate = false; // Reset the flag
    }
}

// --- Update (Placeholder) ---
void Game::m_update(sf::Time dt) {
    float deltaSeconds = dt.asSeconds();
    m_decor.update(deltaSeconds, m_window.getSize(), m_currentTheme);

    // --- Update Score Flourish Timer ---
    if (m_scoreFlourishTimer > 0.f) {
        m_scoreFlourishTimer -= deltaSeconds;
        if (m_scoreFlourishTimer < 0.f) m_scoreFlourishTimer = 0.f;
    }

    // --- Update Bonus Text Flourish Timer --- // <<< NEW
    if (m_bonusTextFlourishTimer > 0.f) {
        m_bonusTextFlourishTimer -= deltaSeconds;
        if (m_bonusTextFlourishTimer < 0.f) m_bonusTextFlourishTimer = 0.f;
    }

    // --- Update Grid Letter Flourish Timers --- // <<< NEW
    m_gridFlourishes.erase(
        std::remove_if(m_gridFlourishes.begin(), m_gridFlourishes.end(),
            [deltaSeconds](GridLetterFlourish& f) {
                f.timer -= deltaSeconds;
                return f.timer <= 0.f; // Remove if timer expired
            }),
        m_gridFlourishes.end()
    );
    // -------------------------------------------

    // Update game elements based on screen
    if (m_currentScreen == GameScreen::Playing || m_currentScreen == GameScreen::GameOver) {
        m_updateAnims(deltaSeconds);
        m_updateScoreFlourishes(deltaSeconds);
        m_updateHintPointAnims(deltaSeconds);
    }
    else if (m_currentScreen == GameScreen::SessionComplete) {
        m_updateCelebrationEffects(deltaSeconds);
    }
    // (No need to call m_updateScoreAnims separately if it's part of m_updateAnims or not used)
}

// --- Render ---
void Game::m_render() {
    m_window.clear(m_currentTheme.winBg);

    // --- DRAW NEW MAIN BACKGROUND SPRITE FIRST ---
    if (m_mainBackgroundSpr && m_mainBackgroundTex.getSize().x > 0) { 
        m_window.draw(*m_mainBackgroundSpr); 
    }
    // --- END DRAW NEW MAIN BACKGROUND ---


    m_decor.draw(m_window); // Draw background decor first

    // Get mouse position once for hover checks within render helpers
    sf::Vector2f mpos = m_window.mapPixelToCoords(sf::Mouse::getPosition(m_window));

    // --- Draw based on current screen ---
    if (m_debugDrawCircleMode) {
        m_renderDebugCircle(); // Call the debug function
    }
    else
    {
        if (m_currentScreen == GameScreen::MainMenu) { m_renderMainMenu(mpos); }
        else if (m_currentScreen == GameScreen::CasualMenu) { m_renderCasualMenu(mpos); }
        else if (m_currentScreen == GameScreen::SessionComplete) { m_renderSessionComplete(mpos); }
        else { m_renderGameScreen(mpos); }
    }

    // --- Draw Bonus Words Popup (if hovering over hint points text) ---
    // Must be after m_renderGameScreen has potentially set m_isHoveringHintPointsText true
    if (m_isHoveringHintPointsText && (m_currentScreen == GameScreen::Playing || m_currentScreen == GameScreen::GameOver)) {
        m_renderBonusWordsPopup(m_window); // Pass m_window as the RenderTarget
    }

    // --- Draw Debug Zones (draw them last to see on top of everything) ---
    if (m_showDebugZones) {
        m_window.draw(m_debugGridZoneShape);
        m_window.draw(m_debugHintZoneShape);
        m_window.draw(m_debugWheelZoneShape);
        m_window.draw(m_debugScoreZoneShape);
        m_window.draw(m_debugTopBarZoneShape);
    }


    // TODO: Draw Pop-ups last if needed

    m_window.display(); // Display everything drawn
}

// --- START OF SIMPLIFIED m_rebuild ---

// --- START OF COMPLETE m_rebuild (Attempt 3 - Verified Fix Location) ---
void Game::m_rebuild() {

    int maxSolutionsForDifficulty = 0;
    int minSubLengthForDifficulty = MIN_WORD_LENGTH;

    // Select Random Theme
    if (!m_themes.empty()) {
        m_currentTheme = m_themes[0]; //forcing to test colors
        //m_currentTheme = m_themes[randRange<std::size_t>(0, m_themes.size() - 1)];
    }
    else {
        m_currentTheme = {}; std::cerr << "Warning: No themes loaded, using default colors.\n";
    }

    // --- Base Word Selection (Collect Candidates, Pick Randomly) ---
    std::string selectedBaseWord = "";
    bool baseWordFound = false;

    if (m_roots.empty()) {
        std::cerr << "Error: No root words available for puzzle generation.\n";
        m_base = "ERROR";
    }
    else {
        PuzzleCriteria baseCriteria = m_getCriteriaForCurrentPuzzle();
        switch (m_selectedDifficulty) {
        case DifficultyLevel::Easy:   maxSolutionsForDifficulty = EASY_MAX_SOLUTIONS; minSubLengthForDifficulty = MIN_WORD_LENGTH; break;
        case DifficultyLevel::Medium: maxSolutionsForDifficulty = MEDIUM_MAX_SOLUTIONS; minSubLengthForDifficulty = MIN_WORD_LENGTH; break;
        case DifficultyLevel::Hard:   maxSolutionsForDifficulty = HARD_MAX_SOLUTIONS; minSubLengthForDifficulty = HARD_MIN_WORD_LENGTH; break;
        default: maxSolutionsForDifficulty = 999; minSubLengthForDifficulty = MIN_WORD_LENGTH; break;
        }

        // --- Print Status ---
        std::cout << "Rebuilding Puzzle " << (m_currentPuzzleIndex + 1) << "/" << m_puzzlesPerSession
            << " (Difficulty: " << static_cast<int>(m_selectedDifficulty) << ") using pre-calculated metrics." << std::endl;
        std::cout << "  Base Criteria: Lengths="; for (int l : baseCriteria.allowedLengths) std::cout << l << ","; std::cout << " Rarities="; for (int r : baseCriteria.allowedRarities) std::cout << r << ","; std::cout << std::endl;
        std::cout << "  Sub Criteria for Final Filter: MinLen=" << minSubLengthForDifficulty << " MaxSol=" << maxSolutionsForDifficulty << std::endl;
        std::cout << "  DEBUG: Current m_usedBaseWordsThisSession size: " << m_usedBaseWordsThisSession.size() << std::endl;
        std::cout << "  DEBUG: Current m_usedLetterSetsThisSession size: " << m_usedLetterSetsThisSession.size() << std::endl;

        // --- Shuffle & Prepare Candidate Lists ---
        std::shuffle(m_roots.begin(), m_roots.end(), Rng());
        std::vector<int> idealCandidateIndices;     // Indices of words meeting score/count criteria
        std::vector<int> fallbackCandidateIndices;  // Indices of words meeting basic criteria but not ideal
        std::vector<int> broadFallbackIndices;    // Indices for broadest fallback if others fail

        std::cout << "DEBUG: Searching for candidate base words..." << std::endl;
        // --- Loop 1: Find Ideal and Fallback Candidates matching session criteria ---
        for (std::size_t i = 0; i < m_roots.size(); ++i) {
            const auto& rootInfo = m_roots[i]; const std::string& candidateWord = rootInfo.text;

            // Check 1: Basic Word Criteria (Length, Rarity)
            bool lengthMatch = false; for (int len : baseCriteria.allowedLengths) { if (candidateWord.length() == len) { lengthMatch = true; break; } } if (!lengthMatch) continue;
            bool rarityMatch = false; for (int rarity : baseCriteria.allowedRarities) { if (rootInfo.rarity == rarity) { rarityMatch = true; break; } } if (!rarityMatch) continue;

            // Check 2: Already used this exact word string?
            if (m_usedBaseWordsThisSession.count(candidateWord)) { continue; }

            // Check 3: Already used this set of letters (anagram)?
            std::string canonicalCandidate = getCanonicalLetters(candidateWord);
            if (m_usedLetterSetsThisSession.count(canonicalCandidate)) {
                // std::cout << "DEBUG: Skipping candidate '" << candidateWord << "' (anagram of used word - canonical: " << canonicalCandidate << ")." << std::endl; // Verbose log
                continue;
            }

            // --- If checks passed, classify candidate ---
            bool isIdeal = false;
            int validSubCount = 0;
            switch (m_selectedDifficulty) {
            case DifficultyLevel::Easy:   validSubCount = rootInfo.easyValidCount; break;
            case DifficultyLevel::Medium: validSubCount = rootInfo.mediumValidCount; break;
            case DifficultyLevel::Hard:   validSubCount = rootInfo.hardValidCount; break;
            default:                      validSubCount = rootInfo.countGE4;
            }
            // Check if it meets the "ideal" threshold (good sub-word count and quality score)
            if (validSubCount >= MIN_DESIRED_GRID_WORDS && rootInfo.avgSubLen > 0) {
                idealCandidateIndices.push_back(static_cast<int>(i));
                isIdeal = true;
            }

            // If it wasn't ideal but met basic criteria, it's a fallback
            if (!isIdeal) {
                fallbackCandidateIndices.push_back(static_cast<int>(i));
            }
        } // --- End Loop 1 ---

        std::cout << "DEBUG: Found " << idealCandidateIndices.size() << " ideal candidates and "
            << fallbackCandidateIndices.size() << " fallback candidates meeting session criteria." << std::endl;

        // --- Decide Which Base Word to Use (Pick Randomly from Best Available Pool) ---
        int chosenIndex = -1;

        if (!idealCandidateIndices.empty()) {
            // Pick randomly from ideal candidates
            std::size_t randomListIndex = randRange<std::size_t>(0, idealCandidateIndices.size() - 1);
            chosenIndex = idealCandidateIndices[randomListIndex];
            selectedBaseWord = m_roots[chosenIndex].text;
            baseWordFound = true;
            std::cout << "DEBUG: Randomly selected IDEAL candidate #" << randomListIndex << " (Root Index: " << chosenIndex << "): '" << selectedBaseWord << "'" << std::endl;
        }
        else if (!fallbackCandidateIndices.empty()) {
            // No ideal candidates, pick randomly from fallback candidates
            std::size_t randomListIndex = randRange<std::size_t>(0, fallbackCandidateIndices.size() - 1);
            chosenIndex = fallbackCandidateIndices[randomListIndex];
            selectedBaseWord = m_roots[chosenIndex].text;
            baseWordFound = true;
            std::cout << "DEBUG: No IDEAL words. Randomly selected FALLBACK candidate #" << randomListIndex << " (Root Index: " << chosenIndex << "): '" << selectedBaseWord << "'" << std::endl;
        }
        else {
            // No candidates met session criteria + unused checks. Apply BROAD fallback search.
            std::cout << "DEBUG: No candidates met session criteria & unused checks. Applying BROAD fallback search..." << std::endl;
            // --- Loop 2: Find Broad Fallback Candidates ---
            for (std::size_t i = 0; i < m_roots.size(); ++i) {
                const auto& rootInfo = m_roots[i]; const std::string& candidateWord = rootInfo.text;
                // Just check if used (word or letters)
                if (m_usedBaseWordsThisSession.count(candidateWord)) continue;
                std::string canonicalCandidate = getCanonicalLetters(candidateWord);
                if (m_usedLetterSetsThisSession.count(canonicalCandidate)) continue;
                // Any unused word/letter set qualifies as broad fallback
                broadFallbackIndices.push_back(static_cast<int>(i));
            }
            std::cout << "DEBUG: Found " << broadFallbackIndices.size() << " broad fallback candidates." << std::endl;

            if (!broadFallbackIndices.empty()) {
                // Pick randomly from broad fallbacks
                std::size_t randomListIndex = randRange<std::size_t>(0, broadFallbackIndices.size() - 1);
                chosenIndex = broadFallbackIndices[randomListIndex];
                selectedBaseWord = m_roots[chosenIndex].text;
                baseWordFound = true;
                std::cout << "DEBUG: Randomly selected BROAD FALLBACK candidate #" << randomListIndex << " (Root Index: " << chosenIndex << "): '" << selectedBaseWord << "'" << std::endl;
            }
            else {
                // Absolute last resort: ALL words/anagrams used
                std::cerr << "CRITICAL FALLBACK: Cannot find ANY unused root word or letter set. Using first available (may repeat)." << std::endl;
                if (!m_roots.empty()) {
                    selectedBaseWord = m_roots[0].text; // Use the very first word
                    baseWordFound = true;
                    // Don't add to used sets intentionally here, as it's a forced repeat
                }
                else {
                    selectedBaseWord = "ERROR"; // Should not happen if list loads
                }
            }
        }

        // --- Add chosen word to used sets (unless it was the absolute fallback) ---
        if (baseWordFound && chosenIndex != -1) { // Check chosenIndex to exclude the absolute fallback case
            m_usedBaseWordsThisSession.insert(selectedBaseWord);
            m_usedLetterSetsThisSession.insert(getCanonicalLetters(selectedBaseWord));
            std::cout << "DEBUG: Added '" << selectedBaseWord << "' (Canonical: " << getCanonicalLetters(selectedBaseWord) << ") to used sets." << std::endl;
        }

        m_base = selectedBaseWord; // Assign the final selected word (or "ERROR")
    } // End if (!m_roots.empty())


    // Scramble the selected base word letters (only if valid)
    if (m_base != "ERROR" && !m_base.empty()) {
        std::shuffle(m_base.begin(), m_base.end(), Rng());
    }


    // --- Sub-word Processing (Generate ONCE, Filter Unique, Sort, Truncate) ---
    std::vector<WordInfo> final_solutions;
    if (m_base != "ERROR") {
        m_allPotentialSolutions = Words::subWords(m_base, m_fullWordList);
        std::cout << "DEBUG: Generating final grid words for selected base letters (current m_base: '" << m_base << "')." << std::endl;
        std::vector<WordInfo> filtered_sub_solutions; // Initial filtering target
        std::vector<int> allowedSubRarities;
        switch (m_selectedDifficulty) { /* ... set allowedSubRarities ... */
        case DifficultyLevel::Easy:   allowedSubRarities = { 1, 2 };    break;
        case DifficultyLevel::Medium: allowedSubRarities = { 1, 2, 3 }; break;
        case DifficultyLevel::Hard:   allowedSubRarities = { 2, 3, 4 }; break;
        default:                      allowedSubRarities = { 1, 2, 3, 4 }; break;
        }

        for (const auto& subInfo : m_allPotentialSolutions) { /* ... filter by length/rarity into filtered_sub_solutions ... */
            if (subInfo.text.length() < minSubLengthForDifficulty) continue;
            bool subRarityMatch = false;
            for (int subRarity : allowedSubRarities) { if (subInfo.rarity == subRarity) { subRarityMatch = true; break; } }
            if (!subRarityMatch) continue;
            filtered_sub_solutions.push_back(subInfo);
        }
        std::cout << "DEBUG: Found " << filtered_sub_solutions.size() << " potential grid words matching sub-word difficulty criteria." << std::endl;

        // --- Ensure Uniqueness ---
        std::cout << "DEBUG: Ensuring uniqueness of potential grid words..." << std::endl;
        std::set<std::string> unique_texts; std::vector<WordInfo> unique_solutions_temp;
        unique_solutions_temp.reserve(filtered_sub_solutions.size());
        for (const auto& info : filtered_sub_solutions) {
            if (unique_texts.insert(info.text).second) { unique_solutions_temp.push_back(info); }
        }
        std::cout << "DEBUG: Reduced to " << unique_solutions_temp.size() << " unique grid words." << std::endl;

        // --- Sort Unique Solutions ---
        std::cout << "DEBUG: Sorting unique solutions by length/rarity/alpha..." << std::endl;
        std::sort(unique_solutions_temp.begin(), unique_solutions_temp.end(),
            [](const WordInfo& a, const WordInfo& b) { /* ... sort logic ... */
                if (a.text.length() != b.text.length()) return a.text.length() > b.text.length();
                if (a.rarity != b.rarity) return a.rarity < b.rarity;
                return a.text < b.text;
            });

        // --- Truncate Unique Solutions ---
        if (unique_solutions_temp.size() > maxSolutionsForDifficulty) {
            std::cout << "DEBUG: Truncating unique sorted solutions from " << unique_solutions_temp.size() << " to " << maxSolutionsForDifficulty << std::endl;
            unique_solutions_temp.resize(maxSolutionsForDifficulty);
        }
        else { std::cout << "DEBUG: No truncation needed for unique grid words." << std::endl; }

        final_solutions = std::move(unique_solutions_temp); // Assign final unique list

    }
    else { // Handle m_base == "ERROR"
        m_allPotentialSolutions.clear(); final_solutions.clear();
    }

    // Assign final lists for game state
    m_solutions = final_solutions; // UNIQUE list
    m_sorted = Words::sortForGrid(m_solutions);

    m_bonusWordsCacheIsValid = false;
    m_cachedBonusWords.clear();

    // --- Debug Print ---
    std::cout << "DEBUG: m_rebuild - Final Base: '" << m_base << "', FINAL m_solutions count (Grid Target): " << m_solutions.size() << ", m_sorted count: " << m_sorted.size() << std::endl;
    /* ... rest of debug print ... */
    std::cout << "DEBUG: Final list for grid (m_solutions, first 5 or fewer):" << std::endl;
    for (size_t i = 0; i < std::min<size_t>(m_solutions.size(), 5); ++i) {
        std::cout << "  - '" << m_solutions[i].text << "' (Len=" << m_solutions[i].text.length() << ", Rarity=" << m_solutions[i].rarity << ")" << std::endl;
    }
    if (!m_sorted.empty()) {
        std::cout << "DEBUG: m_rebuild - First sorted word for grid display: '" << m_sorted[0].text << "'" << std::endl;
    }
    else { std::cout << "DEBUG: m_rebuild - No words selected for the grid." << std::endl; }


    // --- Setup Grid & Reset State ---
    m_grid.assign(m_sorted.size(), {});
    for (std::size_t i = 0; i < m_sorted.size(); ++i) { /* ... assign grid blanks ... */
        if (!m_sorted[i].text.empty()) {
            m_grid[i].assign(m_sorted[i].text.length(), '_');
        }
        else { m_grid[i].clear(); std::cerr << "Warning: Word at m_sorted index " << i << " has empty text. Grid row will be empty." << std::endl; }
    }
    m_found.clear(); m_foundBonusWords.clear(); m_anims.clear(); m_scoreAnims.clear(); m_hintPointAnims.clear(); m_scoreFlourishes.clear(); 
    m_hintPointsTextFlourishTimer = 0.f;
    m_clearDragState(); m_gameState = GState::Playing;

    // --- Reset Score/Hints ---
    if (m_currentPuzzleIndex == 0 && m_isInSession) { /* ... reset score/hints ... */
        m_currentScore = 0; m_wordsSolvedSinceHint = 0;
        std::cout << "DEBUG: First puzzle of session - Resetting score, hints, and words solved count." << std::endl;
    }
    else if (m_isInSession) { /* ... reset words solved count ... */
        m_wordsSolvedSinceHint = 0;
        std::cout << "DEBUG: Subsequent puzzle - Resetting words solved count for next hint." << std::endl;
    }
    if (m_scoreValueText) m_scoreValueText->setString(std::to_string(m_currentScore));
    if (m_hintCountTxt) m_hintCountTxt->setString("Hints: " + std::to_string(m_hintsAvailable));

    // --- Update Layout & Music ---
    m_updateLayout(m_window.getSize());
    if (m_backgroundMusic.getStatus() != sf::SoundSource::Status::Playing) { /* ... start music ... */
        m_backgroundMusic.stop();
        if (!m_musicFiles.empty()) {
            std::string musicPath = m_musicFiles[randRange<std::size_t>(0, m_musicFiles.size() - 1)];
            if (m_backgroundMusic.openFromFile(musicPath)) {
                m_backgroundMusic.setLooping(true);
                // m_backgroundMusic.play();
                std::cout << "DEBUG: Started background music: " << musicPath << std::endl;
            }
            else { std::cerr << "Error loading music file: " << musicPath << std::endl; }
        }
    }

} // End Game::m_rebuild


// ***** START OF COMPLETE Game::m_updateLayout FUNCTION *****


// ***** START OF COMPLETE Game::m_updateLayout FUNCTION (Corrected Rect Access) *****

void Game::m_updateLayout(sf::Vector2u windowSize) {

    // 1. Calculate Global UI Scale
    m_uiScale = std::min(windowSize.x / static_cast<float>(REF_W),
        windowSize.y / static_cast<float>(REF_H));
    m_uiScale = std::clamp(m_uiScale, 0.65f, 1.6f);

    // 2. Define Design Space References & Sections
    const float designW = static_cast<float>(REF_W);
    const float designH = static_cast<float>(REF_H);
    const sf::Vector2f designCenter = { designW / 2.f, designH / 2.f };
    const float designTopEdge = 0.f;
    const float designLeftEdge = 0.f;
    const float designBottomEdge = designH;
    const float topSectionHeight = designH * 0.15f;
    const float wheelSectionHeight = designH * 0.35f;
    const float gridSectionHeight = designH - topSectionHeight - wheelSectionHeight;
    const float topSectionBottomY = designTopEdge + topSectionHeight;
    const float gridSectionTopY = topSectionBottomY;
    const float gridSectionBottomY = gridSectionTopY + gridSectionHeight;
    // const float wheelSectionTopY = gridSectionBottomY; // Not directly used later for wheel pos
    // const float wheelSectionBottomY = designBottomEdge; // Not directly used later for wheel pos

    bool sizeChanged = (windowSize != m_lastLayoutSize);
    if (sizeChanged) {
        std::cout << "--- Layout Update (" << windowSize.x << "x" << windowSize.y << ") ---" << std::endl;
        // ... (other initial log messages)
    }

    // --- Update Main Background Sprite to fill the design view ---
    if (m_mainBackgroundSpr && m_mainBackgroundTex.getSize().x > 0) { // <<< CHECK m_mainBackgroundSpr POINTER
        m_mainBackgroundSpr->setPosition({ 0.f, 0.f }); 

        float scaleX = static_cast<float>(REF_W) / m_mainBackgroundTex.getSize().x;
        float scaleY = static_cast<float>(REF_H) / m_mainBackgroundTex.getSize().y;

        m_mainBackgroundSpr->setScale({ scaleX, scaleY });

        float designAspect = static_cast<float>(REF_W) / REF_H;
        float bgAspect = static_cast<float>(m_mainBackgroundTex.getSize().x) / m_mainBackgroundTex.getSize().y;
        if (bgAspect > designAspect) { // Background is wider than design space
            m_mainBackgroundSpr->setScale({ scaleY, scaleY });
            m_mainBackgroundSpr->setPosition({ (REF_W - m_mainBackgroundTex.getSize().x * scaleY) / 2.f, 0.f });
         } else { 
            m_mainBackgroundSpr->setScale({ scaleX, scaleX }); // Scale by width, height will be cropped
            m_mainBackgroundSpr->setPosition({ 0.f, (REF_H - m_mainBackgroundTex.getSize().y * scaleX) / 2.f }); // Center vertically
         }
    }

    // 3. Position Top Elements (Score Bar, Progress Meter)
    // --- NEW: Layout for Score Zone Elements (Label and Value) ---
    if (m_scoreLabelText && m_scoreValueText) {
        const float zoneX = SCORE_ZONE_RECT_DESIGN.position.x;
        const float zoneY = SCORE_ZONE_RECT_DESIGN.position.y;
        const float zoneWidth = SCORE_ZONE_RECT_DESIGN.size.x;
        // const float zoneHeight = SCORE_ZONE_RECT_DESIGN.size.y; // Used by bonus text in render

        const float scaledPaddingX = S(this, SCORE_ZONE_PADDING_X_DESIGN);
        const float scaledPaddingY = S(this, SCORE_ZONE_PADDING_Y_DESIGN);
        const float scaledLabelValueGap = S(this, SCORE_LABEL_VALUE_GAP_DESIGN);

        // --- "SCORE:" Label (m_scoreLabelText) ---
        m_scoreLabelText->setFont(m_font); // Ensure font
        m_scoreLabelText->setString("SCORE:"); // Confirm string
        m_scoreLabelText->setCharacterSize(static_cast<unsigned int>(S(this, SCORE_ZONE_LABEL_FONT_SIZE)));
        m_scoreLabelText->setFillColor(GLOWING_TUBE_TEXT_COLOR);
        sf::FloatRect labelBounds = m_scoreLabelText->getLocalBounds();
        m_scoreLabelText->setOrigin({ labelBounds.position.x + labelBounds.size.x / 2.f,
                                     labelBounds.position.y });
        m_scoreLabelText->setPosition(sf::Vector2f{ zoneX + zoneWidth / 2.f,
                                                   zoneY + scaledPaddingY });

        // --- Score Value (m_scoreValueText) ---
        m_scoreValueText->setFont(m_font);
        m_scoreValueText->setCharacterSize(static_cast<unsigned int>(S(this, SCORE_ZONE_VALUE_FONT_SIZE)));
        m_scoreValueText->setFillColor(GLOWING_TUBE_TEXT_COLOR);
        sf::FloatRect valueBounds = m_scoreValueText->getLocalBounds(); // Re-get if string can change width
        m_scoreValueText->setOrigin({ valueBounds.position.x + valueBounds.size.x / 2.f,
                                     valueBounds.position.y });
        m_scoreValueText->setPosition(sf::Vector2f{ zoneX + zoneWidth / 2.f,
                                                   m_scoreLabelText->getPosition().y +
                                                       (labelBounds.position.y + labelBounds.size.y) -
                                                       m_scoreLabelText->getOrigin().y +
                                                       scaledLabelValueGap });
    }
    // Bonus words text will be positioned entirely within m_renderGameScreen
    // --- End Layout for Score Zone Elements ---


    // --- 4. Calculate Grid Layout ---
    // --- 4a. Define Grid Zone's Inner boundaries and Original m_gridStartY ---
    // These are in design coordinates. The sf::View handles overall scaling to window.
    const float zoneInnerX = GRID_ZONE_RECT_DESIGN.position.x + GRID_ZONE_PADDING_X_DESIGN;
    const float zoneInnerY = GRID_ZONE_RECT_DESIGN.position.y + GRID_ZONE_PADDING_Y_DESIGN;
    const float zoneInnerWidth = GRID_ZONE_RECT_DESIGN.size.x - 2 * GRID_ZONE_PADDING_X_DESIGN;
    const float zoneInnerHeight = GRID_ZONE_RECT_DESIGN.size.y - 2 * GRID_ZONE_PADDING_Y_DESIGN;
    float actualGridFinalHeight = 0.f;

    m_gridStartY = zoneInnerY; // Grid will start at the top of the padded zone area
    // This might be adjusted later if we center vertically.

    int numCols = 1;
    int maxRowsPerCol = m_sorted.empty() ? 1 : (int)m_sorted.size();
    //float actualGridFinalHeight = 0; // Will store the final height of the rendered grid
    m_wordCol.clear(); m_wordRow.clear(); m_colMaxLen.clear(); m_colXOffset.clear();

    // This scale factor will be calculated to make the grid fit the zone.
    // It will be <= 1.0. If 1.0, it means original TILE_SIZE fits.
    float gridElementsScaleFactor = 1.0f;

    if (!m_sorted.empty()) {
        // --- 4b. Heuristic: Find best column/row count using ORIGINAL (unscaled) design constants ---
        // TILE_SIZE, TILE_PAD, COL_PAD are base design values.
        const float st_base_design = TILE_SIZE; // Original tile size from Constants.h
        const float sp_base_design = TILE_PAD;  // Original tile padding
        const float sc_base_design = COL_PAD;   // Original column padding

        const float stph_base_design = st_base_design + sp_base_design;
        const float stpw_base_design = st_base_design + sp_base_design;

        // The heuristic itself for choosing column/row counts can largely remain.
        // It prioritizes fitting MAX_ROWS_LIMIT and then being narrow.
        // We are NOT using zoneInnerHeight directly in *this* heuristic pass,
        // because we'll scale the result later.
        int bestFitCols = 1; int bestFitRows = (int)m_sorted.size();
        // float minWidthVertFit = std::numeric_limits<float>::max(); bool foundVerticalFit = false; // Old variables
        int narrowestOverallCols = 1; int narrowestOverallRows = (int)m_sorted.size(); float minWidthOverall = std::numeric_limits<float>::max();
        int maxPossibleCols = std::min(8, std::max(1, (int)m_sorted.size()));

        for (int tryCols = 1; tryCols <= maxPossibleCols; ++tryCols) {
            int rowsNeeded = ((int)m_sorted.size() + tryCols - 1) / tryCols; if (rowsNeeded <= 0) rowsNeeded = 1;
            std::vector<int> currentTryColMaxLen(tryCols, 0); float currentTryWidthUnscaled = 0;
            for (size_t w = 0; w < m_sorted.size(); ++w) { int c = (int)w / rowsNeeded; if (c >= 0 && c < tryCols) { currentTryColMaxLen[c] = std::max<int>(currentTryColMaxLen[c], (int)m_sorted[w].text.length()); } }
            for (int len : currentTryColMaxLen) { currentTryWidthUnscaled += (float)len * stpw_base_design - (len > 0 ? sp_base_design : 0.f); }
            currentTryWidthUnscaled += (float)std::max(0, tryCols - 1) * sc_base_design; if (currentTryWidthUnscaled < 0) currentTryWidthUnscaled = 0;

            // float currentTryHeightUnscaled = (float)rowsNeeded * stph_base_design - (rowsNeeded > 0 ? sp_base_design : 0.f); // Not strictly needed for selection here

            if (currentTryWidthUnscaled < minWidthOverall) { // Prioritize narrowness
                minWidthOverall = currentTryWidthUnscaled;
                narrowestOverallCols = tryCols;
                narrowestOverallRows = rowsNeeded;
            }
        }
        int chosenCols = narrowestOverallCols;
        int chosenRows = narrowestOverallRows;

        // Apply MAX_ROWS_LIMIT override (this is a hard constraint on rows per column)
        const int MAX_ROWS_LIMIT = 5; // Keep your existing limit
        if (chosenRows > MAX_ROWS_LIMIT) {
            chosenRows = MAX_ROWS_LIMIT;
            chosenCols = ((int)m_sorted.size() + chosenRows - 1) / chosenRows;
            if (chosenCols <= 0) chosenCols = 1;
        }
        numCols = chosenCols;
        maxRowsPerCol = chosenRows;

        // --- 4c. Calculate Max Length per Column (using final numCols) ---
        m_colMaxLen.assign(numCols, 0);
        for (size_t w = 0; w < m_sorted.size(); ++w) {
            int c = (int)w / maxRowsPerCol; if (c >= numCols) c = numCols - 1;
            if (c >= 0 && c < m_colMaxLen.size()) { m_colMaxLen[c] = std::max<int>(m_colMaxLen[c], (int)m_sorted[w].text.length()); }
        }

        // --- 4d. Calculate Total Required Width & Height with ORIGINAL tile/pad sizes ---
        float totalRequiredWidthUnscaled = 0;
        for (int c = 0; c < numCols; ++c) {
            int len = (c >= 0 && c < m_colMaxLen.size()) ? m_colMaxLen[c] : 0;
            float colWidth_unscaled = (float)len * stpw_base_design - (len > 0 ? sp_base_design : 0.f);
            if (colWidth_unscaled < 0) colWidth_unscaled = 0;
            totalRequiredWidthUnscaled += colWidth_unscaled;
        }
        totalRequiredWidthUnscaled += (float)std::max(0, numCols - 1) * sc_base_design;
        if (totalRequiredWidthUnscaled < 0) totalRequiredWidthUnscaled = 0;

        float totalRequiredHeightUnscaled = (float)maxRowsPerCol * stph_base_design - (maxRowsPerCol > 0 ? sp_base_design : 0.f);
        if (totalRequiredHeightUnscaled < 0) totalRequiredHeightUnscaled = 0;

        // --- 4e. Calculate Scale Factor to Fit this Configuration into the Zone's Inner Area ---
        float scaleToFitX = 1.0f;
        if (totalRequiredWidthUnscaled > zoneInnerWidth && totalRequiredWidthUnscaled > 0) {
            scaleToFitX = zoneInnerWidth / totalRequiredWidthUnscaled;
        }
        float scaleToFitY = 1.0f;
        if (totalRequiredHeightUnscaled > zoneInnerHeight && totalRequiredHeightUnscaled > 0) {
            scaleToFitY = zoneInnerHeight / totalRequiredHeightUnscaled;
        }
        gridElementsScaleFactor = std::min(scaleToFitX, scaleToFitY);
        // Ensure we don't scale *up* beyond original TILE_SIZE if it already fits.
        // gridElementsScaleFactor = std::min(1.0f, gridElementsScaleFactor); // This is implicitly handled if scaleToFitX/Y start at 1.0

        if (sizeChanged) { // Log only on size change for clarity
            std::cout << "  GRID ZONE: Inner W=" << zoneInnerWidth << ", Inner H=" << zoneInnerHeight << std::endl;
            std::cout << "  GRID UNSCALED: Req W=" << totalRequiredWidthUnscaled << ", Req H=" << totalRequiredHeightUnscaled
                << " (for " << numCols << "c x " << maxRowsPerCol << "r)" << std::endl;
            std::cout << "  GRID SCALE FACTOR: " << gridElementsScaleFactor << std::endl;
        }

        // --- 4f. Calculate Final Scaled Tile/Padding Sizes ---
        // These are the sizes that will be used for rendering and m_tilePos
        const float st_final = TILE_SIZE * gridElementsScaleFactor;
        const float sp_final = TILE_PAD * gridElementsScaleFactor;
        const float sc_final = COL_PAD * gridElementsScaleFactor;
        const float stph_final = st_final + sp_final; // Scaled step height
        const float stpw_final = st_final + sp_final; // Scaled step width

        // --- 4g. Calculate Final Total Grid Dimensions (width and height) with scaled elements ---
        float currentX_final_scaled = 0;
        m_colXOffset.resize(numCols); // Will store offset of each column from grid's local 0,0
        for (int c = 0; c < numCols; ++c) {
            m_colXOffset[c] = currentX_final_scaled;
            int len = (c >= 0 && c < m_colMaxLen.size()) ? m_colMaxLen[c] : 0;
            float colWidth_final_scaled = (float)len * stpw_final - (len > 0 ? sp_final : 0.f);
            if (colWidth_final_scaled < 0) colWidth_final_scaled = 0;
            currentX_final_scaled += colWidth_final_scaled + sc_final;
        }
        m_totalGridW = currentX_final_scaled - (numCols > 0 ? sc_final : 0.f); // Final scaled width of the grid content
        if (m_totalGridW < 0) m_totalGridW = 0;

        actualGridFinalHeight = (float)maxRowsPerCol * stph_final - (maxRowsPerCol > 0 ? sp_final : 0.f);
        if (actualGridFinalHeight < 0) actualGridFinalHeight = 0;

        // --- 4h. Calculate Grid Start Position to Center it within the Zone's Inner Area ---
        m_gridStartX = zoneInnerX + (zoneInnerWidth - m_totalGridW) / 2.f;
        m_gridStartY = zoneInnerY + (zoneInnerHeight - actualGridFinalHeight) / 2.f; // Vertically center too

        // Adjust column offsets to be relative to the screen (design space)
        for (int c = 0; c < numCols; ++c) {
            if (c < m_colXOffset.size()) { m_colXOffset[c] += m_gridStartX; }
        }

        // Assign word rows/cols
        m_wordCol.resize(m_sorted.size()); m_wordRow.resize(m_sorted.size());
        for (size_t w = 0; w < m_sorted.size(); ++w) { int c = (int)w / maxRowsPerCol; int r = (int)w % maxRowsPerCol; if (c >= numCols) c = numCols - 1; m_wordCol[w] = c; m_wordRow[w] = r; }

        if (sizeChanged) {
            std::cout << "  GRID FINAL: Scaled Tile=" << st_final << ", Scaled Pad=" << sp_final << std::endl;
            std::cout << "  GRID FINAL: Total W=" << m_totalGridW << ", Total H=" << actualGridFinalHeight << std::endl;
            std::cout << "  GRID FINAL: StartX=" << m_gridStartX << ", StartY=" << m_gridStartY << std::endl;
        }

        // Store the calculated scale factor if m_tilePos or rendering needs it explicitly
        // The old m_currentGridLayoutScale and GRID_TILE_RELATIVE_SCALE_WHEN_SHRUNK are now
        // effectively replaced by this single gridElementsScaleFactor for TILE_SIZE, TILE_PAD, COL_PAD.
        // Let's make a new member if needed, or m_tilePos can recalculate scaled sizes.
        // For now, m_tilePos will need to know gridElementsScaleFactor or be passed st_final, sp_final.
        // It's cleaner if m_tilePos uses this factor.
        // We can reuse m_currentGridLayoutScale for this purpose if its previous meaning is deprecated.
        m_currentGridLayoutScale = gridElementsScaleFactor; // Re-purpose this to store the element scale
        actualGridFinalHeight = (float)maxRowsPerCol * stph_final - (maxRowsPerCol > 0 ? sp_final : 0.f);
    }
    else { // Grid is empty
        m_gridStartX = zoneInnerX + zoneInnerWidth / 2.f; // Center of empty zone
        m_gridStartY = zoneInnerY + zoneInnerHeight / 2.f;
        m_totalGridW = 0; actualGridFinalHeight = 0;
        m_currentGridLayoutScale = 1.0f; // Reset scale factor
        actualGridFinalHeight = 0;
    }
    float calculatedGridActualBottomY = m_gridStartY + actualGridFinalHeight;
    // --- End Grid Calculation Section ---


    // 5. Determine Final Wheel Size & Position
    // ... (This section remains unchanged from your working version) ...
    const float wheelZoneInnerX = WHEEL_ZONE_RECT_DESIGN.position.x + WHEEL_ZONE_PADDING_DESIGN;
    const float wheelZoneInnerY = WHEEL_ZONE_RECT_DESIGN.position.y + WHEEL_ZONE_PADDING_DESIGN;
    const float wheelZoneInnerWidth = WHEEL_ZONE_RECT_DESIGN.size.x - 2 * WHEEL_ZONE_PADDING_DESIGN;
    const float wheelZoneInnerHeight = WHEEL_ZONE_RECT_DESIGN.size.y - 2 * WHEEL_ZONE_PADDING_DESIGN;
    const float scaledGridWheelGap = S(this, GRID_WHEEL_GAP);
    float gridAreaLimitY = calculatedGridActualBottomY + scaledGridWheelGap;

    if (sizeChanged) { // Log only on size change
        std::cout << "  WHEEL ZONE: Inner X=" << wheelZoneInnerX << ", Y=" << wheelZoneInnerY
            << ", W=" << wheelZoneInnerWidth << ", H=" << wheelZoneInnerHeight << std::endl;
    }

    // The wheel's center will be the center of this inner zone.
    m_wheelX = wheelZoneInnerX + wheelZoneInnerWidth / 2.f;
    m_wheelY = wheelZoneInnerY + wheelZoneInnerHeight / 2.f;

    // The wheel's radius must fit within the smaller dimension of the inner zone.
    // This radius is for the m_wheelCentres (where letter interaction points are).
    float maxRadiusForZone = std::min(wheelZoneInnerWidth / 2.f, wheelZoneInnerHeight / 2.f);

    // Compare with your original WHEEL_R constant (which is in design units)
    // We want the smaller of the original design radius or what fits the zone.
    m_currentWheelRadius = std::min(maxRadiusForZone, WHEEL_R); // WHEEL_R is from Constants.h

    // Ensure a minimum sensible radius, especially if the zone is tiny
    // (LETTER_R is also a design unit constant for the visual letter radius)
    float minSensibleRadius = LETTER_R * 1.5f; // e.g., must be at least 1.5x a letter's visual radius
    m_currentWheelRadius = std::max(m_currentWheelRadius, minSensibleRadius);


    if (sizeChanged) {
        std::cout << "  WHEEL FINAL: ZoneMaxR=" << maxRadiusForZone << ", DesignR=" << WHEEL_R
            << ", Clamped CurrentR=" << m_currentWheelRadius << std::endl;
        std::cout << "  WHEEL FINAL: Center X=" << m_wheelX << ", Y=" << m_wheelY << std::endl;
    }
    // --- End Wheel Size & Position Determination ---

    // --- 6. Calculate Final Wheel Letter Positions & Visual Background Radius ---
    // m_currentWheelRadius is now the definitive radius for the logical letter path,
    // already constrained by the zone.

    // --- A. Calculate Visual Size of Individual Letters (m_currentLetterRenderRadius) ---
    if (!m_base.empty()) {
        float radiusBasedOnCount = (PI * m_currentWheelRadius) / static_cast<float>(m_base.size());
        radiusBasedOnCount *= 0.75f;

        float scaleFactorFromDesign = 1.0f;
        if (WHEEL_R > 0.1f) {
            scaleFactorFromDesign = m_currentWheelRadius / WHEEL_R;
        }
        float radiusBasedOnOverallScale = LETTER_R_BASE_DESIGN * scaleFactorFromDesign;

        m_currentLetterRenderRadius = std::min(radiusBasedOnCount, radiusBasedOnOverallScale);

        float minAbsRadius = m_currentWheelRadius * MIN_LETTER_RADIUS_FACTOR;
        float maxAbsRadius = m_currentWheelRadius * MAX_LETTER_RADIUS_FACTOR;
        m_currentLetterRenderRadius = std::clamp(m_currentLetterRenderRadius, minAbsRadius, maxAbsRadius);
        m_currentLetterRenderRadius = std::max(m_currentLetterRenderRadius, 5.f);
    }
    else {
        m_currentLetterRenderRadius = LETTER_R_BASE_DESIGN;
    }

    // --- B. Calculate Radius for Placing Letter Centers (m_letterPositionRadius) ---
    float calculatedLetterPositionRadius = m_currentWheelRadius;

    calculatedLetterPositionRadius = std::max(calculatedLetterPositionRadius, m_currentLetterRenderRadius * 0.5f);
    calculatedLetterPositionRadius = std::max(calculatedLetterPositionRadius, m_currentWheelRadius * 0.3f);

    m_letterPositionRadius = calculatedLetterPositionRadius; 

    // --- C. Calculate Visual Background for the Wheel (m_visualBgRadius) ---
    // Ensure LETTER_R_BASE_DESIGN is not zero to avoid division by zero if m_currentLetterRenderRadius is also zero.
    float letterSizeRatioForPadding = (LETTER_R_BASE_DESIGN > 0.01f) ? (m_currentLetterRenderRadius / LETTER_R_BASE_DESIGN) : 1.0f;
    float calculatedVisualBgRadius = this->m_letterPositionRadius + m_currentLetterRenderRadius +
        (WHEEL_BG_PADDING_AROUND_LETTERS_DESIGN * letterSizeRatioForPadding);
    this->m_visualBgRadius = calculatedVisualBgRadius; // Assign to member

    // Position the actual letters on the wheel using the member m_letterPositionRadius
    m_wheelLetterRenderPos.resize(m_base.size());
    if (!m_base.empty()) {
        float angleStep = (2.f * PI) / (float)m_base.size();
        for (size_t i = 0; i < m_base.size(); ++i) {
            float ang = (float)i * angleStep - PI / 2.f;
            m_wheelLetterRenderPos[i] = { // This is sf::Vector2f construction, should be fine
                m_wheelX + this->m_letterPositionRadius * std::cos(ang),
                m_wheelY + this->m_letterPositionRadius * std::sin(ang)
            };
        }
    }

    // Debugging output
    if (sizeChanged || true) {
        std::cout << "  WHEEL PATH: m_currentWheelRadius = " << m_currentWheelRadius << std::endl;
        std::cout << "  WHEEL LETTERS: Count=" << m_base.size()
            << ", Visual Radius (m_currentLetterRenderRadius) = " << m_currentLetterRenderRadius << std::endl;
        std::cout << "  WHEEL LETTERS: Placement Radius (m_letterPositionRadius) = " << this->m_letterPositionRadius << std::endl;
        std::cout << "  WHEEL BG: Visual Radius (m_visualBgRadius) = " << this->m_visualBgRadius << std::endl;
    }
    // --- End Wheel Letter Positions Calculation ---

    // 7. Other UI Element Positions (Scramble, Continue, Guess Display)
    // ... (This section remains unchanged from your working version) ...
    if (m_scrambleSpr && m_scrambleTex.getSize().y > 0) {
        float h = S(this, SCRAMBLE_BTN_HEIGHT);
        // Ensure texture height is not zero to avoid division by zero
        float texHeight = static_cast<float>(m_scrambleTex.getSize().y);
        if (texHeight > 0.1f) {
            float s = h / texHeight;
            m_scrambleSpr->setScale({ s, s }); // SFML 3 style
        }
        m_scrambleSpr->setOrigin({ 0.f, texHeight / 2.f }); // SFML 3 style
        // Use m_visualBgRadius here
        m_scrambleSpr->setPosition(sf::Vector2f{ m_wheelX + m_visualBgRadius + S(this, SCRAMBLE_BTN_OFFSET_X),
                                                m_wheelY + S(this, SCRAMBLE_BTN_OFFSET_Y) }); // Explicit sf::Vector2f
    }

    if (m_contTxt && m_contBtn.getPointCount() > 0) {
        sf::Vector2f s_size = { S(this, 200.f), S(this, 50.f) }; // Renamed 's' to 's_size' to avoid conflict if 's' is used later
        m_contBtn.setSize(s_size);
        m_contBtn.setRadius(S(this, 10.f));
        m_contBtn.setOrigin({ s_size.x / 2.f, 0.f }); // Use s_size.x
        // Use m_visualBgRadius here
        m_contBtn.setPosition(sf::Vector2f{ m_wheelX,
                                           m_wheelY + m_visualBgRadius + S(this, CONTINUE_BTN_OFFSET_Y) }); // Explicit sf::Vector2f

        const unsigned int bf_cont = 24; // Renamed 'bf' to avoid conflict
        unsigned int sf_cont = (unsigned int)std::max(10.0f, S(this, static_cast<float>(bf_cont)));
        m_contTxt->setCharacterSize(sf_cont);
        sf::FloatRect tb_cont = m_contTxt->getLocalBounds(); // Renamed 'tb'
        m_contTxt->setOrigin({ tb_cont.position.x + tb_cont.size.x / 2.f,
                              tb_cont.position.y + tb_cont.size.y / 2.f });
        m_contTxt->setPosition(m_contBtn.getPosition() + sf::Vector2f{ 0.f, s_size.y / 2.f }); // Use s_size.y
    }

    // Guess Display Text character size and Background radius/outline
    if (m_guessDisplay_Text) {
        const unsigned int bf_guess = 30; // Renamed 'bf'
        unsigned int sf_guess = (unsigned int)std::max(10.0f, S(this, static_cast<float>(bf_guess)));
        m_guessDisplay_Text->setCharacterSize(sf_guess);
    }
    if (m_guessDisplay_Bg.getPointCount() > 0) {
        m_guessDisplay_Bg.setRadius(S(this, 8.f));
        m_guessDisplay_Bg.setOutlineThickness(S(this, 1.f));
    }

    // Logging for HUD Start Y (this used local visualBgRadius before)
    const float scaledHudOffsetY_log = S(this, HUD_TEXT_OFFSET_Y);
    float calculatedHudStartY_log = m_wheelY + m_visualBgRadius + scaledHudOffsetY_log;
    float visualWheelTopEdgeY_log = m_wheelY - m_visualBgRadius;

    if (sizeChanged) { // Only log if size changed to reduce spam
        std::cout << "  WHEEL/HUD INFO (updateLayout): Visual Wheel BG Top Edge Y = " << visualWheelTopEdgeY_log << std::endl;
        std::cout << "  WHEEL/HUD INFO (updateLayout): Calculated HUD Start Y = " << calculatedHudStartY_log << std::endl;
        if (actualGridFinalHeight > 0 && visualWheelTopEdgeY_log < calculatedGridActualBottomY - 0.1f) {
            std::cout << "  WHEEL/HUD WARNING (updateLayout): Visual Wheel BG (Y=" << visualWheelTopEdgeY_log
                << ") overlaps Grid Bottom (Y=" << calculatedGridActualBottomY << ")!" << std::endl;
        }
        if (calculatedHudStartY_log > designBottomEdge + 0.1f) {
            std::cout << "  WHEEL/HUD WARNING (updateLayout): Calculated HUD Start Y (" << calculatedHudStartY_log
                << ") is below Design Bottom Edge (" << designBottomEdge << ")" << std::endl;
        }
    }

    // 8. Menu Layouts
    // ... (This section remains unchanged from your working version) ...
    sf::Vector2f windowCenterPix = sf::Vector2f(windowSize) / 2.f; sf::Vector2f mappedWindowCenter = m_window.mapPixelToCoords(sf::Vector2i(windowCenterPix)); const float scaledMenuPadding = S(this, 40.f); const float scaledButtonSpacing = S(this, 20.f); const unsigned int scaledTitleSize = (unsigned int)std::max(12.0f, S(this, 36.f)); const unsigned int scaledButtonFontSize = (unsigned int)std::max(10.0f, S(this, 24.f)); const sf::Vector2f scaledButtonSize = { S(this,250.f),S(this,50.f) }; const float scaledButtonRadius = S(this, 10.f); const float scaledMenuRadius = S(this, 15.f); auto centerTextOnButton = [&](const std::unique_ptr<sf::Text>& textPtr, const RoundedRectangleShape& button) { if (!textPtr) return; sf::Text* text = textPtr.get(); sf::FloatRect tb = text->getLocalBounds(); text->setOrigin({ tb.position.x + tb.size.x / 2.f,tb.position.y + tb.size.y / 2.f }); text->setPosition(button.getPosition() + sf::Vector2f{ 0.f,button.getSize().y / 2.f }); };
    if (m_mainMenuTitle && m_casualButtonShape.getPointCount() > 0) { m_mainMenuTitle->setCharacterSize(scaledTitleSize); m_casualButtonText->setCharacterSize(scaledButtonFontSize); m_competitiveButtonText->setCharacterSize(scaledButtonFontSize); m_quitButtonText->setCharacterSize(scaledButtonFontSize); m_casualButtonShape.setSize(scaledButtonSize); m_competitiveButtonShape.setSize(scaledButtonSize); m_quitButtonShape.setSize(scaledButtonSize); m_casualButtonShape.setRadius(scaledButtonRadius); m_competitiveButtonShape.setRadius(scaledButtonRadius); m_quitButtonShape.setRadius(scaledButtonRadius); sf::FloatRect titleBounds = m_mainMenuTitle->getLocalBounds(); float sths = titleBounds.size.y + titleBounds.position.y + scaledButtonSpacing; float tbh = 3 * scaledButtonSize.y + 2 * scaledButtonSpacing; float smmh = scaledMenuPadding + sths + tbh + scaledMenuPadding; float smmw = std::max(scaledButtonSize.x, titleBounds.size.x + titleBounds.position.x) + 2 * scaledMenuPadding; m_mainMenuBg.setSize({ smmw,smmh }); m_mainMenuBg.setRadius(scaledMenuRadius); m_mainMenuBg.setOrigin({ smmw / 2.f,smmh / 2.f }); m_mainMenuBg.setPosition(mappedWindowCenter); sf::Vector2f mbp = m_mainMenuBg.getPosition(); float mty = mbp.y - smmh / 2.f; m_mainMenuTitle->setOrigin({ titleBounds.position.x + titleBounds.size.x / 2.f,titleBounds.position.y }); m_mainMenuTitle->setPosition({ mbp.x,mty + scaledMenuPadding }); float currentY = mty + scaledMenuPadding + sths; m_casualButtonShape.setOrigin({ scaledButtonSize.x / 2.f,0.f }); m_casualButtonShape.setPosition({ mbp.x,currentY }); centerTextOnButton(m_casualButtonText, m_casualButtonShape); currentY += scaledButtonSize.y + scaledButtonSpacing; m_competitiveButtonShape.setOrigin({ scaledButtonSize.x / 2.f,0.f }); m_competitiveButtonShape.setPosition({ mbp.x,currentY }); centerTextOnButton(m_competitiveButtonText, m_competitiveButtonShape); currentY += scaledButtonSize.y + scaledButtonSpacing; m_quitButtonShape.setOrigin({ scaledButtonSize.x / 2.f,0.f }); m_quitButtonShape.setPosition({ mbp.x,currentY }); centerTextOnButton(m_quitButtonText, m_quitButtonShape); }
    if (m_casualMenuTitle && m_easyButtonShape.getPointCount() > 0) { m_casualMenuTitle->setCharacterSize(scaledTitleSize); m_easyButtonText->setCharacterSize(scaledButtonFontSize); m_mediumButtonText->setCharacterSize(scaledButtonFontSize); m_hardButtonText->setCharacterSize(scaledButtonFontSize); m_returnButtonText->setCharacterSize(scaledButtonFontSize); m_easyButtonShape.setSize(scaledButtonSize); m_mediumButtonShape.setSize(scaledButtonSize); m_hardButtonShape.setSize(scaledButtonSize); m_returnButtonShape.setSize(scaledButtonSize); m_easyButtonShape.setRadius(scaledButtonRadius); m_mediumButtonShape.setRadius(scaledButtonRadius); m_hardButtonShape.setRadius(scaledButtonRadius); m_returnButtonShape.setRadius(scaledButtonRadius); sf::FloatRect ctb = m_casualMenuTitle->getLocalBounds(); float sths_c = ctb.size.y + ctb.position.y + scaledButtonSpacing; float tbh_c = 4 * scaledButtonSize.y + 3 * scaledButtonSpacing; float scmh = scaledMenuPadding + sths_c + tbh_c + scaledMenuPadding; float scmw = std::max(scaledButtonSize.x, ctb.size.x + ctb.position.x) + 2 * scaledMenuPadding; m_casualMenuBg.setSize({ scmw,scmh }); m_casualMenuBg.setRadius(scaledMenuRadius); m_casualMenuBg.setOrigin({ scmw / 2.f,scmh / 2.f }); m_casualMenuBg.setPosition(mappedWindowCenter); sf::Vector2f cmbp = m_casualMenuBg.getPosition(); float cmty = cmbp.y - scmh / 2.f; m_casualMenuTitle->setOrigin({ ctb.position.x + ctb.size.x / 2.f,ctb.position.y }); m_casualMenuTitle->setPosition({ cmbp.x,cmty + scaledMenuPadding }); float ccy = cmty + scaledMenuPadding + sths_c; m_easyButtonShape.setOrigin({ scaledButtonSize.x / 2.f,0.f }); m_easyButtonShape.setPosition({ cmbp.x,ccy }); centerTextOnButton(m_easyButtonText, m_easyButtonShape); ccy += scaledButtonSize.y + scaledButtonSpacing; m_mediumButtonShape.setOrigin({ scaledButtonSize.x / 2.f,0.f }); m_mediumButtonShape.setPosition({ cmbp.x,ccy }); centerTextOnButton(m_mediumButtonText, m_mediumButtonShape); ccy += scaledButtonSize.y + scaledButtonSpacing; m_hardButtonShape.setOrigin({ scaledButtonSize.x / 2.f,0.f }); m_hardButtonShape.setPosition({ cmbp.x,ccy }); centerTextOnButton(m_hardButtonText, m_hardButtonShape); ccy += scaledButtonSize.y + scaledButtonSpacing; m_returnButtonShape.setOrigin({ scaledButtonSize.x / 2.f,0.f }); m_returnButtonShape.setPosition({ cmbp.x,ccy }); centerTextOnButton(m_returnButtonText, m_returnButtonShape); }


    // --- 9. Hint UI Layout (Left Side) ---
    // This is the critical section to correct for 4 hint buttons.
    const float HINT_UI_MAX_SCALE = 1.0f;
    float hintUiInternalScale = std::min(m_uiScale, HINT_UI_MAX_SCALE);

    const float hintAreaPadding = hintUiInternalScale * 15.f;
    const float hintElementSpacing = hintUiInternalScale * 10.f; // spacing between elements
    const sf::Vector2f hintButtonSize = { hintUiInternalScale * 120.f, hintUiInternalScale * 35.f };
    const float hintButtonRadius = hintUiInternalScale * 6.f;
    const float costTextOffsetX = hintUiInternalScale * 8.f;
    const float gapBelowGridBaseValue = 75.f;
    const float gapBelowGrid = S(this, gapBelowGridBaseValue);

    // --- Stage 1: Calculate required dimensions using HINT scale ---
    float requiredWidth = 0.f;
    float requiredHeight = 0.f;
    float currentSimulatedY = 0;

    auto estimateRowWidth = [&](float buttonWidth, float textScale, float textOffsetX) {
        float estimatedCostTextWidth = textScale * 60.f; // Approx "Cost: XX"
        return buttonWidth + textOffsetX + estimatedCostTextWidth;
        };

    // Points Text
    if (m_hintPointsText) {
        float estimatedPointsWidth = hintUiInternalScale * 100.f;
        requiredWidth = std::max(requiredWidth, estimatedPointsWidth);
        const unsigned int pointsFontSize = static_cast<unsigned int>(std::max(8.0f, hintUiInternalScale * 20.f));
        currentSimulatedY += (float)pointsFontSize * 1.5f; // Approx line height with some spacing
    }

    // Hint 1 (RevealFirst "Letter")
    requiredWidth = std::max(requiredWidth, estimateRowWidth(hintButtonSize.x, hintUiInternalScale, costTextOffsetX));
    currentSimulatedY += (currentSimulatedY > 0 && m_hintPointsText ? hintElementSpacing : 0) + hintButtonSize.y;

    // Hint 2 (RevealRandom "Random")
    requiredWidth = std::max(requiredWidth, estimateRowWidth(hintButtonSize.x, hintUiInternalScale, costTextOffsetX));
    currentSimulatedY += hintElementSpacing + hintButtonSize.y;

    // Hint 3 (RevealLast "Last Word")
    requiredWidth = std::max(requiredWidth, estimateRowWidth(hintButtonSize.x, hintUiInternalScale, costTextOffsetX));
    currentSimulatedY += hintElementSpacing + hintButtonSize.y;

    // Hint 4 (RevealFirstOfEach "1st of Each")
    requiredWidth = std::max(requiredWidth, estimateRowWidth(hintButtonSize.x, hintUiInternalScale, costTextOffsetX));
    currentSimulatedY += hintElementSpacing + hintButtonSize.y;

    requiredHeight = currentSimulatedY; // Total height for elements

    // --- Stage 2: Calculate Background Frame properties ---
    float bgWidth = requiredWidth + hintAreaPadding * 2.f;
    float bgHeight = requiredHeight + hintAreaPadding * 2.f; // bgHeight now reflects 4 buttons
    float bgRadius = hintUiInternalScale * 10.f;

    // --- Stage 3: Determine Background Top-Left Position ---
    float finalBgPosX = HINT_ZONE_RECT_DESIGN.position.x + HINT_BG_PADDING_X;
    float finalBgPosY = HINT_ZONE_RECT_DESIGN.position.y + HINT_BG_PADDING_Y;

    // --- Stage 4: Set Background Position and Size ---
    m_hintAreaBg.setSize({ bgWidth, bgHeight });
    m_hintAreaBg.setRadius(bgRadius);
    m_hintAreaBg.setOrigin({ 0.f, 0.f });
    m_hintAreaBg.setPosition({ finalBgPosX, finalBgPosY });
    
    // --- Stage 5: Position Elements INSIDE the Background Frame ---
    float elementStartX = finalBgPosX + hintAreaPadding;
    float currentElementY = finalBgPosY + hintAreaPadding; // Start Y for the first element

    // Position "Points: XXX" Text
    if (m_hintPointsText) {
        const unsigned int pointsFontSize = static_cast<unsigned int>(std::max(8.0f, hintUiInternalScale * 20.f));
        m_hintPointsText->setCharacterSize(pointsFontSize);
        sf::FloatRect ptBounds = m_hintPointsText->getLocalBounds();
        m_hintPointsText->setOrigin({ ptBounds.position.x, ptBounds.position.y });
        m_hintPointsText->setPosition({ elementStartX, currentElementY });
        currentElementY += ptBounds.size.y + ptBounds.position.y + hintElementSpacing; // Advance Y for the next element
    }

    // --- Helper lambda to position a hint button and its texts ---
    auto positionHintUIElements =
        [&](RoundedRectangleShape& buttonShape,
            const std::unique_ptr<sf::Text>& buttonTextElem,
            const std::unique_ptr<sf::Text>& costTextElem)
        {
            sf::Vector2f buttonPos = { elementStartX, currentElementY };
            buttonShape.setSize(hintButtonSize);
            buttonShape.setRadius(hintButtonRadius);
            buttonShape.setOrigin({ 0.f, 0.f });
            buttonShape.setPosition(buttonPos);

            if (buttonTextElem) {
                const unsigned int btnFontSize = static_cast<unsigned int>(std::max(8.0f, hintUiInternalScale * 18.f));
                buttonTextElem->setCharacterSize(btnFontSize);
                sf::FloatRect txtBounds = buttonTextElem->getLocalBounds();
                buttonTextElem->setOrigin({ txtBounds.position.x + txtBounds.size.x / 2.f,
                                            txtBounds.position.y + txtBounds.size.y / 2.f });
                buttonTextElem->setPosition(buttonPos + hintButtonSize / 2.f);
            }
            if (costTextElem) {
                const unsigned int costFontSize = static_cast<unsigned int>(std::max(8.0f, hintUiInternalScale * 16.f));
                costTextElem->setCharacterSize(costFontSize);
                sf::FloatRect costBounds = costTextElem->getLocalBounds();
                costTextElem->setOrigin({ costBounds.position.x, costBounds.position.y + costBounds.size.y / 2.f });
                float costTextX = buttonPos.x + hintButtonSize.x + costTextOffsetX;
                float costTextY = buttonPos.y + hintButtonSize.y / 2.f;
                costTextElem->setPosition({ costTextX, costTextY });
            }
            currentElementY += hintButtonSize.y + hintElementSpacing; // Advance Y for the next button
        };

    // Position Hint 1 (RevealFirst "Letter")
    positionHintUIElements(m_hintRevealFirstButtonShape, m_hintRevealFirstButtonText, m_hintRevealFirstCostText);

    // Position Hint 2 (RevealRandom "Random")
    positionHintUIElements(m_hintRevealRandomButtonShape, m_hintRevealRandomButtonText, m_hintRevealRandomCostText);

    // Position Hint 3 (RevealLast "Last Word")
    positionHintUIElements(m_hintRevealLastButtonShape, m_hintRevealLastButtonText, m_hintRevealLastCostText);

    // Position Hint 4 (RevealFirstOfEach "1st of Each")
    positionHintUIElements(m_hintRevealFirstOfEachButtonShape, m_hintRevealFirstOfEachButtonText, m_hintRevealFirstOfEachCostText);

    // --- Update DEBUG Zone Shapes ---
    if (m_showDebugZones) {
        float scaledOutlineThickness = S(this, 2.0f);

        m_debugGridZoneShape.setPosition({ GRID_ZONE_RECT_DESIGN.position.x, GRID_ZONE_RECT_DESIGN.position.y });
        m_debugGridZoneShape.setSize({ GRID_ZONE_RECT_DESIGN.size.x, GRID_ZONE_RECT_DESIGN.size.y });
        m_debugGridZoneShape.setOutlineThickness(scaledOutlineThickness);

        m_debugHintZoneShape.setPosition({ HINT_ZONE_RECT_DESIGN.position.x, HINT_ZONE_RECT_DESIGN.position.y });
        m_debugHintZoneShape.setSize({ HINT_ZONE_RECT_DESIGN.size.x, HINT_ZONE_RECT_DESIGN.size.y });
        m_debugHintZoneShape.setOutlineThickness(scaledOutlineThickness);

        m_debugWheelZoneShape.setPosition({ WHEEL_ZONE_RECT_DESIGN.position.x, WHEEL_ZONE_RECT_DESIGN.position.y });
        m_debugWheelZoneShape.setSize({ WHEEL_ZONE_RECT_DESIGN.size.x, WHEEL_ZONE_RECT_DESIGN.size.y });
        m_debugWheelZoneShape.setOutlineThickness(scaledOutlineThickness);

        m_debugScoreZoneShape.setPosition({ SCORE_ZONE_RECT_DESIGN.position.x, SCORE_ZONE_RECT_DESIGN.position.y });
        m_debugScoreZoneShape.setSize({ SCORE_ZONE_RECT_DESIGN.size.x, SCORE_ZONE_RECT_DESIGN.size.y });
        m_debugScoreZoneShape.setOutlineThickness(scaledOutlineThickness);

        m_debugTopBarZoneShape.setPosition({ TOP_BAR_ZONE_DESIGN.position.x, TOP_BAR_ZONE_DESIGN.position.y });
        m_debugTopBarZoneShape.setSize({ TOP_BAR_ZONE_DESIGN.size.x, TOP_BAR_ZONE_DESIGN.size.y });
        m_debugTopBarZoneShape.setOutlineThickness(scaledOutlineThickness);
    }

    // --- Final Summary Log ---
    if (sizeChanged) {
        // ... (existing summary log) ...
        m_lastLayoutSize = windowSize;
    }
}
// ***** END OF COMPLETE Game::m_updateLayout FUNCTION (Corrected Rect Access) *****


sf::Vector2f Game::m_tilePos(int wordIdx, int charIdx) {
    sf::Vector2f result = { -1000.f, -1000.f }; // Default off-screen

    if (m_sorted.empty() || wordIdx < 0 || static_cast<size_t>(wordIdx) >= m_wordCol.size() ||
        static_cast<size_t>(wordIdx) >= m_wordRow.size() || charIdx < 0) {
        return result;
    }
    int c = m_wordCol[wordIdx];
    int r = m_wordRow[wordIdx];
    if (c < 0 || static_cast<size_t>(c) >= m_colXOffset.size()) {
        return result;
    }

    // --- Calculate Tile/Padding/Step Sizes using the gridElementsScaleFactor ---
    // m_currentGridLayoutScale now stores the uniform scale factor for grid elements.
    const float scaledTileSize = TILE_SIZE * m_currentGridLayoutScale;
    const float scaledTilePad = TILE_PAD * m_currentGridLayoutScale;
    // Column padding is handled by m_colXOffset structure.

    const float scaledStepWidth = scaledTileSize + scaledTilePad;
    const float scaledStepHeight = scaledTileSize + scaledTilePad;

    // m_colXOffset[c] is already the absolute starting X for this column in design space.
    // m_gridStartY is the absolute starting Y for the grid content area in design space.
    float x = m_colXOffset[c] + static_cast<float>(charIdx) * scaledStepWidth;
    float y = m_gridStartY + static_cast<float>(r) * scaledStepHeight;

    result = { x, y };
    return result;
}
// ***** END OF COMPLETE Game::m_tilePos FUNCTION *****

void Game::m_clearDragState() {
    m_dragging = false;
    m_path.clear();
    m_currentGuess.clear();
}

void Game::m_handleMainMenuEvents(const sf::Event& event) {
    if (const auto* mb = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mb->button != sf::Mouse::Button::Left) return;
        sf::Vector2f mp = m_window.mapPixelToCoords(mb->position);

        if (m_casualButtonShape.getGlobalBounds().contains(mp)) {
            m_clickSound->play(); // Use ->
            // Switch screen - Game will start via this menu later
            m_currentScreen = GameScreen::CasualMenu; // Just switch for now
            // m_rebuild(); // DON'T rebuild here, wait for difficulty selection
        }
        else if (m_competitiveButtonShape.getGlobalBounds().contains(mp)) {
            m_clickSound->play(); // Use ->
            m_currentScreen = GameScreen::CompetitiveMenu; // Just switch for now
            // m_rebuild(); // DON'T rebuild here, wait for difficulty selection
        }
        else if (m_quitButtonShape.getGlobalBounds().contains(mp)) {
            m_clickSound->play(); // Use ->
            m_window.close(); // Quit game
        }
    }
    // Handle other menu events like keyboard navigation if desired
}

void Game::m_handleCasualMenuEvents(const sf::Event& event) {
    if (const auto* mb = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mb->button != sf::Mouse::Button::Left) return;
        sf::Vector2f mp = m_window.mapPixelToCoords(mb->position);

        DifficultyLevel selected = DifficultyLevel::None; // Temp variable
        int puzzles = 0;

        if (m_easyButtonShape.getGlobalBounds().contains(mp)) {
            selected = DifficultyLevel::Easy;
            puzzles = EASY_PUZZLE_COUNT; // Or use a constant EASY_PUZZLE_COUNT
        }
        else if (m_mediumButtonShape.getGlobalBounds().contains(mp)) {
            selected = DifficultyLevel::Medium;
            puzzles = MEDIUM_PUZZLE_COUNT; // MEDIUM_PUZZLE_COUNT
        }
        else if (m_hardButtonShape.getGlobalBounds().contains(mp)) {
            selected = DifficultyLevel::Hard;
            puzzles = HARD_PUZZLE_COUNT; // HARD_PUZZLE_COUNT
        }
        else if (m_returnButtonShape.getGlobalBounds().contains(mp)) {
            if (m_clickSound) m_clickSound->play();
            m_currentScreen = GameScreen::MainMenu; // Go back
            return; // Exit early
        }

        // If a difficulty button was pressed:
        if (selected != DifficultyLevel::None) {
            if (m_clickSound) m_clickSound->play();

            // --- Set Session State ---
            m_selectedDifficulty = selected;
            m_puzzlesPerSession = puzzles;
            m_currentPuzzleIndex = 0; // Start at the first puzzle (index 0)
            m_isInSession = true;
            m_usedBaseWordsThisSession.clear();
            // ------------------------

            m_rebuild(); // Rebuild game for the FIRST puzzle of the session
            m_currentScreen = GameScreen::Playing; // Switch to playing screen
        }
    }
}

// In Game.cpp (add this new function definition)

Game::PuzzleCriteria Game::m_getCriteriaForCurrentPuzzle() const {
    PuzzleCriteria criteria;
    bool isLastPuzzle = (m_currentPuzzleIndex == m_puzzlesPerSession - 1);

    switch (m_selectedDifficulty) {
    case DifficultyLevel::Easy:
        criteria.allowedRarities = { 1 }; // Primarily Rarity 1
        if (isLastPuzzle) {
            criteria.allowedLengths = { 6 }; // Last puzzle is 6 letters
        }
        else {
            criteria.allowedLengths = { 4, 5 }; // Regular puzzles are 4 or 5
        }
        break;

    case DifficultyLevel::Medium:
        criteria.allowedRarities = { 1, 2, 3 }; // Rarity 1, 2, or 3
        if (isLastPuzzle) {
            criteria.allowedLengths = { 7 }; // Last puzzle is 7 letters
            // Optional: Could tighten rarity for last word, e.g. {2, 3}
        }
        else {
            criteria.allowedLengths = { 4, 5, 6 }; // Regular puzzles
        }
        break;

    case DifficultyLevel::Hard:
        if (isLastPuzzle) {
            criteria.allowedLengths = { 7 };   // Last puzzle is 7 letters
            criteria.allowedRarities = { 4 };  // Last puzzle is Rarity 4
        }
        else {
            criteria.allowedLengths = { 5, 6, 7 }; // Regular puzzles
            criteria.allowedRarities = { 3, 4 };   // Rarity 3 or 4
        }
        break;

    case DifficultyLevel::None: // Fallback or other modes
    default:
        // Default criteria (e.g., allow a wide range if not in a session)
        criteria.allowedLengths = { 4, 5, 6, 7 }; // Or use WORD_LENGTH constant range
        criteria.allowedRarities = { 1, 2, 3, 4 }; // All rarities
        break;
    }
    return criteria;
}

void Game::m_renderMainMenu(const sf::Vector2f& mousePos) {
    // Apply theme colors
    m_mainMenuBg.setFillColor(m_currentTheme.menuBg);
    m_mainMenuTitle->setFillColor(m_currentTheme.menuTitleText);
    m_casualButtonText->setFillColor(m_currentTheme.menuButtonText); // Use -> for unique_ptr
    m_competitiveButtonText->setFillColor(m_currentTheme.menuButtonText);
    m_quitButtonText->setFillColor(m_currentTheme.menuButtonText);

    // Handle button hover colors
    m_casualButtonShape.setFillColor(m_casualButtonShape.getGlobalBounds().contains(mousePos) ? m_currentTheme.menuButtonHover : m_currentTheme.menuButtonNormal);
    m_competitiveButtonShape.setFillColor(m_competitiveButtonShape.getGlobalBounds().contains(mousePos) ? m_currentTheme.menuButtonHover : m_currentTheme.menuButtonNormal);
    m_quitButtonShape.setFillColor(m_quitButtonShape.getGlobalBounds().contains(mousePos) ? m_currentTheme.menuButtonHover : m_currentTheme.menuButtonNormal);

    // Draw Main Menu elements
    m_window.draw(m_mainMenuBg);
    m_window.draw(*m_mainMenuTitle); // Use * to dereference unique_ptr for drawing
    m_window.draw(m_casualButtonShape);
    m_window.draw(*m_casualButtonText);
    m_window.draw(m_competitiveButtonShape);
    m_window.draw(*m_competitiveButtonText);
    m_window.draw(m_quitButtonShape);
    m_window.draw(*m_quitButtonText);
}

void Game::m_renderCasualMenu(const sf::Vector2f& mousePos) {
    // Apply theme colors
    m_casualMenuBg.setFillColor(m_currentTheme.menuBg);
    m_casualMenuTitle->setFillColor(m_currentTheme.menuTitleText);
    m_easyButtonText->setFillColor(m_currentTheme.menuButtonText);
    m_mediumButtonText->setFillColor(m_currentTheme.menuButtonText);
    m_hardButtonText->setFillColor(m_currentTheme.menuButtonText);
    m_returnButtonText->setFillColor(m_currentTheme.menuButtonText);

    // Handle button hover colors
    m_easyButtonShape.setFillColor(m_easyButtonShape.getGlobalBounds().contains(mousePos) ? m_currentTheme.menuButtonHover : m_currentTheme.menuButtonNormal);
    m_mediumButtonShape.setFillColor(m_mediumButtonShape.getGlobalBounds().contains(mousePos) ? m_currentTheme.menuButtonHover : m_currentTheme.menuButtonNormal);
    m_hardButtonShape.setFillColor(m_hardButtonShape.getGlobalBounds().contains(mousePos) ? m_currentTheme.menuButtonHover : m_currentTheme.menuButtonNormal);
    m_returnButtonShape.setFillColor(m_returnButtonShape.getGlobalBounds().contains(mousePos) ? m_currentTheme.menuButtonHover : m_currentTheme.menuButtonNormal);

    // Draw Casual Menu elements
    m_window.draw(m_casualMenuBg);
    m_window.draw(*m_casualMenuTitle);
    m_window.draw(m_easyButtonShape);
    m_window.draw(*m_easyButtonText);
    m_window.draw(m_mediumButtonShape);
    m_window.draw(*m_mediumButtonText);
    m_window.draw(m_hardButtonShape);
    m_window.draw(*m_hardButtonText);
    m_window.draw(m_returnButtonShape);
    m_window.draw(*m_returnButtonText);

    // TODO: Draw pop-up if hovering
}

// --- Event Handlers for Specific Screens ---

void Game::m_handlePlayingEvents(const sf::Event& event) {
    // --- Handle Window Close ---
    // (Redundant if handled globally in m_processEvents, but safe to keep)
    if (event.is<sf::Event::Closed>()) {
        m_window.close();
        return;
    }

    // Window Resized
    if (const auto* rs = event.getIf<sf::Event::Resized>()) {
        unsigned int newWidth = rs->size.x;
        unsigned int newHeight = rs->size.y;
        bool changed = false;

        // Check against minimums
        if (newWidth < MIN_WINDOW_WIDTH) { /* clamp width */ changed = true; }
        if (newHeight < MIN_WINDOW_HEIGHT) { /* clamp height */ changed = true; }

        if (changed) {
            std::cout << "DEBUG: Window too small, resizing to " << newWidth << "x" << newHeight << std::endl;
            m_window.setSize({ newWidth, newHeight });
        }

        // --- Update View and Layout using CLAMPED size ---
        sf::Vector2u clampedSize = { newWidth, newHeight }; // Store the clamped size

        sf::FloatRect visibleArea({ 0.f, 0.f }, { static_cast<float>(clampedSize.x), static_cast<float>(clampedSize.y) });
        m_window.setView(sf::View(visibleArea));

        // *** PASS the clamped size to m_updateLayout ***
        m_updateLayout(clampedSize);
        // ***********************************************

    } // --- End Resized Handling ---


    // Only process game input if not solved (already checked by screen state, but double-check internal state)
    if (m_gameState != GState::Playing) {
        return; // Don't process game input if already solved internally
    }

    // --- Mouse Button Pressed ---
    if (const auto* mb = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mb->button == sf::Mouse::Button::Left) {
            sf::Vector2f mp = m_window.mapPixelToCoords(mb->position); // Mapped mouse position

            // Check Return to Menu Button
            if (m_returnToMenuButtonShape.getGlobalBounds().contains(mp)) {
                if (m_clickSound) m_clickSound->play();
                m_currentScreen = GameScreen::MainMenu;
                m_backgroundMusic.stop();
                m_isInSession = false;
                m_selectedDifficulty = DifficultyLevel::None;
                m_clearDragState();
                return; // Processed button click
            }

            // Check Scramble Button
            if (m_scrambleSpr && m_scrambleSpr->getGlobalBounds().contains(mp)) {
                if (m_clickSound) m_clickSound->play();
                std::shuffle(m_base.begin(), m_base.end(), Rng());
                // m_updateLayout(m_window.getSize()); // m_updateLayout is called in main loop's update if needed
                                                    // or specifically after m_base changes if that's the only trigger.
                                                    // For now, let's assume layout updates handle it.
                m_clearDragState();
                return; // Processed button click
            }

            // Check Hint Buttons
            bool hintButtonClicked = false;

            // Hint 1: Reveal First Letter
            if (!hintButtonClicked && m_hintRevealFirstButtonShape.getGlobalBounds().contains(mp)) {
                if (m_hintPoints >= HINT_COST_REVEAL_FIRST) {
                    std::cout << "DEBUG: Clicked Hint 1 (Reveal First)" << std::endl;
                    m_hintPoints -= HINT_COST_REVEAL_FIRST;
                    m_activateHint(HintType::RevealFirst);
                }
                else {
                    std::cout << "DEBUG: Clicked Hint 1, but cannot afford." << std::endl;
                    if (m_errorWordSound) m_errorWordSound->play();
                }
                hintButtonClicked = true;
            }

            // Hint 2: Reveal Random Letters
            if (!hintButtonClicked && m_hintRevealRandomButtonShape.getGlobalBounds().contains(mp)) {
                if (m_hintPoints >= HINT_COST_REVEAL_RANDOM) {
                    std::cout << "DEBUG: Clicked Hint 2 (Reveal Random)" << std::endl;
                    m_hintPoints -= HINT_COST_REVEAL_RANDOM;
                    m_activateHint(HintType::RevealRandom);
                }
                else {
                    std::cout << "DEBUG: Clicked Hint 2, but cannot afford." << std::endl;
                    if (m_errorWordSound) m_errorWordSound->play();
                }
                hintButtonClicked = true;
            }

            // Hint 3: Reveal Last Word
            if (!hintButtonClicked && m_hintRevealLastButtonShape.getGlobalBounds().contains(mp)) {
                if (m_hintPoints >= HINT_COST_REVEAL_LAST) {
                    std::cout << "DEBUG: Clicked Hint 3 (Reveal Last)" << std::endl;
                    m_hintPoints -= HINT_COST_REVEAL_LAST;
                    m_activateHint(HintType::RevealLast);
                }
                else {
                    std::cout << "DEBUG: Clicked Hint 3, but cannot afford." << std::endl;
                    if (m_errorWordSound) m_errorWordSound->play();
                }
                hintButtonClicked = true;
            }

            // Hint 4: Reveal First of Each
            if (!hintButtonClicked && m_hintRevealFirstOfEachButtonShape.getGlobalBounds().contains(mp)) {
                if (m_hintPoints >= HINT_COST_REVEAL_FIRST_OF_EACH) {
                    std::cout << "DEBUG: Clicked Hint 4 (Reveal First of Each)" << std::endl;
                    m_hintPoints -= HINT_COST_REVEAL_FIRST_OF_EACH;
                    m_activateHint(HintType::RevealFirstOfEach);
                }
                else {
                    std::cout << "DEBUG: Clicked Hint 4 (First of Each), but cannot afford." << std::endl;
                    if (m_errorWordSound) m_errorWordSound->play();
                }
                hintButtonClicked = true;
            }


            if (hintButtonClicked) {
                // m_updateLayout(m_window.getSize()); // Consider if hint usage needs immediate layout update for hint points text
                return; // Processed a hint button click
            }
            // --- End Hint Button Checks ---


            // --- START: DIAGNOSTIC LOGGING FOR WHEEL CLICK ---
            std::cout << "[Click] Mouse Coords: X=" << mp.x << " Y=" << mp.y << std::endl;
            std::cout << "[Click] m_currentLetterRenderRadius: " << m_currentLetterRenderRadius << std::endl;
            if (!m_wheelLetterRenderPos.empty() && !m_base.empty()) { // Ensure there are letters to check
                // Log for the first letter as an example
                std::cout << "[Click] First Letter Target (" << m_base[0] << "): X=" << m_wheelLetterRenderPos[0].x
                    << " Y=" << m_wheelLetterRenderPos[0].y << std::endl;
                float distSqDebug = distSq(mp, m_wheelLetterRenderPos[0]);
                float radiusSqDebug = m_currentLetterRenderRadius * m_currentLetterRenderRadius;
                std::cout << "[Click] DistSq to first letter: " << distSqDebug
                    << ", RadiusSq target: " << radiusSqDebug
                    << (distSqDebug < radiusSqDebug ? " (INSIDE)" : " (OUTSIDE)") << std::endl;
            }
            else {
                std::cout << "[Click] m_wheelLetterRenderPos or m_base is empty." << std::endl;
            }
            // --- END: DIAGNOSTIC LOGGING FOR WHEEL CLICK ---


            // --- Check Letter Wheel Click ---
            // Loop up to m_base.size() as m_wheelLetterRenderPos should be sized accordingly.
            for (std::size_t i = 0; i < m_base.size(); ++i) {
                if (i >= m_wheelLetterRenderPos.size()) { // Safety break if sizes somehow mismatch
                    std::cerr << "Warning: m_base.size() and m_wheelLetterRenderPos.size() mismatch in click check!" << std::endl;
                    break;
                }

                if (distSq(mp, m_wheelLetterRenderPos[i]) < m_currentLetterRenderRadius * m_currentLetterRenderRadius) {
                    std::cout << "[Click] SUCCESS: Hit letter " << m_base[i] << " (index " << i << ")" << std::endl;
                    m_dragging = true;
                    m_path.clear();
                    m_path.push_back(static_cast<int>(i)); // Store index of letter in m_base
                    m_currentGuess += static_cast<char>(std::toupper(m_base[i]));
                    if (m_selectSound) m_selectSound->play();
                    return; // Started drag
                }
            }
            // If no wheel letter was clicked, and no button was clicked, this click did nothing relevant.
            // std::cout << "[Click] No UI element hit by this click." << std::endl; // Optional: for more detailed logging

        } // End Left Mouse Button Check
    } // End Mouse Button Pressed Check


    // --- Mouse Moved ---
    else if (const auto* mm = event.getIf<sf::Event::MouseMoved>()) {
        if (m_dragging) {
            sf::Vector2f mp = m_window.mapPixelToCoords(mm->position);
            // No need for actionTaken flag here if we only process the first hovered letter.
            // However, the original logic allowed adding a letter if not in path, or backtracking.
            // Let's keep the structure that allows for adding/removing if mouse hovers over a letter.

            // Iterate based on the number of letters in your base word.
            // m_wheelLetterRenderPos should be sized accordingly by m_updateLayout.
            for (std::size_t i = 0; i < m_base.size(); ++i) {
                if (i >= m_wheelLetterRenderPos.size()) { // Safety check
                    std::cerr << "Warning: m_base.size() and m_wheelLetterRenderPos.size() mismatch in MouseMoved!" << std::endl;
                    break;
                }

                // Check if mouse is inside the visual circle for letter 'i'
                if (distSq(mp, m_wheelLetterRenderPos[i]) < m_currentLetterRenderRadius * m_currentLetterRenderRadius) {

                    int letterIndexInMPath = static_cast<int>(i); // The index 'i' directly corresponds to m_base

                    auto it = std::find(m_path.begin(), m_path.end(), letterIndexInMPath);
                    bool alreadyInPath = (it != m_path.end());

                    if (!alreadyInPath) {
                        // --- Add new letter to path ---
                        m_path.push_back(letterIndexInMPath);
                        m_currentGuess += static_cast<char>(std::toupper(m_base[i])); // Use m_base[i]
                        if (m_selectSound) m_selectSound->play();
                        // No need for actionTaken if we allow multiple letters to be evaluated per mouse move,
                        // though usually, you only want to act on the *first one* you enter.
                        // If you only want one action per mouse move event:
                        // actionTaken = true; break; // Then uncomment actionTaken bool declaration
                        // For smooth dragging, it's usually fine to process any letter you enter.
                    }
                    else {
                        // --- Letter is already in path - Check for backtracking ---
                        // Condition: Path has at least 2 letters AND
                        //            we are hovering over the second-to-last letter ADDED to m_path
                        if (m_path.size() >= 2 && m_path[m_path.size() - 2] == letterIndexInMPath) {
                            // Remove the *last* element from path and guess
                            m_path.pop_back();
                            if (!m_currentGuess.empty()) {
                                m_currentGuess.pop_back();
                            }
                            // Optional: Play an "unselect" sound
                            // actionTaken = true; break; // If only one action per move event
                        }
                        // Else: Hovering over current last letter, or some other letter already in path
                        // (but not the one that enables backtracking) -> Do nothing.
                    }
                    // If you only want to process the FIRST letter the mouse is over in this event frame:
                    // break; // Exit the for loop after handling one letter.
                    // If you don't break, and mouse is large enough to cover two letters, both might be processed.
                    // For typical dragging, breaking after the first interaction is common.
                    // Let's assume for now we process the first letter we find ourselves over.
                    break;
                }
            }
        }
    } // End Mouse Moved Check


// --- Mouse Button Released ---
    else if (const auto* mb = event.getIf<sf::Event::MouseButtonReleased>()) {
        if (m_dragging && mb->button == sf::Mouse::Button::Left) {

            const int MIN_GUESS_LENGTH = 3; // Minimum letters required for a valid word attempt
            if (m_currentGuess.length() < MIN_GUESS_LENGTH) {
                // Guess is too short (1 or 2 letters), just clear the state and do nothing else.
                std::cout << "DEBUG: Ignoring short guess (length " << m_currentGuess.length() << "): '" << m_currentGuess << "'" << std::endl;
                m_clearDragState(); // Clear path, guess, and reset dragging flag
                return;             // Exit the handler for this event, preventing failure sound/logic
            }

            bool actionTaken = false; // Flag: Did we find a new word or trigger a flourish?
            std::string wordMatched = ""; // Store the original case of the matched word
            int wordIndexMatched = -1; // Store index if it was a grid word

            std::cout << "DEBUG: Mouse Released. Processing Guess (Length >= " << MIN_GUESS_LENGTH << "): '" << m_currentGuess << "'" << std::endl;

            // --- Phase 1: Check against GRID words ---
            for (std::size_t w = 0; w < m_sorted.size(); ++w) {
                const std::string& solutionOriginalCase = m_sorted[w].text;
                std::string solutionUpper = solutionOriginalCase;
                std::transform(solutionUpper.begin(), solutionUpper.end(), solutionUpper.begin(), ::toupper);

                if (solutionUpper == m_currentGuess) {
                    wordMatched = solutionOriginalCase; // Store the matched word (original case)
                    wordIndexMatched = static_cast<int>(w); // Store its index

                    if (m_found.count(solutionOriginalCase)) {
                        // --- Repeated GRID Word ---
                        std::cout << "DEBUG: Matched GRID word '" << solutionOriginalCase << "', but already found." << std::endl;
                        // Trigger flourish for existing grid letters
                        for (int c = 0; c < solutionOriginalCase.length(); ++c) {
                            m_gridFlourishes.push_back({ wordIndexMatched, c, GRID_FLOURISH_DURATION });
                        }
                        if (m_placeSound) m_placeSound->play(); // Use error/repeat sound
                        actionTaken = true;
                    }
                    else {
                        // --- NEW Grid Word Found ---
                        std::cout << "DEBUG: Found NEW match on GRID: '" << solutionOriginalCase << "'" << std::endl;
                        m_found.insert(solutionOriginalCase);
                        // ... (Scoring, Hint Check, Letter Animations - keep this existing logic) ...
                        int baseScore = static_cast<int>(m_currentGuess.length()) * 10;
                        int rarityBonus = (m_sorted[w].rarity > 1) ? (m_sorted[w].rarity * 25) : 0;
                        int wordScoreForThisWord = baseScore + rarityBonus;

                        m_currentScore += wordScoreForThisWord;
                        m_spawnScoreFlourish(wordScoreForThisWord, static_cast<int>(w));

                        if (m_scoreValueText) m_scoreValueText->setString(std::to_string(m_currentScore));
                        m_wordsSolvedSinceHint++;
                        if (m_wordsSolvedSinceHint >= WORDS_PER_HINT) {
                            m_hintsAvailable++; m_wordsSolvedSinceHint = 0;
                            if (m_hintCountTxt) m_hintCountTxt->setString("Hints: " + std::to_string(m_hintsAvailable));
                        }
                        for (std::size_t c = 0; c < m_currentGuess.length(); ++c) {
                            if (c < m_path.size()) { // m_path stores indices of letters from m_base
                                int pathNodeIdx = m_path[c]; // This is the index in m_base
                                // Ensure pathNodeIdx is valid for m_wheelLetterRenderPos (it should be if m_base matches)
                                if (pathNodeIdx >= 0 && static_cast<size_t>(pathNodeIdx) < m_wheelLetterRenderPos.size() &&
                                    static_cast<size_t>(pathNodeIdx) < m_base.size()) {

                                    sf::Vector2f startPos = m_wheelLetterRenderPos[pathNodeIdx]; // Animation STARTS from visual letter pos
                                    sf::Vector2f endPos = m_tilePos(wordIndexMatched, static_cast<int>(c)); // wordIndexMatched is index in m_sorted

                                    // Critical: Adjust endPos to be the CENTER of the tile for visual appeal
                                    // Assuming m_tilePos gives top-left of the tile
                                    float finalRenderTileSize = TILE_SIZE * m_currentGridLayoutScale; // Get current scaled tile size
                                    endPos.x += finalRenderTileSize / 2.f;
                                    endPos.y += finalRenderTileSize / 2.f;

                                    // Ensure m_currentGuess[c] is correct char, wordIndexMatched for m_sorted, c for char in word
                                    m_anims.push_back({
                                        m_currentGuess[c], // Character being animated
                                        startPos,
                                        endPos,
                                        0.f - (c * 0.03f), // Staggered start time
                                        wordIndexMatched,      // Word index in m_grid / m_sorted
                                        static_cast<int>(c),   // Character index in that word
                                        AnimTarget::Grid
                                        });
                                }
                            }
                        }
                        std::cout << "GRID Word: " << m_currentGuess << " | Rarity: " << m_sorted[wordIndexMatched].rarity << " | Len: " << m_currentGuess.length() << " | Rarity Bonus: " << rarityBonus << " | BasePts: " << baseScore << " | Total: " << m_currentScore << std::endl;
                        // Check for Puzzle Solved
                        if (m_found.size() == m_solutions.size()) {
                            std::cout << "DEBUG: All grid words found! Puzzle solved." << std::endl;
                            if (m_winSound) m_winSound->play();
                            m_gameState = GState::Solved;
                            m_currentScreen = GameScreen::GameOver;
                            m_updateLayout(m_window.getSize());
                        }
                        // ... (End of new grid word logic) ...
                        actionTaken = true;
                    }
                    goto process_outcome; // Found a grid match (new or repeat), skip bonus check
                }
            } // --- End Grid Check Loop ---


            // --- Phase 2: Check against BONUS words (only if no grid match occurred) ---
            std::cout << "DEBUG: Checking for BONUS word..." << std::endl;
            for (const auto& potentialWordInfo : m_allPotentialSolutions) {
                const std::string& bonusWordOriginalCase = potentialWordInfo.text;
                if (m_found.count(bonusWordOriginalCase)) { continue; } // Already handled by grid check

                std::string bonusWordUpper = bonusWordOriginalCase;
                std::transform(bonusWordUpper.begin(), bonusWordUpper.end(), bonusWordUpper.begin(), ::toupper);

                if (bonusWordUpper == m_currentGuess) {
                    wordMatched = bonusWordOriginalCase;

                    if (m_foundBonusWords.count(bonusWordOriginalCase)) {
                        // --- Repeated BONUS Word ---
                        std::cout << "DEBUG: Matched BONUS word '" << bonusWordOriginalCase << "', but already found AS BONUS." << std::endl;
                        m_bonusTextFlourishTimer = BONUS_TEXT_FLOURISH_DURATION;
                        if (m_placeSound) m_placeSound->play();
                        actionTaken = true;
                    }
                    else {
                        // --- NEW Bonus Word Found ---
                        std::cout << "DEBUG: Found NEW match for BONUS: '" << bonusWordOriginalCase << "'" << std::endl;
                        m_foundBonusWords.insert(bonusWordOriginalCase);

                        m_hintPoints++; // <<< THIS LINE WAS ALREADY THERE FROM PREVIOUS WORK
                        std::cout << "DEBUG: Hint Points increased to: " << m_hintPoints << std::endl; // <<< THIS LINE WAS ALREADY THERE

                        // --- Spawn the "+1" animation for the hint point ---
                        // Calculate an approximate center Y for where the "Bonus Words: X/Y" text is displayed.
                        // This is based on the HUD layout logic.
                        // Adjust S(this, 25.f) or S(this, HUD_TEXT_OFFSET_Y) if the "Bonus Words" text is higher/lower.
                        float bonusTextApproxY = m_wheelY + (m_currentWheelRadius + S(this, 30.f)) /* visual wheel bottom edge */
                            + S(this, HUD_TEXT_OFFSET_Y)    /* gap below wheel */
                            + S(this, 20.f)                 /* approx height of "Found: X/Y" */
                            + S(this, HUD_LINE_SPACING)     /* space after "Found: X/Y" */
                            + S(this, 10.f);                /* approx half-height of "Bonus: X/Y" itself */
                        sf::Vector2f hintAnimStartPos = { m_wheelX, bonusTextApproxY };
                        m_spawnHintPointAnimation(hintAnimStartPos); // <<< --- ADD THIS LINE ---

                        // Existing logic for bonus score and letter animations to score bar
                        int bonusScore = 25; m_currentScore += bonusScore;
                        if (m_scoreValueText) m_scoreValueText->setString(std::to_string(m_currentScore));
                        std::cout << "BONUS Word: " << m_currentGuess << " | Points: " << bonusScore << " | Total: " << m_currentScore << std::endl;

                        if (m_scoreValueText) {
                            sf::FloatRect scoreBounds = m_scoreValueText->getGlobalBounds();
                            sf::Vector2f scoreCenterPos = { scoreBounds.position.x + scoreBounds.size.x / 2.f, 
                                                            scoreBounds.position.y + scoreBounds.size.y / 2.f  
                            };
                            for (std::size_t c = 0; c < m_currentGuess.length(); ++c) {
                                if (c < m_path.size()) {
                                    int pathNodeIdx = m_path[c];
                                    if (pathNodeIdx >= 0 && pathNodeIdx < m_wheelCentres.size()) {
                                        sf::Vector2f startPos = m_wheelCentres[pathNodeIdx];
                                        m_anims.push_back({ m_currentGuess[c], startPos, scoreCenterPos, 0.f - (c * 0.05f), -1, -1, AnimTarget::Score });
                                    }
                                }
                            }
                            std::cout << "DEBUG: Bonus animations CREATED." << std::endl;
                        }
                        else {
                            std::cerr << "Warning: Cannot animate bonus to score - m_scoreValueText is null." << std::endl;
                        }

                        m_bonusTextFlourishTimer = BONUS_TEXT_FLOURISH_DURATION; // This flourishes the "Bonus Words: X/Y" text itself
                        actionTaken = true;
                    }
                    goto process_outcome;
                }
            } // --- End Bonus Check Loop ---


            // --- Phase 3: Incorrect Word ---
            // This block is only reached if the guess didn't match ANY grid or potential bonus word text
            std::cout << "Word '" << m_currentGuess << "' is not valid for this puzzle." << std::endl;
            if (m_errorWordSound) m_errorWordSound->play();
            actionTaken = true;


        process_outcome:; // Label for jumps after processing a match

            m_clearDragState(); // Clear path and guess regardless of outcome
        }
        } // --- End Mouse Button Released ---

} // End m_handlePlayingEvents


void Game::m_handleGameOverEvents(const sf::Event& event) {
    // --- Handle Window Close ---
    // (Redundant if handled globally in m_processEvents, but safe to keep)
    if (event.is<sf::Event::Closed>()) {
        m_window.close();
        return;
    }

    // --- Handle Window Resize ---
    // (Redundant if handled globally in m_processEvents, but safe to keep)
    if (const auto* rs = event.getIf<sf::Event::Resized>()) {
        sf::FloatRect visibleArea(
            { 0.f, 0.f }, // Top-left position as sf::Vector2f
            { static_cast<float>(rs->size.x), static_cast<float>(rs->size.y) } // Size as sf::Vector2f
        );
        m_window.setView(sf::View(visibleArea));
        m_updateLayout(m_window.getSize()); // Recalculate layout for the game over screen too
        return;
    }

    // --- Mouse Button Pressed (for Continue button) ---
    if (const auto* mb = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mb->button == sf::Mouse::Button::Left) {
            sf::Vector2f mp = m_window.mapPixelToCoords(mb->position);

            // Check Return to Menu Button
            if (m_returnToMenuButtonShape.getGlobalBounds().contains(mp)) {
                if (m_clickSound) m_clickSound->play();
                m_currentScreen = GameScreen::MainMenu;
                m_backgroundMusic.stop(); // Example
                m_isInSession = false;    // Example
                m_selectedDifficulty = DifficultyLevel::None; // Example
                m_gameState = GState::Playing; // Reset internal state for menu
                return; // Processed button click
            }

            // Check Continue Button Click
            if (m_contBtn.getGlobalBounds().contains(mp)) {
                if (m_clickSound) m_clickSound->play();

                // --- Session Handling ---
                if (m_isInSession) {
                    m_currentPuzzleIndex++; // Move to the next puzzle index

                    if (m_currentPuzzleIndex < m_puzzlesPerSession) {
                        // --- Go to Next Puzzle ---
                        std::cout << "DEBUG: Continuing Session - Calling m_rebuild for puzzle " << m_currentPuzzleIndex + 1 << std::endl;
                        std::cout << "DEBUG: BEFORE rebuilding puzzle " << m_currentPuzzleIndex + 1
                            << ", m_usedBaseWordsThisSession size = " << m_usedBaseWordsThisSession.size() << std::endl;
                        m_rebuild();
                        m_currentScreen = GameScreen::Playing;
                        m_gameState = GState::Playing;
                    }
                    else 
                    {
                        // --- Session Finished --- Transition to Celebration ---
                        std::cout << "Session Complete! Final Score: " << m_currentScore << ". Starting celebration..." << std::endl;
                        m_isInSession = false;
                        m_selectedDifficulty = DifficultyLevel::None;
                        m_gameState = GState::Playing; // Or maybe a dedicated GState::Celebrating? Using Playing for now.

                        m_startCelebrationEffects(); // <<< Initialize effects
                        m_currentScreen = GameScreen::SessionComplete; // <<< Change screen state
                        // Don't reset score yet, display it on celebration screen

                        // Consider stopping game music and playing victory music?
                        m_backgroundMusic.stop();
                        // if (m_victoryMusic.openFromFile("...")) { m_victoryMusic.play(); }
                    }
                }
                else {
                    // Not in a session (e.g., maybe a future single puzzle mode ended)
                    m_currentScreen = GameScreen::MainMenu;
                    m_gameState = GState::Playing; // Reset internal state for menu
                }
            }
        }
    }


} // End m_handleGameOverEvents

// --- Render Game Screen ---
void Game::m_renderGameScreen(const sf::Vector2f& mousePos) { // mousePos is already mapped to coords

    //------------------------------------------------------------
    //  Calculate common scaled values ONCE (using base m_uiScale)
    //------------------------------------------------------------
    // const float scaledLetterRadius = S(this, LETTER_R); // m_currentLetterRenderRadius is used now
    const float scaledWheelOutlineThickness = S(this, 3.f);
    const float scaledLetterCircleOutline = S(this, 2.f);
    const float scaledPathThickness = S(this, 5.0f);
    const float scaledGuessDisplayGap = S(this, GUESS_DISPLAY_GAP);
    const float scaledGuessDisplayPadX = S(this, 15.f);
    const float scaledGuessDisplayPadY = S(this, 5.f);
    const float scaledGuessDisplayRadius = S(this, 8.f);
    const float scaledGuessDisplayOutline = S(this, 1.f);
    const float scaledHudOffsetY = S(this, HUD_TEXT_OFFSET_Y); // Gap below wheel for HUD
    const float scaledHudLineSpacing = S(this, HUD_LINE_SPACING);

    // Scaled Font Sizes (ensure minimum usable size)
    const unsigned int scaledGridLetterFontSize = static_cast<unsigned int>(std::max(8.0f, S(this, 20.f) * m_currentGridLayoutScale)); // Font scales with grid tiles
    const unsigned int scaledFlyingLetterFontSize = static_cast<unsigned int>(std::max(8.0f, S(this, 20.f)));
    const unsigned int scaledGuessDisplayFontSize = static_cast<unsigned int>(std::max(8.0f, S(this, 30.f)));
    const unsigned int scaledFoundFontSize = static_cast<unsigned int>(std::max(8.0f, S(this, 20.f)));
    const unsigned int scaledBonusFontSize = static_cast<unsigned int>(std::max(8.0f, S(this, 18.f)));
    const unsigned int scaledSolvedFontSize = static_cast<unsigned int>(std::max(8.0f, S(this, 26.f)));
    const unsigned int scaledContinueFontSize = static_cast<unsigned int>(std::max(8.0f, S(this, 24.f)));

    //------------------------------------------------------------
    //  Draw Progress Meter (If in session)
    //------------------------------------------------------------
    if (m_isInSession) {
        m_progressMeterBg.setFillColor(sf::Color(50, 50, 50, 150)); m_progressMeterBg.setOutlineColor(sf::Color(150, 150, 150));
        m_progressMeterBg.setOutlineThickness(S(this, PROGRESS_METER_OUTLINE)); m_progressMeterFill.setFillColor(sf::Color(0, 180, 0, 200));
        float progressRatio = 0.f; if (m_puzzlesPerSession > 0) { progressRatio = static_cast<float>(m_currentPuzzleIndex + 1) / static_cast<float>(m_puzzlesPerSession); }
        float fillWidth = m_progressMeterBg.getSize().x * progressRatio; m_progressMeterFill.setSize({ fillWidth, m_progressMeterBg.getSize().y });
        m_window.draw(m_progressMeterBg); m_window.draw(m_progressMeterFill);
        if (m_progressMeterText) { const unsigned int scaledProgressFontSize = static_cast<unsigned int>(std::max(8.0f, S(this, 16.f))); m_progressMeterText->setCharacterSize(scaledProgressFontSize); std::string progressStr = std::to_string(m_currentPuzzleIndex + 1) + "/" + std::to_string(m_puzzlesPerSession); m_progressMeterText->setString(progressStr); m_progressMeterText->setFillColor(sf::Color::White); sf::FloatRect textBounds = m_progressMeterText->getLocalBounds(); m_progressMeterText->setOrigin({ textBounds.position.x + textBounds.size.x / 2.f, textBounds.position.y + textBounds.size.y / 2.f }); m_progressMeterText->setPosition(m_progressMeterBg.getPosition()); m_window.draw(*m_progressMeterText); }
    }

    // --- Draw Return to Menu Button ---
    if (m_currentScreen == GameScreen::Playing || m_currentScreen == GameScreen::GameOver) {
        bool returnHover = m_returnToMenuButtonShape.getGlobalBounds().contains(mousePos);
        sf::Color returnBaseColor = m_currentTheme.menuButtonNormal;
        sf::Color returnHoverColor = m_currentTheme.menuButtonHover;
        m_returnToMenuButtonShape.setFillColor(returnHover ? returnHoverColor : returnBaseColor);
        m_window.draw(m_returnToMenuButtonShape);
        if (m_returnToMenuButtonText) {
            m_returnToMenuButtonText->setFillColor(m_currentTheme.menuButtonText);
            m_window.draw(*m_returnToMenuButtonText);
        }
    }

    //------------------------------------------------------------
    //  Draw Score Bar
    //------------------------------------------------------------
    //m_scoreBar.setFillColor(m_currentTheme.scoreBarBg); m_scoreBar.setOutlineColor(m_currentTheme.wheelOutline); m_scoreBar.setOutlineThickness(1.f);
    //m_window.draw(m_scoreBar);
    if (m_scoreLabelText) { m_window.draw(*m_scoreLabelText); }
    if (m_scoreValueText) {
        m_scoreValueText->setString(std::to_string(m_currentScore));
        sf::FloatRect currentValueBounds = m_scoreValueText->getLocalBounds();
        m_scoreValueText->setOrigin({ currentValueBounds.position.x + currentValueBounds.size.x / 2.f,
                                         m_scoreValueText->getOrigin().y });
        sf::Vector2f valOriginalPos = m_scoreValueText->getPosition();
        sf::Vector2f valOriginalOrigin = m_scoreValueText->getOrigin();
        sf::Vector2f valOriginalScale = m_scoreValueText->getScale();
        if (m_scoreFlourishTimer > 0.f) {
            float scaleFactor = 1.0f + 0.4f * std::sin((SCORE_FLOURISH_DURATION - m_scoreFlourishTimer) / SCORE_FLOURISH_DURATION * PI);
            sf::FloatRect valLocalBoundsFlourish = m_scoreValueText->getLocalBounds();
            float valCenterX = valLocalBoundsFlourish.position.x + valLocalBoundsFlourish.size.x / 2.f;
            float valCenterY = valLocalBoundsFlourish.position.y + valLocalBoundsFlourish.size.y / 2.f;
            m_scoreValueText->setOrigin({ valCenterX, valCenterY });
            m_scoreValueText->setPosition(sf::Vector2f{ valOriginalPos.x - valOriginalOrigin.x + valCenterX,
                                                       valOriginalPos.y - valOriginalOrigin.y + valCenterY });
            m_scoreValueText->setScale({ scaleFactor, scaleFactor });
        }
        m_window.draw(*m_scoreValueText);
        if (m_scoreFlourishTimer > 0.f) {
            m_scoreValueText->setScale(valOriginalScale);
            m_scoreValueText->setOrigin(valOriginalOrigin);
            m_scoreValueText->setPosition(valOriginalPos);
        }
    }
    //------------------------------------------------------------
    //  Draw letter grid
    //------------------------------------------------------------
    if (!m_sorted.empty()) {
        const float finalRenderTileSize = TILE_SIZE * m_currentGridLayoutScale; // m_currentGridLayoutScale is gridElementsScaleFactor
        const float finalRenderTileRadius = finalRenderTileSize * 0.18f;
        const float tileOutlineRenderThickness = 1.0f; // Fixed or scaled: 1.0f * m_currentGridLayoutScale;

        RoundedRectangleShape tileBackground({ finalRenderTileSize, finalRenderTileSize }, finalRenderTileRadius, 10);
        tileBackground.setOutlineThickness(tileOutlineRenderThickness);
        sf::Text letterText(m_font, "", scaledGridLetterFontSize);

        for (std::size_t w = 0; w < m_sorted.size(); ++w) {
            int wordRarity = m_sorted[w].rarity;
            if (w >= m_grid.size()) continue;
            for (std::size_t c = 0; c < m_sorted[w].text.length(); ++c) {
                if (c >= m_grid[w].size()) continue;
                sf::Vector2f p = m_tilePos(static_cast<int>(w), static_cast<int>(c));
                bool isFilled = (m_grid[w][c] != '_');
                tileBackground.setPosition(p);
                if (isFilled) { tileBackground.setFillColor(m_currentTheme.gridFilledTile); tileBackground.setOutlineColor(m_currentTheme.gridFilledTile); }
                else { tileBackground.setFillColor(m_currentTheme.gridEmptyTile); tileBackground.setOutlineColor(m_currentTheme.gridEmptyTile); }
                m_window.draw(tileBackground);

                if (!isFilled) {
                    sf::Sprite* gemSprite = nullptr;
                    const sf::Texture* gemTexture = nullptr; // Pointer to the relevant texture

                    if (wordRarity == 2 && m_sapphireSpr) {
                        gemSprite = m_sapphireSpr.get();
                        gemTexture = &m_sapphireTex; // Get address of the texture object
                    }
                    else if (wordRarity == 3 && m_rubySpr) {
                        gemSprite = m_rubySpr.get();
                        gemTexture = &m_rubyTex;
                    }
                    else if (wordRarity == 4 && m_diamondSpr) {
                        gemSprite = m_diamondSpr.get();
                        gemTexture = &m_diamondTex;
                    }

                    if (gemSprite && gemTexture) { // Check both sprite and its intended texture
                        float desiredGemHeight = finalRenderTileSize * 0.60f; // Scale gem relative to new tile size

                        // Get texture dimensions using the dot operator on the texture object
                        // (or -> if gemTexture was a pointer to a texture, but here it's const sf::Texture*)
                        if (gemTexture->getSize().y > 0) { // Use -> because gemTexture is a pointer
                            float gemScale = desiredGemHeight / static_cast<float>(gemTexture->getSize().y);
                            gemSprite->setScale({ gemScale, gemScale }); // SFML 3
                            // Origin is usually set once at load based on original texture dimensions
                            // If you want to ensure it's always center of the *original* texture:
                            gemSprite->setOrigin({ static_cast<float>(gemTexture->getSize().x) / 2.f,
                                                  static_cast<float>(gemTexture->getSize().y) / 2.f });
                        }

                        gemSprite->setPosition({ p.x + finalRenderTileSize / 2.f, p.y + finalRenderTileSize / 2.f });
                        m_window.draw(*gemSprite);
                    }
                }
                else {
                    bool isAnimatingToTile = false;
                    for (const auto& anim : m_anims) { if (anim.target == AnimTarget::Grid && anim.wordIdx == w && anim.charIdx == c && anim.t < 1.0f) { isAnimatingToTile = true; break; } }
                    if (!isAnimatingToTile) {
                        float currentFlourishScale = 1.0f; bool isFlourishing = false;
                        for (const auto& flourish : m_gridFlourishes) { if (flourish.wordIdx == w && flourish.charIdx == c) { float progress = (GRID_FLOURISH_DURATION - flourish.timer) / GRID_FLOURISH_DURATION; currentFlourishScale = 1.0f + 0.4f * std::sin(progress * PI); isFlourishing = true; break; } }
                        letterText.setString(std::string(1, m_grid[w][c]));
                        letterText.setFillColor(m_currentTheme.gridLetter);
                        sf::FloatRect b = letterText.getLocalBounds();
                        letterText.setOrigin({ b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f });
                        letterText.setPosition(p + sf::Vector2f(finalRenderTileSize / 2.f, finalRenderTileSize / 2.f));
                        letterText.setScale({ currentFlourishScale, currentFlourishScale });
                        m_window.draw(letterText);
                        if (!isFlourishing) letterText.setScale({ 1.f, 1.f });
                    }
                }
            }
        }
    }

    //------------------------------------------------------------
 //  Draw Path lines (BEFORE Wheel Letters)
 //------------------------------------------------------------
    if (m_dragging && !m_path.empty()) {
        const float halfThickness = scaledPathThickness / 2.0f; // scaledPathThickness from top of function
        const sf::PrimitiveType stripType = sf::PrimitiveType::TriangleStrip;
        const sf::Color pathColor = m_currentTheme.dragLine;

        // Draw lines between selected letters in the path
        if (m_path.size() >= 2) {
            sf::VertexArray finalPathStrip(stripType); // Dynamically add vertices
            // finalPathStrip.resize((m_path.size() - 1) * 4); // Old pre-sizing, dynamic is safer

            for (size_t i = 0; i < m_path.size() - 1; ++i) {
                int path_idx1 = m_path[i]; // Index for m_base and m_wheelLetterRenderPos
                int path_idx2 = m_path[i + 1];

                // Use m_wheelLetterRenderPos for coordinates
                if (path_idx1 < 0 || static_cast<size_t>(path_idx1) >= m_wheelLetterRenderPos.size() ||
                    path_idx2 < 0 || static_cast<size_t>(path_idx2) >= m_wheelLetterRenderPos.size()) {
                    std::cerr << "Warning: Path index out of bounds for m_wheelLetterRenderPos (Path Segment)" << std::endl;
                    continue;
                }
                sf::Vector2f p1 = m_wheelLetterRenderPos[path_idx1];
                sf::Vector2f p2 = m_wheelLetterRenderPos[path_idx2];

                sf::Vector2f direction = p2 - p1;
                float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
                if (length < 0.1f) continue; // Avoid division by zero for tiny segments

                sf::Vector2f unitDirection = direction / length;
                sf::Vector2f unitPerpendicular = { -unitDirection.y, unitDirection.x }; // SFML 3: Direct init
                sf::Vector2f offset = unitPerpendicular * halfThickness;

                // Add vertices for this segment
                finalPathStrip.append(sf::Vertex(p1 - offset, pathColor));
                finalPathStrip.append(sf::Vertex(p1 + offset, pathColor));
                finalPathStrip.append(sf::Vertex(p2 - offset, pathColor));
                finalPathStrip.append(sf::Vertex(p2 + offset, pathColor));
            }
            if (finalPathStrip.getVertexCount() > 0) {
                m_window.draw(finalPathStrip);
            }
        }

        // Draw "rubber band" line from last selected letter to mouse cursor
        // m_path is guaranteed not empty here due to outer if condition
        int lastPathIdx = m_path.back();
        if (lastPathIdx >= 0 && static_cast<size_t>(lastPathIdx) < m_wheelLetterRenderPos.size()) {
            sf::Vector2f p1 = m_wheelLetterRenderPos[lastPathIdx]; // Use visual position
            sf::Vector2f p2 = mousePos; // mousePos is already mapped in m_renderGameScreen

            sf::Vector2f direction = p2 - p1;
            float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);

            if (length > 0.1f) { // Only draw if there's some length
                sf::Vector2f unitDirection = direction / length;
                sf::Vector2f unitPerpendicular = { -unitDirection.y, unitDirection.x }; // SFML 3
                sf::Vector2f offset = unitPerpendicular * halfThickness;

                sf::VertexArray rubberBandStrip(stripType, 4);
                rubberBandStrip[0].position = p1 - offset;
                rubberBandStrip[1].position = p1 + offset;
                rubberBandStrip[2].position = p2 - offset;
                rubberBandStrip[3].position = p2 + offset;
                for (unsigned int k = 0; k < 4; ++k) rubberBandStrip[k].color = pathColor;
                m_window.draw(rubberBandStrip);
            }
        }
    }

    //------------------------------------------------------------
    //  Draw wheel background & letters
    //------------------------------------------------------------
    /*m_wheelBg.setRadius(m_visualBgRadius); // m_visualBgRadius is set in m_updateLayout
    m_wheelBg.setOrigin({ m_visualBgRadius, m_visualBgRadius });
    m_wheelBg.setPosition(sf::Vector2f{ m_wheelX, m_wheelY });
    m_wheelBg.setFillColor(m_currentTheme.wheelBg);
    m_wheelBg.setOutlineColor(m_currentTheme.wheelOutline);
    m_wheelBg.setOutlineThickness(scaledWheelOutlineThickness);
    m_window.draw(m_wheelBg); */

    float fontScaleRatio = 1.f;
    if (LETTER_R_BASE_DESIGN > 0.1f && m_currentLetterRenderRadius > 0.1f) {
        fontScaleRatio = m_currentLetterRenderRadius / LETTER_R_BASE_DESIGN;
    }
    // Clamp to prevent excessively tiny or huge fonts relative to original design.
    fontScaleRatio = std::clamp(fontScaleRatio, 0.5f, 1.5f);

    // S(this, ...) scales the base font size by the overall m_uiScale.
    // fontScaleRatio further refines it based on the letter circle's current relative size.
    unsigned int actualScaledWheelLetterFontSize = static_cast<unsigned int>(
        std::max(8.0f, S(this, WHEEL_LETTER_FONT_SIZE_BASE_DESIGN) * fontScaleRatio)
        );

    sf::Text chTxt(m_font, "", actualScaledWheelLetterFontSize);

    for (std::size_t i = 0; i < m_base.size(); ++i) {
        if (i >= m_wheelLetterRenderPos.size()) continue;
        bool isHilited = std::find(m_path.begin(), m_path.end(), static_cast<int>(i)) != m_path.end();
        sf::Vector2f renderPos = m_wheelLetterRenderPos[i];
        sf::CircleShape letterCircle(m_currentLetterRenderRadius);
        letterCircle.setOrigin({ m_currentLetterRenderRadius, m_currentLetterRenderRadius });
        letterCircle.setPosition(renderPos);
        letterCircle.setFillColor(isHilited ? m_currentTheme.wheelOutline : m_currentTheme.letterCircleNormal);
        letterCircle.setOutlineColor(m_currentTheme.wheelOutline);
        letterCircle.setOutlineThickness(scaledLetterCircleOutline);
        m_window.draw(letterCircle);
        chTxt.setString(std::string(1, static_cast<char>(std::toupper(m_base[i]))));
        chTxt.setFillColor(isHilited ? m_currentTheme.letterTextHighlight : m_currentTheme.letterTextNormal);
        sf::FloatRect txtBounds_ch = chTxt.getLocalBounds();
        chTxt.setOrigin({ txtBounds_ch.position.x + txtBounds_ch.size.x / 2.f, txtBounds_ch.position.y + txtBounds_ch.size.y / 2.f });
        chTxt.setPosition(renderPos);
        m_window.draw(chTxt);
    }

    // --- DRAW FLYING LETTER ANIMATIONS ---
    sf::Text flyingLetterText(m_font, "", scaledFlyingLetterFontSize);
    sf::Color flyColorBase = m_currentTheme.gridLetter;
    for (const auto& a : m_anims) { sf::Color currentFlyColor = flyColorBase; if (a.target == AnimTarget::Score) { currentFlyColor = sf::Color::Yellow; } float alpha_ratio = 1.0f; if (a.t > 0.7f) { alpha_ratio = (1.0f - a.t) / 0.3f; alpha_ratio = std::max(0.0f, std::min(1.0f, alpha_ratio)); } currentFlyColor.a = static_cast<std::uint8_t>(255.f * alpha_ratio); flyingLetterText.setFillColor(currentFlyColor); float eased_t = a.t * a.t * (3.f - 2.f * a.t); sf::Vector2f p_anim = a.start + (a.end - a.start) * eased_t; flyingLetterText.setString(std::string(1, a.ch)); sf::FloatRect bounds_fly = flyingLetterText.getLocalBounds(); flyingLetterText.setOrigin({ bounds_fly.position.x + bounds_fly.size.x / 2.f, bounds_fly.position.y + bounds_fly.size.y / 2.f }); flyingLetterText.setPosition(p_anim); m_window.draw(flyingLetterText); }

    // --- DRAW SCORE FLOURISHES (from grid words) ---
    m_renderScoreFlourishes(m_window);

    // --- DRAW HINT POINT ANIMATIONS (+1 flying to Points text) ---
    m_renderHintPointAnims(m_window);

 
    // --- Draw Guess Display ---
    if (m_gameState == GState::Playing && !m_currentGuess.empty() && m_guessDisplay_Text && m_guessDisplay_Bg.getPointCount() > 0) {
        m_guessDisplay_Text->setCharacterSize(scaledGuessDisplayFontSize);
        m_guessDisplay_Text->setString(m_currentGuess);
        sf::FloatRect textBounds_guess = m_guessDisplay_Text->getLocalBounds();
        sf::Vector2f bgSize_guess = { textBounds_guess.position.x + textBounds_guess.size.x + 2 * scaledGuessDisplayPadX,
                                     textBounds_guess.position.y + textBounds_guess.size.y + 2 * scaledGuessDisplayPadY };
        float wheelVisualTopY = m_wheelY - m_visualBgRadius; // m_visualBgRadius is correct here
        float guessY_val = wheelVisualTopY - (bgSize_guess.y / 2.f) - scaledGuessDisplayGap;
        m_guessDisplay_Bg.setSize(bgSize_guess);
        m_guessDisplay_Bg.setRadius(scaledGuessDisplayRadius);
        m_guessDisplay_Bg.setFillColor(m_currentTheme.gridFilledTile);
        m_guessDisplay_Bg.setOutlineColor(sf::Color(150, 150, 150, 200));
        m_guessDisplay_Bg.setOutlineThickness(scaledGuessDisplayOutline);
        m_guessDisplay_Bg.setOrigin({ bgSize_guess.x / 2.f, bgSize_guess.y / 2.f });
        m_guessDisplay_Bg.setPosition({ m_wheelX, guessY_val });
        m_guessDisplay_Text->setOrigin({ textBounds_guess.position.x + textBounds_guess.size.x / 2.f,
                                         textBounds_guess.position.y + textBounds_guess.size.y / 2.f });
        m_guessDisplay_Text->setPosition({ m_wheelX, guessY_val });
        m_guessDisplay_Text->setFillColor(m_currentTheme.gridLetter);
        m_window.draw(m_guessDisplay_Bg);
        m_window.draw(*m_guessDisplay_Text);
    }

    //------------------------------------------------------------
    //  Draw UI Buttons / HUD
    //------------------------------------------------------------
    if (m_gameState == GState::Playing) {
        if (m_scrambleSpr) { bool scrambleHover = m_scrambleSpr->getGlobalBounds().contains(mousePos); m_scrambleSpr->setColor(scrambleHover ? sf::Color::White : sf::Color(255, 255, 255, 200)); m_window.draw(*m_scrambleSpr); }

        float wheelVisualBottomY = m_wheelY + m_visualBgRadius; // m_visualBgRadius is correct here
        float bottomHudStartY = wheelVisualBottomY + scaledHudOffsetY;
        float currentTopY = bottomHudStartY;

/*        sf::Text foundTxt(m_font, "", scaledFoundFontSize);
        foundTxt.setString("Found: " + std::to_string(m_found.size()) + "/" + std::to_string(m_solutions.size()));
        foundTxt.setFillColor(m_currentTheme.hudTextFound);
        sf::FloatRect foundBounds = foundTxt.getLocalBounds();
        foundTxt.setOrigin({ foundBounds.position.x + foundBounds.size.x / 2.f, foundBounds.position.y }); // Use .position.y for vertical origin if aligning top
        foundTxt.setPosition({ m_wheelX, currentTopY });
        m_window.draw(foundTxt);
        currentTopY += foundBounds.size.y + scaledHudLineSpacing;*/

        if (!m_allPotentialSolutions.empty() || !m_foundBonusWords.empty()) {
            int totalPossibleBonus = 0;
            // Iterate through all words that *could* be formed from the base letters
            for (const auto& potentialWordInfo : m_allPotentialSolutions) {
                // A word is a "possible bonus word" if it's NOT a main grid solution
                if (!isGridSolution(potentialWordInfo.text)) { // <<< USE THE HELPER HERE
                    totalPossibleBonus++;
                }
            }
            // m_foundBonusWords correctly counts how many of these *actual* bonus words have been found by the player.
            // This assumes that when a bonus word is added to m_foundBonusWords, it's already confirmed not to be a grid solution.

            std::string bonusCountStr = "Bonus Words: " + std::to_string(m_foundBonusWords.size()) + "/" + std::to_string(totalPossibleBonus);

            sf::Text bonusWordsDisplay(m_font, bonusCountStr, static_cast<unsigned int>(S(this, SCORE_ZONE_BONUS_FONT_SIZE)));
            bonusWordsDisplay.setFillColor(GLOWING_TUBE_TEXT_COLOR); // Or your theme color

            sf::FloatRect bonusTextBounds = bonusWordsDisplay.getLocalBounds();
            bonusWordsDisplay.setOrigin({ bonusTextBounds.position.x + bonusTextBounds.size.x / 2.f,
                                         bonusTextBounds.position.y + bonusTextBounds.size.y });
            bonusWordsDisplay.setPosition(sf::Vector2f{
                SCORE_ZONE_RECT_DESIGN.position.x + SCORE_ZONE_RECT_DESIGN.size.x / 2.f,
                SCORE_ZONE_RECT_DESIGN.position.y + SCORE_ZONE_RECT_DESIGN.size.y - S(this, SCORE_ZONE_PADDING_Y_DESIGN)
                });

            // Apply flourish (m_bonusTextFlourishTimer)
            // ... (flourish logic for bonusWordsDisplay remains the same) ...
            sf::Vector2f bonusDispOriginalPos = bonusWordsDisplay.getPosition();
            sf::Vector2f bonusDispOriginalOrigin = bonusWordsDisplay.getOrigin();
            sf::Vector2f bonusDispOriginalScale = bonusWordsDisplay.getScale();
            if (m_bonusTextFlourishTimer > 0.f) {
                float progress = (BONUS_TEXT_FLOURISH_DURATION - m_bonusTextFlourishTimer) / BONUS_TEXT_FLOURISH_DURATION;
                float bonusFlourishScaleFactor = 1.0f + 0.4f * std::sin(progress * PI);
                sf::FloatRect bonusLocalBoundsActual = bonusWordsDisplay.getLocalBounds();
                float bonusCenterX = bonusLocalBoundsActual.position.x + bonusLocalBoundsActual.size.x / 2.f;
                float bonusCenterY = bonusLocalBoundsActual.position.y + bonusLocalBoundsActual.size.y / 2.f;
                bonusWordsDisplay.setOrigin({ bonusCenterX, bonusCenterY });
                bonusWordsDisplay.setPosition(sf::Vector2f{ bonusDispOriginalPos.x - bonusDispOriginalOrigin.x + bonusCenterX,
                                                           bonusDispOriginalPos.y - bonusDispOriginalOrigin.y + bonusCenterY });
                bonusWordsDisplay.setScale({ bonusFlourishScaleFactor, bonusFlourishScaleFactor });
            }
            m_window.draw(bonusWordsDisplay);
            if (m_bonusTextFlourishTimer > 0.f) { // Restore
                bonusWordsDisplay.setScale(bonusDispOriginalScale);
                bonusWordsDisplay.setOrigin(bonusDispOriginalOrigin);
                bonusWordsDisplay.setPosition(bonusDispOriginalPos);
            }
        }
            // --- End Score Zone Elements Drawing ---

        // Hint "Points:" Text Display with Flourish
        if (m_hintPointsText) {
            m_hintPointsText->setString("Points: " + std::to_string(m_hintPoints));
            m_hintPointsText->setFillColor(m_currentTheme.hudTextFound);
            sf::Vector2f originalPosition = m_hintPointsText->getPosition();
            sf::Vector2f originalOrigin = m_hintPointsText->getOrigin();
            sf::Vector2f originalScale = m_hintPointsText->getScale();
            if (m_hintPointsTextFlourishTimer > 0.f) {
                float progress = (HINT_POINT_TEXT_FLOURISH_DURATION - m_hintPointsTextFlourishTimer) / HINT_POINT_TEXT_FLOURISH_DURATION;
                float flourishScaleFactor = 1.0f + 0.3f * std::sin(progress * PI);
                sf::FloatRect localBounds = m_hintPointsText->getLocalBounds();
                float centerX = localBounds.position.x + localBounds.size.x / 2.f;
                float centerY = localBounds.position.y + localBounds.size.y / 2.f;
                m_hintPointsText->setOrigin({ centerX, centerY });
                m_hintPointsText->setPosition({ originalPosition.x + (centerX - originalOrigin.x), originalPosition.y + (centerY - originalOrigin.y) });
                m_hintPointsText->setScale({ flourishScaleFactor, flourishScaleFactor });
            }
            m_window.draw(*m_hintPointsText);
            if (m_hintPointsTextFlourishTimer > 0.f) {
                m_hintPointsText->setScale(originalScale);
                m_hintPointsText->setOrigin(originalOrigin);
                m_hintPointsText->setPosition(originalPosition);
            }
        }

        // Update hover state for bonus words popup trigger
        if (m_hintPointsText && m_currentScreen == GameScreen::Playing) { // Only if text exists and on playing screen
            // Convert m_hintPointsText's local bounds to global bounds for accurate hover check
            // This assumes m_hintPointsText is already positioned correctly by m_updateLayout
            sf::FloatRect hintTextGlobalBounds = m_hintPointsText->getGlobalBounds();
            if (hintTextGlobalBounds.contains(mousePos)) {
                m_isHoveringHintPointsText = true;
            }
            else {
                m_isHoveringHintPointsText = false;
            }
        }
        else {
            m_isHoveringHintPointsText = false; // Not hovering if text doesn't exist or not on playing screen
        }

        // Draw Hint UI Background and Buttons
        m_hintAreaBg.setFillColor(sf::Color(200, 200, 200, 50));
        m_hintAreaBg.setOutlineColor(sf::Color(220, 220, 220, 100));
        m_hintAreaBg.setOutlineThickness(S(this, 1.f));
        m_window.draw(m_hintAreaBg);

        sf::Color affordableColor = m_currentTheme.menuButtonNormal;
        sf::Color affordableHoverColor = m_currentTheme.menuButtonHover;
        sf::Color unaffordableColor = sf::Color(100, 100, 100, 180);
        sf::Color affordableTextColor = m_currentTheme.menuButtonText;
        sf::Color unaffordableTextColor = sf::Color(180, 180, 180, 200);

        bool canAffordFirst = m_hintPoints >= HINT_COST_REVEAL_FIRST;
        bool hoverFirst = m_hintRevealFirstButtonShape.getGlobalBounds().contains(mousePos);
        m_hintRevealFirstButtonShape.setFillColor(canAffordFirst ? (hoverFirst ? affordableHoverColor : affordableColor) : unaffordableColor);
        m_window.draw(m_hintRevealFirstButtonShape);
        if (m_hintRevealFirstButtonText) { m_hintRevealFirstButtonText->setFillColor(canAffordFirst ? affordableTextColor : unaffordableTextColor); m_window.draw(*m_hintRevealFirstButtonText); }
        if (m_hintRevealFirstCostText) { m_hintRevealFirstCostText->setString("Cost: " + std::to_string(HINT_COST_REVEAL_FIRST)); m_hintRevealFirstCostText->setFillColor(canAffordFirst ? affordableTextColor : unaffordableTextColor); m_window.draw(*m_hintRevealFirstCostText); }

        bool canAffordRandom = m_hintPoints >= HINT_COST_REVEAL_RANDOM;
        bool hoverRandom = m_hintRevealRandomButtonShape.getGlobalBounds().contains(mousePos);
        m_hintRevealRandomButtonShape.setFillColor(canAffordRandom ? (hoverRandom ? affordableHoverColor : affordableColor) : unaffordableColor);
        m_window.draw(m_hintRevealRandomButtonShape);
        if (m_hintRevealRandomButtonText) { m_hintRevealRandomButtonText->setFillColor(canAffordRandom ? affordableTextColor : unaffordableTextColor); m_window.draw(*m_hintRevealRandomButtonText); }
        if (m_hintRevealRandomCostText) { m_hintRevealRandomCostText->setString("Cost: " + std::to_string(HINT_COST_REVEAL_RANDOM)); m_hintRevealRandomCostText->setFillColor(canAffordRandom ? affordableTextColor : unaffordableTextColor); m_window.draw(*m_hintRevealRandomCostText); }

        bool canAffordLast = m_hintPoints >= HINT_COST_REVEAL_LAST;
        bool hoverLast = m_hintRevealLastButtonShape.getGlobalBounds().contains(mousePos);
        m_hintRevealLastButtonShape.setFillColor(canAffordLast ? (hoverLast ? affordableHoverColor : affordableColor) : unaffordableColor);
        m_window.draw(m_hintRevealLastButtonShape);
        if (m_hintRevealLastButtonText) { m_hintRevealLastButtonText->setFillColor(canAffordLast ? affordableTextColor : unaffordableTextColor); m_window.draw(*m_hintRevealLastButtonText); }
        if (m_hintRevealLastCostText) { m_hintRevealLastCostText->setString("Cost: " + std::to_string(HINT_COST_REVEAL_LAST)); m_hintRevealLastCostText->setFillColor(canAffordLast ? affordableTextColor : unaffordableTextColor); m_window.draw(*m_hintRevealLastCostText); }

        bool canAffordFirstOfEach = m_hintPoints >= HINT_COST_REVEAL_FIRST_OF_EACH;
        bool hoverFirstOfEach = m_hintRevealFirstOfEachButtonShape.getGlobalBounds().contains(mousePos);
        m_hintRevealFirstOfEachButtonShape.setFillColor(canAffordFirstOfEach ? (hoverFirstOfEach ? affordableHoverColor : affordableColor) : unaffordableColor);
        m_window.draw(m_hintRevealFirstOfEachButtonShape);
        if (m_hintRevealFirstOfEachButtonText) { m_hintRevealFirstOfEachButtonText->setFillColor(canAffordFirstOfEach ? affordableTextColor : unaffordableTextColor); m_window.draw(*m_hintRevealFirstOfEachButtonText); }
        if (m_hintRevealFirstOfEachCostText) { m_hintRevealFirstOfEachCostText->setString("Cost: " + std::to_string(HINT_COST_REVEAL_FIRST_OF_EACH)); m_hintRevealFirstOfEachCostText->setFillColor(canAffordFirstOfEach ? affordableTextColor : unaffordableTextColor); m_window.draw(*m_hintRevealFirstOfEachCostText); }
    }

    //------------------------------------------------------------
    //  Draw Solved State overlay
    //------------------------------------------------------------
    if (m_currentScreen == GameScreen::GameOver) {
        sf::Text winTxt(m_font, "Puzzle Solved!", scaledSolvedFontSize);
        winTxt.setFillColor(m_currentTheme.hudTextSolved);
        winTxt.setStyle(sf::Text::Bold);
        sf::FloatRect winTxtBounds = winTxt.getLocalBounds();
        sf::Vector2f contBtnSize = m_contBtn.getSize();
        const float scaledPadding = S(this, 25.f);
        const float scaledSpacing = S(this, 20.f);
        float overlayWidth = std::max(winTxtBounds.size.x, contBtnSize.x) + 2.f * scaledPadding;
        float overlayHeight = winTxtBounds.size.y + contBtnSize.y + scaledSpacing + 2.f * scaledPadding;
        m_solvedOverlay.setSize({ overlayWidth, overlayHeight });
        m_solvedOverlay.setRadius(S(this, 15.f));
        m_solvedOverlay.setFillColor(m_currentTheme.solvedOverlayBg);
        m_solvedOverlay.setOrigin({ overlayWidth / 2.f, overlayHeight / 2.f });
        sf::Vector2f windowCenterPix = sf::Vector2f(m_window.getSize()) / 2.f;
        sf::Vector2f overlayCenter = m_window.mapPixelToCoords(sf::Vector2i(windowCenterPix));
        m_solvedOverlay.setPosition(overlayCenter);
        float winTxtCenterY = overlayCenter.y - overlayHeight / 2.f + scaledPadding + (winTxtBounds.position.y + winTxtBounds.size.y / 2.f);
        float contBtnPosY = winTxtCenterY + (winTxtBounds.size.y / 2.f) + scaledSpacing;
        winTxt.setOrigin({ winTxtBounds.position.x + winTxtBounds.size.x / 2.f, winTxtBounds.position.y + winTxtBounds.size.y / 2.f });
        winTxt.setPosition({ overlayCenter.x, winTxtCenterY });
        if (m_contTxt) {
            m_contTxt->setCharacterSize(scaledContinueFontSize);
            sf::FloatRect contTxtBounds = m_contTxt->getLocalBounds();
            m_contTxt->setOrigin({ contTxtBounds.position.x + contTxtBounds.size.x / 2.f, contTxtBounds.position.y + contTxtBounds.size.y / 2.f });
            m_contBtn.setOrigin({ contBtnSize.x / 2.f, 0.f });
            m_contBtn.setPosition({ overlayCenter.x, contBtnPosY });
            m_contTxt->setPosition(m_contBtn.getPosition() + sf::Vector2f{ 0.f, contBtnSize.y / 2.f });
        }
        bool contHover = m_contBtn.getGlobalBounds().contains(mousePos); sf::Color continueHoverColor = adjustColorBrightness(m_currentTheme.continueButton, 1.2f);
        m_contBtn.setFillColor(contHover ? continueHoverColor : m_currentTheme.continueButton);
        m_window.draw(m_solvedOverlay);
        m_window.draw(winTxt);
        m_window.draw(m_contBtn);
        if (m_contTxt) m_window.draw(*m_contTxt);
    }
}
// ***** END OF COMPLETE Game::m_renderGameScreen FUNCTION *****


// --- Celebration Effects ---
void Game::m_startCelebrationEffects() {
    m_confetti.clear();
    m_balloons.clear();
    m_celebrationEffectTimer = 0.f;

    // *** USE DESIGN SPACE COORDINATES ***
    const float designW = static_cast<float>(REF_W);
    const float designH = static_cast<float>(REF_H);

    std::cout << "DEBUG: Starting celebration effects in design space: "
        << designW << "x" << designH << std::endl;

    // --- Spawn Initial Confetti Burst ---
    int confettiCount = 200;
    for (int i = 0; i < confettiCount; ++i) {
        ConfettiParticle p;
        // Start near bottom corners *of the design space*
        float startXConfetti = (randRange(0, 1) == 0)
            ? randRange(-20.f, 60.f) // Near left edge
            : randRange(designW - 60.f, designW + 20.f); // Near right edge
        float startYConfetti = randRange(designH - 40.f, designH + 20.f); // Near bottom edge

        p.shape.setPosition({ startXConfetti, startYConfetti });
        p.shape.setSize({ randRange(4.f, 8.f), randRange(6.f, 12.f) }); // Size is absolute (design units)
        p.shape.setFillColor(sf::Color(randRange(100, 255), randRange(100, 255), randRange(100, 255)));
        p.shape.setOrigin(p.shape.getSize() / 2.f);

        // Angle calculation based on design space center
        float angle = 0;
        if (startXConfetti < designW / 2.f) { // Left side launch upwards-right
            angle = randRange(270.f + 10.f, 270.f + 80.f); // -80 to -10 degrees from vertical
        }
        else { // Right side launch upwards-left
            angle = randRange(180.f + 10.f, 180.f + 80.f); // -170 to -100 degrees from vertical
        }
        float speed = randRange(150.f, 450.f); // Speed in design units per second
        float angleRad = angle * PI / 180.f;
        p.velocity = { std::cos(angleRad) * speed, std::sin(angleRad) * speed };
        p.angularVelocity = randRange(-360.f, 360.f); // Degrees per second
        p.lifetime = randRange(2.0f, 5.0f); // Seconds
        p.initialLifetime = p.lifetime;
        m_confetti.push_back(std::move(p));
    }

    // --- Spawn Initial Balloons ---
    int balloonCount = 7;
    const float balloonRadius = 30.f; // Radius in design units

    for (int i = 0; i < balloonCount; ++i) {
        Balloon b;
        // Start below screen *in design space*
        float startX = randRange(balloonRadius * 2.f, designW - balloonRadius * 2.f);
        b.position = { startX, designH + balloonRadius + randRange(10.f, 100.f) };
        b.initialX = startX;

        b.shape.setRadius(balloonRadius);
        b.shape.setFillColor(sf::Color(randRange(100, 255), randRange(100, 255), randRange(100, 255), 230));
        b.shape.setOutlineColor(sf::Color::White);
        b.shape.setOutlineThickness(1.f);
        b.shape.setOrigin({ balloonRadius, balloonRadius });

        b.stringShape.setSize({ 2.f, randRange(40.f, 70.f) }); // Size in design units
        b.stringShape.setFillColor(sf::Color(200, 200, 200));
        b.stringShape.setOrigin({ 1.f, 0 }); // Origin top-center

        // Movement parameters in design units / seconds
        b.riseSpeed = randRange(-100.f, -50.f);
        b.swaySpeed = randRange(0.8f, 1.8f);
        b.swayAmount = randRange(30.f, 60.f); // Sway amount in design units
        b.swayTimer = randRange(0.f, 2.f * PI);
        b.timeToDisappear = randRange(6.0f, 15.0f); // Seconds

        m_balloons.push_back(std::move(b));
    }
}

void Game::m_updateCelebrationEffects(float dt) {
    // *** USE DESIGN SPACE COORDINATES FOR BOUNDS ***
    const float designW = static_cast<float>(REF_W);
    const float designH = static_cast<float>(REF_H);
    const float GRAVITY = 98.0f; // Gravity in design units per second squared

    // --- Update Confetti ---
    for (size_t i = 0; i < m_confetti.size(); /* no increment here */) {
        ConfettiParticle& p = m_confetti[i];
        p.velocity.y += GRAVITY * dt;
        p.shape.move(p.velocity * dt); // Move based on velocity (design units/sec * sec)
        float rotationDegrees = p.angularVelocity * dt;
        p.shape.rotate(sf::degrees(rotationDegrees));
        p.lifetime -= dt;

        float alphaRatio = std::max(0.f, p.lifetime / p.initialLifetime);
        sf::Color color = p.shape.getFillColor();
        color.a = static_cast<std::uint8_t>(255.f * alphaRatio); // Cast needed here
        p.shape.setFillColor(color);

        // Check for removal based on design space boundaries
        if (p.lifetime <= 0.f || p.shape.getPosition().y > designH + 50.f) { // Check against designH
            std::swap(m_confetti[i], m_confetti.back());
            m_confetti.pop_back();
        }
        else {
            ++i;
        }
    }

    // --- Update Balloons ---
    for (size_t i = 0; i < m_balloons.size(); /* no increment */) {
        Balloon& b = m_balloons[i];
        b.position.y += b.riseSpeed * dt; // Update internal position
        b.swayTimer += dt;
        float currentSwayOffset = std::sin(b.swayTimer * b.swaySpeed) * b.swayAmount;
        // Set shape position based on internal state and sway
        b.shape.setPosition({ b.initialX + currentSwayOffset, b.position.y });
        b.stringShape.setPosition(b.shape.getPosition() + sf::Vector2f(0, b.shape.getRadius()));

        b.timeToDisappear -= dt;
        float topEdge = b.position.y - b.shape.getRadius();
        // Check for removal based on design space top edge
        if (b.timeToDisappear <= 0.f || topEdge < -100.f) { // Check against top of design space
            std::swap(m_balloons[i], m_balloons.back());
            m_balloons.pop_back();
        }
        else {
            ++i;
        }
    }

    // --- Optional: Spawn more effects over time ---
    m_celebrationEffectTimer += dt;
    if (m_celebrationEffectTimer > 0.15f) {
        m_celebrationEffectTimer = 0.f;

        // *** Implement spawning using DESIGN COORDINATES ***

        // Example: Spawn 5 more confetti
        for (int j = 0; j < 5; ++j) {
            ConfettiParticle p;
            float startXConfetti = (randRange(0, 1) == 0) ? randRange(-20.f, 60.f) : randRange(designW - 60.f, designW + 20.f);
            float startYConfetti = randRange(designH - 40.f, designH + 20.f);
            p.shape.setPosition({ startXConfetti, startYConfetti });
            p.shape.setSize({ randRange(4.f, 8.f), randRange(6.f, 12.f) });
            p.shape.setFillColor(sf::Color(randRange(100, 255), randRange(100, 255), randRange(100, 255)));
            p.shape.setOrigin(p.shape.getSize() / 2.f);
            float angle = (startXConfetti < designW / 2.f) ? randRange(280.f, 350.f) : randRange(190.f, 260.f);
            float speed = randRange(150.f, 450.f); float angleRad = angle * PI / 180.f;
            p.velocity = { std::cos(angleRad) * speed, std::sin(angleRad) * speed };
            p.angularVelocity = randRange(-360.f, 360.f); p.lifetime = randRange(2.0f, 5.0f); p.initialLifetime = p.lifetime;
            m_confetti.push_back(std::move(p));
        }

        // Example: Spawn 1 more balloon maybe
        if (randRange(0, 10) < 2) {
            Balloon b; const float balloonRadius = 30.f; // Use same radius
            float startX = randRange(balloonRadius * 2.f, designW - balloonRadius * 2.f);
            b.position = { startX, designH + balloonRadius + randRange(10.f, 100.f) }; b.initialX = startX;
            b.shape.setRadius(balloonRadius); b.shape.setFillColor(sf::Color(randRange(100, 255), randRange(100, 255), randRange(100, 255), 230)); b.shape.setOutlineColor(sf::Color::White); b.shape.setOutlineThickness(1.f); b.shape.setOrigin({ balloonRadius, balloonRadius });
            b.stringShape.setSize({ 2.f, randRange(40.f, 70.f) }); b.stringShape.setFillColor(sf::Color(200, 200, 200)); b.stringShape.setOrigin({ 1.f, 0 });
            b.riseSpeed = randRange(-100.f, -50.f); b.swaySpeed = randRange(0.8f, 1.8f); b.swayAmount = randRange(30.f, 60.f); b.swayTimer = randRange(0.f, 2.f * PI); b.timeToDisappear = randRange(6.0f, 15.0f);
            m_balloons.push_back(std::move(b));
        }
        // *** End spawning implementation ***
    }
}


// m_renderCelebrationEffects remains unchanged

void Game::m_renderCelebrationEffects(sf::RenderTarget& target) {
    // Draw Confetti
    for (const auto& p : m_confetti) {
        target.draw(p.shape);
    }

    // Draw Balloons (Strings first, then shapes so shapes overlap strings slightly)
    for (const auto& b : m_balloons) {
        target.draw(b.stringShape);
    }
    for (const auto& b : m_balloons) {
        target.draw(b.shape);
    }
}

void Game::m_renderSessionComplete(const sf::Vector2f& mousePos) {
    // Option 1: Draw previous screen dimly as background
    // You might need to capture the last frame to a texture, which is complex.
    // Easier option: Just clear normally or draw a specific background.
    // m_window.clear(m_currentTheme.winBg); // Already done in m_render
    // m_decor.draw(m_window);              // Already done in m_render

    // Option 2: Draw game elements dimmed (requires modifying their colors)
    // DrawGridDimmed(); DrawWheelDimmed(); etc.

    // Option 3: Draw a simple overlay?
    sf::RectangleShape overlay(sf::Vector2f(m_window.getSize()));
    overlay.setFillColor(sf::Color(0, 0, 0, 150)); // Dark semi-transparent overlay
    m_window.draw(overlay);


    // --- Draw Final Score Prominently ---
    if (m_scoreValueText) {
        std::string finalScoreStr = "Final Score: " + std::to_string(m_currentScore);
        m_scoreValueText->setString(finalScoreStr);

        // --- Style for this screen ---
        // Define base size for final score display in design units
        const unsigned int finalScoreBaseFontSize = 48;
        // Calculate scaled font size using the S() helper
        unsigned int finalScoreScaledFontSize = static_cast<unsigned int>(std::max(12.0f, S(this, static_cast<float>(finalScoreBaseFontSize))));
        m_scoreValueText->setCharacterSize(finalScoreScaledFontSize); // Set scaled size
        m_scoreValueText->setFillColor(sf::Color::Yellow); // Highlight color
        m_scoreValueText->setStyle(sf::Text::Bold); // Make it stand out

        // --- Positioning using DESIGN SPACE ---
        // Get design space dimensions
        const float designW = static_cast<float>(REF_W);
        const float designH = static_cast<float>(REF_H);

        // Center origin based on the text's current bounds (after setting size)
        sf::FloatRect bounds = m_scoreValueText->getLocalBounds();
        m_scoreValueText->setOrigin({ bounds.position.x + bounds.size.x / 2.f,
                                     bounds.position.y + bounds.size.y / 2.f });

        // Position relative to design space (e.g., horizontally centered, 30% down)
        m_scoreValueText->setPosition({ designW / 2.f, designH * 0.3f });

        m_scoreValueText->setScale({ 1.f, 1.f }); // Ensure no previous flourish scale applied

        // Draw the text
        m_window.draw(*m_scoreValueText);

        // --- Reset style/size/color for normal use ---
        // (If m_scoreValueText is reused for the regular score bar display)
        m_scoreValueText->setStyle(sf::Text::Regular); // Reset style
        // Reset to default base size, scaled
        const unsigned int defaultScoreBaseFontSize = 24;
        unsigned int defaultScoreScaledFontSize = static_cast<unsigned int>(std::max(10.0f, S(this, static_cast<float>(defaultScoreBaseFontSize))));
        m_scoreValueText->setCharacterSize(defaultScoreScaledFontSize);
        m_scoreValueText->setFillColor(m_currentTheme.scoreTextValue); // Use theme color for regular display
        // Origin might need recalculation if layout expects something different,
        // but m_updateLayout likely resets it anyway.
    }


    // --- Draw Celebration Effects --- (On top of score/background)
    m_renderCelebrationEffects(m_window);


    // --- Draw Continue Button ---
        // Explicitly position the button for *this specific screen* using design coords.
    const float designW = static_cast<float>(REF_W);
    const float designH = static_cast<float>(REF_H);

    // Get the button's scaled size (set previously by layout, assuming it's consistent)
    sf::Vector2f contBtnSize = m_contBtn.getSize();

    // Set origin to Center-Center for easier positioning
    m_contBtn.setOrigin({ contBtnSize.x / 2.f, contBtnSize.y / 2.f });

    // Calculate desired position in design space (e.g., centered horizontally, 80% down)
    sf::Vector2f buttonPos = { designW / 2.f, designH * 0.8f };
    m_contBtn.setPosition(buttonPos); // <<< SET POSITION using design coords

    if (m_contTxt) { // Position text centered ON the button
        // Ensure text properties are set (size should be fine from layout)
        const unsigned int scaledContinueFontSize = static_cast<unsigned int>(std::max(8.0f, S(this, 24.f)));
        m_contTxt->setCharacterSize(scaledContinueFontSize);

        sf::FloatRect contTxtBounds = m_contTxt->getLocalBounds();
        // Set text origin to its center
        m_contTxt->setOrigin({ contTxtBounds.position.x + contTxtBounds.size.x / 2.f,
                               contTxtBounds.position.y + contTxtBounds.size.y / 2.f });
        // Set text position to the button's center position
        m_contTxt->setPosition(m_contBtn.getPosition()); // <<< Position relative to button's new center
    }

    // Handle hover using the button's current global bounds
    bool contHover = m_contBtn.getGlobalBounds().contains(mousePos);
    sf::Color continueHoverColor = adjustColorBrightness(m_currentTheme.continueButton, 1.3f); // Assuming func exists
    m_contBtn.setFillColor(contHover ? continueHoverColor : m_currentTheme.continueButton);

    // Draw the button and text
    m_window.draw(m_contBtn);
    if (m_contTxt) m_window.draw(*m_contTxt);

}

void Game::m_handleSessionCompleteEvents(const sf::Event& event) {
    // Optional: Handle Close, Resize if not done globally

    if (const auto* mb = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mb->button == sf::Mouse::Button::Left) {
            sf::Vector2f mp = m_window.mapPixelToCoords(mb->position);

            // Check Return to Menu Button
            if (m_returnToMenuButtonShape.getGlobalBounds().contains(mp)) {
                if (m_clickSound) m_clickSound->play();
                m_confetti.clear(); // Stop effects
                m_balloons.clear();
                m_currentScreen = GameScreen::MainMenu;
                // Reset score if desired
                // m_currentScore = 0;
                return; // Processed button click
            }

            // Use the button's current position (set in m_renderSessionComplete)
            if (m_contBtn.getGlobalBounds().contains(mp)) {
                if (m_clickSound) m_clickSound->play();

                // Stop celebration effects & transition
                m_confetti.clear();
                m_balloons.clear();
                m_currentScreen = GameScreen::MainMenu; // Or CasualMenu
                // Reset score if desired
                // m_currentScore = 0;
                // Potentially stop victory music and restart background music
            }
        }
    }
    // Optional: Handle key press (Enter/Space) to continue?
}

void Game::m_renderDebugCircle() {
    // Define circle properties in DESIGN SPACE coordinates
    const float designW = static_cast<float>(REF_W);
    const float designH = static_cast<float>(REF_H);
    sf::Vector2f designCenter = { designW / 2.f, designH / 2.f };
    float designRadius = std::min(designW, designH) * 0.2f; // e.g., 20% of the smaller design dimension

    // Create the circle shape
    sf::CircleShape circle(designRadius);
    circle.setOrigin({ designRadius, designRadius }); // Set origin to the center of the circle
    circle.setPosition(designCenter);           // Position using design space center
    circle.setFillColor(sf::Color::Yellow);
    circle.setOutlineColor(sf::Color::Red);
    circle.setOutlineThickness(5.f); // Use a fixed pixel thickness for the outline (or scale it too if desired)

    // Draw the circle - SFML handles the scaling based on the current view
    m_window.draw(circle);

    // --- Optional: Draw reference lines/borders ---
    // Draw lines marking the center of the design space
    sf::RectangleShape hLine({ designW, 1.f }); // Thin horizontal line across design width
    hLine.setOrigin({ designW / 2.f, 0.5f });
    hLine.setPosition(designCenter);
    hLine.setFillColor(sf::Color::Magenta);
    m_window.draw(hLine);

    sf::RectangleShape vLine({ 1.f, designH }); // Thin vertical line across design height
    vLine.setOrigin({ 0.5f, designH / 2.f });
    vLine.setPosition(designCenter);
    vLine.setFillColor(sf::Color::Magenta);
    m_window.draw(vLine);

    // Draw a border around the design space boundaries
    sf::RectangleShape designBorder;
    designBorder.setPosition({ 0.f, 0.f });
    designBorder.setSize({ designW, designH });
    designBorder.setFillColor(sf::Color::Transparent);
    designBorder.setOutlineColor(sf::Color::Cyan);
    designBorder.setOutlineThickness(1.f);
    m_window.draw(designBorder);
}

void Game::m_activateHint(HintType type) {
    std::cout << "DEBUG: Attempting to activate hint type: " << static_cast<int>(type) << std::endl;

    // --- Common checks: Don't activate if already solved or no blanks left ---
    if (m_gameState == GState::Solved) {
        std::cout << "DEBUG: Hint not activated - Puzzle already solved." << std::endl;
        if (m_errorWordSound) m_errorWordSound->play();
        return;
    }

    bool anyBlanksLeft = false;
    for (std::size_t w = 0; w < m_grid.size() && w < m_sorted.size(); ++w) {
        if (!m_found.count(m_sorted[w].text)) { // Only check unsolved words
            for (char gridChar : m_grid[w]) {
                if (gridChar == '_') {
                    anyBlanksLeft = true;
                    break;
                }
            }
        }
        if (anyBlanksLeft) break;
    }

    if (!anyBlanksLeft) {
        std::cout << "DEBUG: Hint not activated - No blank spaces left in unsolved words." << std::endl;
        if (m_errorWordSound) m_errorWordSound->play();
        return;
    }
    // --- End Common Checks ---


    // --- Hint-Specific Logic ---
    std::vector<std::tuple<int, int, char>> lettersToReveal; // Stores {wordIdx, charIdx, char}

    switch (type) {
        // ======================================
    case HintType::RevealFirst: {
        // ======================================
        std::cout << "DEBUG: Processing HintType::RevealFirst..." << std::endl;
        int targetWordIdx = -1;
        int targetCharIdx = -1;
        char targetChar = '_';
        bool foundSpot = false;

        // Find first unsolved word
        for (std::size_t w = 0; w < m_grid.size() && w < m_sorted.size(); ++w) {
            const std::string& solutionWord = m_sorted[w].text;
            if (!m_found.count(solutionWord)) { // Is it unsolved?
                // Find first blank in this word
                for (std::size_t c = 0; c < m_grid[w].size(); ++c) {
                    if (m_grid[w][c] == '_') {
                        targetWordIdx = static_cast<int>(w);
                        targetCharIdx = static_cast<int>(c);
                        if (targetCharIdx < solutionWord.length()) {
                            targetChar = solutionWord[targetCharIdx];
                        }
                        else {
                            std::cerr << "ERROR: Hint RevealFirst - targetCharIdx out of bounds for solutionWord!" << std::endl;
                            targetChar = '?'; // Fallback character
                        }
                        foundSpot = true;
                        break; // Found first blank in this word
                    }
                }
            }
            if (foundSpot) break; // Found first blank in the first unsolved word
        }

        if (foundSpot && targetChar != '_' && targetChar != '?') {
            lettersToReveal.emplace_back(targetWordIdx, targetCharIdx, targetChar);
        }
        else {
            std::cout << "DEBUG: RevealFirst could not find a suitable blank spot or invalid char." << std::endl;
            if (m_errorWordSound) m_errorWordSound->play();
            return;
        }
        break;
    }

    // ======================================
    case HintType::RevealRandom: {
    // ======================================
        std::cout << "DEBUG: Processing HintType::RevealRandom..." << std::endl;

        // Iterate through each word in the grid
        for (std::size_t w = 0; w < m_grid.size() && w < m_sorted.size(); ++w) {
            // Check if this word is unsolved
            if (!m_found.count(m_sorted[w].text)) {
                const std::string& solutionWord = m_sorted[w].text;
                std::vector<int> blankIndicesInThisWord; // Store indices of blanks for *this* word

                // Find all blank spots in *this specific* unsolved word
                for (std::size_t c = 0; c < m_grid[w].size(); ++c) {
                    if (m_grid[w][c] == '_') {
                        // Ensure the character index is valid for the solution word
                        if (c < solutionWord.length()) {
                            blankIndicesInThisWord.push_back(static_cast<int>(c));
                        }
                        else {
                            std::cerr << "WARNING: Hint RevealRandom - Blank spot index " << c
                                << " is out of bounds for solutionWord '" << solutionWord
                                << "' (length " << solutionWord.length() << ") at word index " << w << std::endl;
                        }
                    }
                }

                // If this unsolved word has any blank spots, pick one randomly to reveal
                if (!blankIndicesInThisWord.empty()) {
                    std::size_t randomBlankIdxInWord = randRange<std::size_t>(0, blankIndicesInThisWord.size() - 1);
                    int targetCharIdx = blankIndicesInThisWord[randomBlankIdxInWord];

                    // Double-check targetCharIdx validity (should be fine if populated correctly)
                    if (targetCharIdx >= 0 && targetCharIdx < solutionWord.length()) {
                        char targetChar = solutionWord[targetCharIdx];
                        lettersToReveal.emplace_back(static_cast<int>(w), targetCharIdx, targetChar);
                        std::cout << "  RevealRandom: Adding '" << targetChar << "' for word '" << solutionWord << "' at [" << w << "][" << targetCharIdx << "]" << std::endl;
                    }
                    else {
                        std::cerr << "ERROR: Hint RevealRandom - picked targetCharIdx " << targetCharIdx
                            << " is out of bounds for solutionWord '" << solutionWord
                            << "' (length " << solutionWord.length() << ") at word index " << w << std::endl;
                    }
                }
                else {
                    // This means the unsolved word was already fully revealed by another hint or prior reveal, which is fine.
                    std::cout << "  RevealRandom: Word '" << solutionWord << "' is unsolved but has no blanks to reveal." << std::endl;
                }
            } // End if word is unsolved
        } // End for each word in grid

        if (lettersToReveal.empty()) { // If NO letters were added (e.g., all unsolved words were already full)
            std::cout << "DEBUG: RevealRandom could not find any blank spots in any unsolved words." << std::endl;
            if (m_errorWordSound) m_errorWordSound->play();
            return; // Exit if no spots found across all unsolved words
        }
        break;
    }
    
    // =====================================
    case HintType::RevealLast: {
        std::cout << "DEBUG: Processing HintType::RevealLast..." << std::endl;
        int lastUnsolvedWordIdx = -1;

        for (int w = static_cast<int>(m_grid.size()) - 1; w >= 0; --w) {
            if (w < m_sorted.size() && !m_found.count(m_sorted[w].text)) {
                lastUnsolvedWordIdx = w;
                break;
            }
        }

        if (lastUnsolvedWordIdx != -1) {
            const std::string& solutionWord = m_sorted[lastUnsolvedWordIdx].text;
            bool anyRevealedInThisWord = false;
            for (std::size_t c = 0; c < m_grid[lastUnsolvedWordIdx].size(); ++c) {
                if (m_grid[lastUnsolvedWordIdx][c] == '_') {
                    if (c < solutionWord.length()) {
                        char targetChar = solutionWord[c];
                        lettersToReveal.emplace_back(lastUnsolvedWordIdx, static_cast<int>(c), targetChar);
                        anyRevealedInThisWord = true;
                    }
                    else {
                        std::cerr << "ERROR: Hint RevealLast - char index out of bounds for solutionWord!" << std::endl;
                    }
                }
            }
            if (!anyRevealedInThisWord) { // If the last unsolved word was somehow already filled
                std::cout << "DEBUG: RevealLast found last unsolved word, but it has no blank spaces (already revealed?)." << std::endl;
                if (m_errorWordSound) m_errorWordSound->play();
                return;
            }
        }
        else {
            std::cout << "DEBUG: RevealLast could not find an unsolved word." << std::endl;
            if (m_errorWordSound) m_errorWordSound->play();
            return;
        }
        break;
    }

    case HintType::RevealFirstOfEach: {
        std::cout << "DEBUG: Processing HintType::RevealFirstOfEach..." << std::endl;
        bool actionTakenForThisHint = false;

        for (std::size_t w = 0; w < m_grid.size() && w < m_sorted.size(); ++w) {
            if (!m_found.count(m_sorted[w].text)) { // If word is unsolved
                const std::string& solutionWord = m_sorted[w].text;
                int firstBlankCharIdxInThisWord = -1;

                // Find the *first* blank spot in this specific unsolved word
                for (std::size_t c = 0; c < m_grid[w].size(); ++c) {
                    if (m_grid[w][c] == '_') { // Found the first blank
                        if (c < solutionWord.length()) {
                            firstBlankCharIdxInThisWord = static_cast<int>(c);
                            break; // Done with this word's blanks, move to reveal this one
                        }
                        else {
                            std::cerr << "WARNING: Hint RevealFirstOfEach - Blank spot index " << c
                                << " is out of bounds for solutionWord '" << solutionWord
                                << "' (length " << solutionWord.length() << ") at word index " << w << std::endl;
                        }
                    }
                }

                if (firstBlankCharIdxInThisWord != -1) { // If a blank was found for this word
                    // Ensure index is still valid
                    if (static_cast<size_t>(firstBlankCharIdxInThisWord) < solutionWord.length()) {
                        char charToReveal = solutionWord[firstBlankCharIdxInThisWord];
                        lettersToReveal.emplace_back(static_cast<int>(w), firstBlankCharIdxInThisWord, charToReveal);
                        actionTakenForThisHint = true; // Mark that we are doing something
                        std::cout << "  RevealFirstOfEach: Adding '" << charToReveal << "' for word '"
                            << solutionWord << "' at [" << w << "][" << firstBlankCharIdxInThisWord << "]" << std::endl;
                    }
                    else {
                        std::cerr << "ERROR: Hint RevealFirstOfEach - firstBlankCharIdxInThisWord " << firstBlankCharIdxInThisWord
                            << " became invalid for solutionWord '" << solutionWord << "'" << std::endl;
                    }
                }
                else {
                    // This unsolved word has no blanks (already fully revealed by other means)
                    // std::cout << "  RevealFirstOfEach: Word '" << solutionWord << "' at index " << w
                    //           << " is unsolved but has no blanks." << std::endl;
                }
            }
        }

        if (!actionTakenForThisHint) { // If loop completed and NO letters were added AT ALL
            std::cout << "DEBUG: RevealFirstOfEach found no suitable blank spots in any unsolved words." << std::endl;
            // Important: If no action, refund points or don't play success sound
            // The points were already deducted in m_handlePlayingEvents.
            // We need to decide if we should refund or just play error.
            // For simplicity now, if it gets here, it means no letters were revealed.
            if (m_errorWordSound) m_errorWordSound->play();
            // We might need to add points back if no letters were revealed.
            // m_hintPoints += HINT_COST_REVEAL_FIRST_OF_EACH; // Example refund (if desired)
            return; // Exit without creating animations if no action.
        }
        break;
    }

    default:
        std::cerr << "WARNING: Unknown HintType passed to m_activateHint: " << static_cast<int>(type) << std::endl;
        return;
    }

    // --- Create Animations for Revealed Letters ---
    if (!lettersToReveal.empty()) {
        std::cout << "DEBUG: Creating " << lettersToReveal.size() << " hint animations." << std::endl;
        if (m_hintUsedSound) m_hintUsedSound->play(); // Play sound once for the batch

        float animationDelay = 0.0f; // Initial delay for the first letter
        for (const auto& revealInfo : lettersToReveal) {
            int wordIdx = std::get<0>(revealInfo);
            int charIdx = std::get<1>(revealInfo);
            char letter = std::get<2>(revealInfo);

            sf::Vector2f hintEndPos = m_tilePos(wordIdx, charIdx);
            float renderTileScaleFactor = (m_currentGridLayoutScale < 1.0f) ? GRID_TILE_RELATIVE_SCALE_WHEN_SHRUNK : 1.0f;
            const float finalRenderTileSize = S(this, TILE_SIZE) * renderTileScaleFactor;
            hintEndPos += sf::Vector2f{ finalRenderTileSize / 2.f, finalRenderTileSize / 2.f };

            sf::Vector2f hintStartPos = { m_wheelX, m_wheelY };
            for (size_t i = 0; i < m_base.length(); ++i) {
                if (std::toupper(m_base[i]) == std::toupper(letter)) {
                    if (i < m_wheelLetterRenderPos.size()) {
                        hintStartPos = m_wheelLetterRenderPos[i];
                    }
                    else if (i < m_wheelCentres.size()) {
                        hintStartPos = m_wheelCentres[i];
                    }
                    break;
                }
            }

            m_anims.push_back({
                static_cast<char>(std::toupper(letter)),
                hintStartPos,
                hintEndPos,
                animationDelay, // Apply delay for this letter
                wordIdx,
                charIdx,
                AnimTarget::Grid
                });
            animationDelay -= 0.03f; // Stagger subsequent letters slightly (negative because anim start time)
        }
    }
    else {
        std::cout << "DEBUG: No letters to reveal for the activated hint (after processing type)." << std::endl;
        // No sound if nothing happens (already handled by earlier returns if specific hints fail)
    }
}

void Game::m_checkWordCompletion(int wordIdx) {
    if (wordIdx < 0 || wordIdx >= m_grid.size() || wordIdx >= m_sorted.size()) {
        std::cerr << "ERROR: m_checkWordCompletion called with invalid wordIdx: " << wordIdx << std::endl;
        return;
    }

    const std::string& solutionWord = m_sorted[wordIdx].text;

    // If this word is already found, do nothing
    if (m_found.count(solutionWord)) {
        return;
    }

    // Construct the word from the grid
    std::string gridWordStr;
    gridWordStr.reserve(m_grid[wordIdx].size());
    bool wordIsFullyRevealedOnGrid = true;
    for (char ch : m_grid[wordIdx]) {
        if (ch == '_') {
            wordIsFullyRevealedOnGrid = false;
            break; // Not fully revealed yet
        }
        gridWordStr += ch;
    }

    // Check if the fully revealed grid word matches the solution
    if (wordIsFullyRevealedOnGrid) {
        std::string gridWordUpper = gridWordStr;
        std::string solutionWordUpper = solutionWord;
        std::transform(gridWordUpper.begin(), gridWordUpper.end(), gridWordUpper.begin(), ::toupper);
        std::transform(solutionWordUpper.begin(), solutionWordUpper.end(), solutionWordUpper.begin(), ::toupper);

        if (gridWordUpper == solutionWordUpper) {
            std::cout << "DEBUG: Word '" << solutionWord << "' completed by hint/auto-reveal." << std::endl;
            m_found.insert(solutionWord);

            int baseScore = static_cast<int>(solutionWord.length()) * 10;
            int rarityBonus = (m_sorted[wordIdx].rarity > 1) ? (m_sorted[wordIdx].rarity * 25) : 0;
            int wordScoreForThisWord = baseScore + rarityBonus; // <<< NEW LINE

            m_currentScore += wordScoreForThisWord;
            m_spawnScoreFlourish(wordScoreForThisWord, wordIdx); // <<< NEW LINE

            if (m_scoreValueText) {
                m_scoreValueText->setString(std::to_string(m_currentScore));
            }

            std::cout << "HINT-COMPLETED Word: " << solutionWord << " | Rarity: " << m_sorted[wordIdx].rarity
                << " | Len: " << solutionWord.length() << " | Rarity Bonus: " << rarityBonus
                << " | BasePts: " << baseScore << " | Total: " << m_currentScore << std::endl;

            if (m_found.size() == m_solutions.size()) {
                std::cout << "DEBUG: All grid words found (via hint)! Puzzle solved." << std::endl;
                if (m_winSound) m_winSound->play();
                m_gameState = GState::Solved;
                m_currentScreen = GameScreen::GameOver;
                m_updateLayout(m_window.getSize());
            }
        }
    }
}

void Game::m_spawnScoreFlourish(int points, int wordIdxOnGrid) {
    if (points == 0) return;
    if (wordIdxOnGrid < 0 || static_cast<std::size_t>(wordIdxOnGrid) >= m_sorted.size()) {
        std::cerr << "ERROR: m_spawnScoreFlourish - Invalid wordIdxOnGrid: " << wordIdxOnGrid << std::endl;
        return;
    }

    ScoreFlourishParticle particle;

    // 1. Set Text Data
    particle.textString = "+" + std::to_string(points);
    particle.color = sf::Color(255, 215, 0, 255); // Gold-like color, full alpha initially

    // 2. Determine Initial Position
    const std::string& solvedWordText = m_sorted[wordIdxOnGrid].text;
    if (solvedWordText.empty()) { /* ... error handling ... */ return; }
    if (static_cast<int>(solvedWordText.length()) - 1 < 0) { /* ... error handling ... */ return; }


    sf::Vector2f firstLetterPos = m_tilePos(wordIdxOnGrid, 0);
    sf::Vector2f lastLetterPos = m_tilePos(wordIdxOnGrid, static_cast<int>(solvedWordText.length()) - 1);

    float renderTileScaleFactor = (m_currentGridLayoutScale < 1.0f) ? GRID_TILE_RELATIVE_SCALE_WHEN_SHRUNK : 1.0f;
    const float finalRenderTileSize = S(this, TILE_SIZE) * renderTileScaleFactor;

    sf::Vector2f firstLetterCenter = firstLetterPos + sf::Vector2f(finalRenderTileSize / 2.f, finalRenderTileSize / 2.f);
    sf::Vector2f lastLetterCenter = lastLetterPos + sf::Vector2f(finalRenderTileSize / 2.f, finalRenderTileSize / 2.f);

    particle.position = {
        (firstLetterCenter.x + lastLetterCenter.x) / 2.f,
        firstLetterCenter.y - (finalRenderTileSize * 0.5f)
    };

    // 3. Set Movement Properties
    particle.velocity = {
        randRange(-SCORE_FLOURISH_VEL_X_RANGE_DESIGN, SCORE_FLOURISH_VEL_X_RANGE_DESIGN),
        randRange(SCORE_FLOURISH_VEL_Y_MAX_DESIGN, SCORE_FLOURISH_VEL_Y_MIN_DESIGN)
    };

    // 4. Set Lifetime
    particle.lifetime = randRange(SCORE_FLOURISH_LIFETIME_MIN_SEC, SCORE_FLOURISH_LIFETIME_MAX_SEC);
    particle.initialLifetime = particle.lifetime;

    m_scoreFlourishes.push_back(std::move(particle));
}

void Game::m_updateScoreFlourishes(float dt) {
    m_scoreFlourishes.erase(
        std::remove_if(m_scoreFlourishes.begin(), m_scoreFlourishes.end(),
            [&](ScoreFlourishParticle& p) {
                // Update position
                p.position += p.velocity * dt; // position is sf::Vector2f

                // Update lifetime
                p.lifetime -= dt;

                if (p.lifetime <= 0.f) {
                    return true; // Mark for removal
                }

                // Update alpha for fade-out effect
                float alphaRatio = std::max(0.f, p.lifetime / p.initialLifetime);
                p.color.a = static_cast<std::uint8_t>(alphaRatio * 255.f);

                return false; // Keep particle
            }),
        m_scoreFlourishes.end()
    );
}

void Game::m_renderScoreFlourishes(sf::RenderTarget& target) {
    if (m_font.getInfo().family.empty()) { // Don't try to render if font is bad
        std::cerr << "WARNING: m_renderScoreFlourishes - m_font is not loaded or invalid. Skipping flourish rendering." << std::endl;
        return;
    }

    // Calculate scaled font size once
    unsigned int scaledCharacterSize = static_cast<unsigned int>(std::max(8.0f, S(this, SCORE_FLOURISH_FONT_SIZE_BASE_DESIGN)));

    // Construct sf::Text with font, an initial empty string, and character size as per your finding.
    // The string will be updated in the loop.
    sf::Text renderText(m_font, "", scaledCharacterSize);
    renderText.setStyle(sf::Text::Bold); // Assuming always bold for flourish

    for (const auto& p : m_scoreFlourishes) {
        renderText.setString(p.textString);   // Set the specific string for this particle
        renderText.setFillColor(p.color);     // Set the color (handles fading)
        renderText.setPosition(p.position);   // Set the current position

        // Set origin to center the text at p.position
        // This needs to be done after setString and setCharacterSize if bounds depend on them.
        // Since character size is set at construction and string is set here, getLocalBounds should be up-to-date.
        sf::FloatRect textBounds = renderText.getLocalBounds();
        renderText.setOrigin({ textBounds.position.x + textBounds.size.x / 2.f,
                              textBounds.position.y + textBounds.size.y / 2.f });

        target.draw(renderText);
    }
}

void Game::m_spawnHintPointAnimation(const sf::Vector2f& bonusWordTextCenterPos) {
    HintPointAnimParticle particle;
    particle.startPosition = bonusWordTextCenterPos; // Where the "Bonus Word" text is
    particle.currentPosition = particle.startPosition;
    particle.textString = "+1";
    particle.color = sf::Color(255, 215, 0, 255); 

    // Determine target position: the center of m_hintPointsText
    if (m_hintPointsText) {
        sf::FloatRect hintTextGlobalBounds = m_hintPointsText->getGlobalBounds(); // Use global for screen pos
        particle.targetPosition = {
            hintTextGlobalBounds.position.x + hintTextGlobalBounds.size.x / 2.f,
            hintTextGlobalBounds.position.y + hintTextGlobalBounds.size.y / 2.f
        };
    }
    else {
        // Fallback if m_hintPointsText is somehow not available
        // This should ideally not happen if layout is done correctly.
        // Target a generic spot on the left.
        std::cerr << "WARNING: m_hintPointsText is null. Hint point anim targeting fallback." << std::endl;
        particle.targetPosition = { S(this, 100.f), S(this, 50.f) }; // Example fallback
    }

    particle.speed = HINT_POINT_ANIM_SPEED;
    particle.t = 0.f; // Start animation

    m_hintPointAnims.push_back(particle);
}

void Game::m_updateHintPointAnims(float dt) {
    m_hintPointAnims.erase(
        std::remove_if(m_hintPointAnims.begin(), m_hintPointAnims.end(),
            [&](HintPointAnimParticle& p) {
                p.t += dt * p.speed;
                if (p.t >= 1.f) {
                    p.t = 1.f; // Clamp
                    // When animation finishes, trigger the flourish of the "Points:" text
                    m_hintPointsTextFlourishTimer = HINT_POINT_TEXT_FLOURISH_DURATION;
                    // Note: The actual increment of m_hintPoints happens when the bonus word is found.
                    // This animation is purely visual for the "+1".
                    return true; // Mark for removal
                }

                // Interpolate position (simple linear for now, can add easing)
                p.currentPosition.x = p.startPosition.x + (p.targetPosition.x - p.startPosition.x) * p.t;
                p.currentPosition.y = p.startPosition.y + (p.targetPosition.y - p.startPosition.y) * p.t;

                // Fade out towards the end of the animation (e.g., last 30%)
                if (p.t > 0.7f) {
                    float fadeRatio = (1.0f - p.t) / 0.3f; // 0.0 at t=1.0, 1.0 at t=0.7
                    p.color.a = static_cast<std::uint8_t>(std::max(0.f, std::min(255.f, fadeRatio * 255.f)));
                }
                return false; // Keep particle
            }),
        m_hintPointAnims.end()
    );

    // Update the "Points:" text flourish timer
    if (m_hintPointsTextFlourishTimer > 0.f) {
        m_hintPointsTextFlourishTimer -= dt;
        if (m_hintPointsTextFlourishTimer < 0.f) {
            m_hintPointsTextFlourishTimer = 0.f;
        }
    }
}

void Game::m_renderHintPointAnims(sf::RenderTarget& target) {
    if (m_font.getInfo().family.empty()) {
        return;
    }

    unsigned int scaledCharSize = static_cast<unsigned int>(std::max(8.0f, S(this, HINT_POINT_ANIM_FONT_SIZE_DESIGN)));

    // Construct with font, empty string (will be set), and char size
    sf::Text renderText(m_font, "", scaledCharSize);
    // No need to setStyle if it's always regular for this animation

    for (const auto& p : m_hintPointAnims) {
        renderText.setString(p.textString); // Should be "+1"
        renderText.setFillColor(p.color);
        renderText.setPosition(p.currentPosition);

        sf::FloatRect textBounds = renderText.getLocalBounds();
        renderText.setOrigin({ textBounds.position.x + textBounds.size.x / 2.f,
                              textBounds.position.y + textBounds.size.y / 2.f });
        target.draw(renderText);
    }
}

// Helper function (can be a private static method or in an anonymous namespace if preferred)
// to check if a word is a main grid solution.
bool Game::isGridSolution(const std::string& wordText) const { // Ensure Game:: is present
    for (const auto& solInfo : m_solutions) { // m_solutions is a member
        if (solInfo.text == wordText) {
            return true;
        }
    }
    return false;
}

// In Game.cpp

// (Ensure PopupDrawItem struct is defined, e.g., in an anonymous namespace)
// namespace {
//     struct PopupDrawItem { /* ... */ };
// }

// In Game.cpp

// (PopupDrawItem struct should be defined, e.g., in an anonymous namespace)

// In Game.cpp

// (PopupDrawItem struct should be defined)

void Game::m_renderBonusWordsPopup(sf::RenderTarget& target) {
    if (m_font.getInfo().family.empty()) return;

    // --- Constants ---
    const float POPUP_PADDING_BASE = 12.f;         // Padding around the entire pop-up
    const float MAJOR_COL_SPACING_BASE = 10.f;   // Horizontal space between major length groups (3-letter, 4-letter)
    const float MINOR_COL_SPACING_BASE = 8.f;    // Horizontal space between word columns *within* a length group
    const float TITLE_BOTTOM_MARGIN_BASE = 5.f;  // Space below a group title (e.g., "3-Letter Words:")
    const float WORD_LINE_SPACING_BASE = 4.f;    // Vertical space between words in a minor column
    const unsigned int POPUP_WORD_FONT_SIZE_BASE = 13; // Slightly smaller for density
    const unsigned int POPUP_TITLE_FONT_SIZE_BASE = 15;
    const float POPUP_MAX_WIDTH_DESIGN_RATIO = 0.8f;   // Can use more width now
    const float POPUP_MAX_HEIGHT_DESIGN_RATIO = 0.8f;  // Can use more height
    const float POPUP_MIN_TEXT_SCALE = 0.40f;
    const float POPUP_CORNER_RADIUS_BASE = 8.f;
    const int MAX_MINOR_COLS_PER_GROUP = 3; // Max vertical columns for words under one title

    // --- 1. Prepare Data (Cache if needed) ---
    // (This section remains the same)
    if (!m_bonusWordsCacheIsValid) {
        m_cachedBonusWords.clear();
        std::set<std::string> uniqueBonusWordTexts;
        for (const auto& wordInfo : m_allPotentialSolutions) {
            if (!isGridSolution(wordInfo.text)) {
                if (uniqueBonusWordTexts.insert(wordInfo.text).second) {
                    m_cachedBonusWords.push_back(wordInfo);
                }
            }
        }
        std::sort(m_cachedBonusWords.begin(), m_cachedBonusWords.end(),
            [](const WordInfo& a, const WordInfo& b) {
                if (a.text.length() != b.text.length()) {
                    return a.text.length() < b.text.length();
                }
                return a.text < b.text;
            });
        m_bonusWordsCacheIsValid = true;
    }
    if (m_cachedBonusWords.empty()) return;

    // Group words by length (still useful)
    std::map<int, std::vector<WordInfo>> bonusWordsByLength;
    for (const auto& wi : m_cachedBonusWords) {
        bonusWordsByLength[static_cast<int>(wi.text.length())].push_back(wi);
    }

    // --- 2. Determine Text Scale ---
    // This requires estimating the layout with a trial scale.
    // We need to estimate total width if all groups were side-by-side,
    // and max height of any group.

    float textRenderScale = 1.0f;
    const float maxPopupDesignWidth = static_cast<float>(REF_W) * POPUP_MAX_WIDTH_DESIGN_RATIO;
    const float maxPopupDesignHeight = static_cast<float>(REF_H) * POPUP_MAX_HEIGHT_DESIGN_RATIO;
    const float actualPadding = S(this, POPUP_PADDING_BASE); // Padding for the whole popup

    // Estimate required width and height at scale 1.0
    float estimatedTotalWidthAtScale1 = 0;
    float estimatedMaxGroupHeightAtScale1 = 0;
    sf::Text measureText(m_font, "", 10);

    bool firstMajorCol = true;
    for (const auto& pair_len_words : bonusWordsByLength) {
        int length = pair_len_words.first;
        const std::vector<WordInfo>& words = pair_len_words.second;
        if (words.empty()) continue;

        if (!firstMajorCol) {
            estimatedTotalWidthAtScale1 += S(this, MAJOR_COL_SPACING_BASE);
        }
        firstMajorCol = false;

        // Title
        measureText.setString(std::to_string(length) + "-Letter Words:");
        measureText.setCharacterSize(S(this, static_cast<float>(POPUP_TITLE_FONT_SIZE_BASE)));
        sf::FloatRect titleBounds = measureText.getLocalBounds();
        float currentGroupWidth = titleBounds.position.x + titleBounds.size.x;
        float currentGroupHeight = titleBounds.position.y + titleBounds.size.y + S(this, TITLE_BOTTOM_MARGIN_BASE);

        // Words (assume 1 minor column for this initial width estimate for the group)
        float maxWordWidthInGroup = 0;
        int wordCountInGroup = 0;
        for (const auto& wordInfo : words) {
            wordCountInGroup++;
            std::string tempDisp = std::string(wordInfo.text.length(), '*'); // Use stars for width
            measureText.setString(tempDisp);
            measureText.setCharacterSize(S(this, static_cast<float>(POPUP_WORD_FONT_SIZE_BASE)));
            sf::FloatRect wordB = measureText.getLocalBounds();
            maxWordWidthInGroup = std::max(maxWordWidthInGroup, wordB.position.x + wordB.size.x);
            currentGroupHeight += (wordB.position.y + wordB.size.y);
            if (wordCountInGroup < words.size()) currentGroupHeight += S(this, WORD_LINE_SPACING_BASE);
        }
        currentGroupWidth = std::max(currentGroupWidth, maxWordWidthInGroup);
        estimatedTotalWidthAtScale1 += currentGroupWidth;
        estimatedMaxGroupHeightAtScale1 = std::max(estimatedMaxGroupHeightAtScale1, currentGroupHeight);
    }
    estimatedTotalWidthAtScale1 += 2 * actualPadding; // Add padding for the whole pop-up
    estimatedMaxGroupHeightAtScale1 += 2 * actualPadding;

    // Calculate scale based on width and height constraints
    float scaleBasedOnWidth = 1.0f;
    if (estimatedTotalWidthAtScale1 > maxPopupDesignWidth && maxPopupDesignWidth > 0) {
        float contentWidth = estimatedTotalWidthAtScale1 - 2 * actualPadding;
        float targetContentWidth = maxPopupDesignWidth - 2 * actualPadding;
        if (contentWidth > 0 && targetContentWidth > 0) {
            scaleBasedOnWidth = targetContentWidth / contentWidth;
        }
        else if (targetContentWidth <= 0) {
            scaleBasedOnWidth = POPUP_MIN_TEXT_SCALE * 0.8f;
        }
    }
    float scaleBasedOnHeight = 1.0f;
    if (estimatedMaxGroupHeightAtScale1 > maxPopupDesignHeight && maxPopupDesignHeight > 0) {
        float contentHeight = estimatedMaxGroupHeightAtScale1 - 2 * actualPadding;
        float targetContentHeight = maxPopupDesignHeight - 2 * actualPadding;
        if (contentHeight > 0 && targetContentHeight > 0) {
            scaleBasedOnHeight = targetContentHeight / contentHeight;
        }
        else if (targetContentHeight <= 0) {
            scaleBasedOnHeight = POPUP_MIN_TEXT_SCALE * 0.8f;
        }
    }
    textRenderScale = std::min({ scaleBasedOnWidth, scaleBasedOnHeight, 1.0f });
    textRenderScale = std::max(textRenderScale, POPUP_MIN_TEXT_SCALE);

    // --- 3. Prepare Final Scaled Items & Layout Info for Each Major Group ---
    const unsigned int finalWordFontSize = static_cast<unsigned int>(S(this, static_cast<float>(POPUP_WORD_FONT_SIZE_BASE)) * textRenderScale);
    const unsigned int finalTitleFontSize = static_cast<unsigned int>(S(this, static_cast<float>(POPUP_TITLE_FONT_SIZE_BASE)) * textRenderScale);

    // Scaled spacing values (these are scaled by textRenderScale as they are relative to text size)
    const float actualMajorColSpacing = S(this, MAJOR_COL_SPACING_BASE) * textRenderScale;
    const float actualMinorColSpacing = S(this, MINOR_COL_SPACING_BASE) * textRenderScale;
    const float actualTitleBottomMargin = S(this, TITLE_BOTTOM_MARGIN_BASE) * textRenderScale;
    const float actualWordLineSpacing = S(this, WORD_LINE_SPACING_BASE) * textRenderScale;

    struct MajorGroupRenderData {
        std::string title;
        sf::Vector2f titleSize;
        std::vector<std::vector<PopupDrawItem>> minorColumns; // Words distributed into minor columns
        float totalWidth;  // Total width of this major group (title or widest minor col sum)
        float totalHeight; // Total height of this major group
    };
    std::vector<MajorGroupRenderData> majorGroups;
    float finalTotalContentWidth = 0;
    float finalMaxContentHeight = 0;

    firstMajorCol = true; // Reset for this pass
    for (const auto& pair_len_words : bonusWordsByLength) {
        int length = pair_len_words.first;
        const std::vector<WordInfo>& wordsInGroup = pair_len_words.second;
        if (wordsInGroup.empty()) continue;

        if (!firstMajorCol) {
            finalTotalContentWidth += actualMajorColSpacing;
        }
        firstMajorCol = false;

        MajorGroupRenderData currentGroup;
        currentGroup.title = std::to_string(length) + "-Letter Words:";
        measureText.setString(currentGroup.title);
        measureText.setCharacterSize(finalTitleFontSize);
        sf::FloatRect titleB = measureText.getLocalBounds();
        currentGroup.titleSize = { titleB.position.x + titleB.size.x, titleB.position.y + titleB.size.y };

        currentGroup.totalHeight = currentGroup.titleSize.y + actualTitleBottomMargin;
        float maxWordTextWidthThisGroup = 0;

        // Prepare word items for this group
        std::vector<PopupDrawItem> wordsToDistribute;
        for (const auto& wordInfo : wordsInGroup) {
            std::string displayText;
            bool isFound = m_foundBonusWords.count(wordInfo.text);
            if (isFound) {
                displayText = wordInfo.text;
                std::transform(displayText.begin(), displayText.end(), displayText.begin(),
                    [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
            }
            else {
                displayText = std::string(wordInfo.text.length(), '*');
            }
            measureText.setString(displayText);
            measureText.setCharacterSize(finalWordFontSize);
            sf::FloatRect wordB = measureText.getLocalBounds();
            wordsToDistribute.push_back({ displayText, false, wordB.position.y + wordB.size.y, wordB.position.x + wordB.size.x, (isFound ? m_currentTheme.hudTextFound : m_currentTheme.menuButtonText) });
            maxWordTextWidthThisGroup = std::max(maxWordTextWidthThisGroup, wordB.position.x + wordB.size.x);
        }

        // Distribute wordsToDistribute into minor columns for this major group
        int numMinorCols = 1;
        float totalWordListHeight = 0;
        for (size_t i = 0; i < wordsToDistribute.size(); ++i) {
            totalWordListHeight += wordsToDistribute[i].height;
            if (i < wordsToDistribute.size() - 1) totalWordListHeight += actualWordLineSpacing;
        }

        float maxContentHeightForMinorCols = maxPopupDesignHeight - 2 * actualPadding - currentGroup.titleSize.y - actualTitleBottomMargin;

        if (totalWordListHeight > maxContentHeightForMinorCols && wordsToDistribute.size() > 1) { // only try more cols if it's too tall for 1 AND there's more than 1 word
            for (int tryMinor = 2; tryMinor <= MAX_MINOR_COLS_PER_GROUP; ++tryMinor) {
                if ((totalWordListHeight / tryMinor) + ((wordsToDistribute.size() / tryMinor - 1) * actualWordLineSpacing / tryMinor) < maxContentHeightForMinorCols) {
                    numMinorCols = tryMinor;
                    break;
                }
                if (tryMinor == MAX_MINOR_COLS_PER_GROUP) numMinorCols = MAX_MINOR_COLS_PER_GROUP; // Use max if still no fit
            }
        }

        currentGroup.minorColumns.resize(numMinorCols);
        std::vector<float> minorColumnHeights(numMinorCols, 0.f);
        int currentMinorColIdx = 0;
        for (size_t i = 0; i < wordsToDistribute.size(); ++i) {
            const auto& wordItem = wordsToDistribute[i];
            // Simple distribution: fill columns one by one
            // A better approach might be to find the shortest column, but for fixed maxMinorCols, this is simpler
            if (currentMinorColIdx >= numMinorCols) currentMinorColIdx = 0; // Should not happen with good logic but safety.

            // Check if adding to currentMinorColIdx would overflow (simplified check)
            if (minorColumnHeights[currentMinorColIdx] > 0 && // if not the first item in this col
                minorColumnHeights[currentMinorColIdx] + actualWordLineSpacing + wordItem.height > maxContentHeightForMinorCols &&
                currentMinorColIdx < numMinorCols - 1) {
                currentMinorColIdx++;
            }

            currentGroup.minorColumns[currentMinorColIdx].push_back(wordItem);
            if (minorColumnHeights[currentMinorColIdx] > 0) minorColumnHeights[currentMinorColIdx] += actualWordLineSpacing;
            minorColumnHeights[currentMinorColIdx] += wordItem.height;

            if (i < wordsToDistribute.size() - 1 && currentMinorColIdx == numMinorCols - 1 && currentGroup.minorColumns[currentMinorColIdx].size() >= (wordsToDistribute.size() / numMinorCols)) {
                // If filling last column and it's getting its share, try to wrap to first for next item
                // This attempts a more balanced fill if strict height limit is hit.
                // This part is tricky and might need refinement. For now, stick to simpler fill.
            }
            if (currentMinorColIdx < numMinorCols - 1 && // If not last column
                minorColumnHeights[currentMinorColIdx] >= (totalWordListHeight / numMinorCols)) { // And it received its "fair share"
                currentMinorColIdx++; // Move to next minor column
            }

        }

        float thisGroupMinorColsWidth = numMinorCols * maxWordTextWidthThisGroup + (numMinorCols > 1 ? (numMinorCols - 1) * actualMinorColSpacing : 0);
        currentGroup.totalWidth = std::max(currentGroup.titleSize.x, thisGroupMinorColsWidth);

        float maxMinorColH = 0;
        for (float h : minorColumnHeights) maxMinorColH = std::max(maxMinorColH, h);
        currentGroup.totalHeight += maxMinorColH; // Add height of the tallest minor column

        majorGroups.push_back(currentGroup);
        finalTotalContentWidth += currentGroup.totalWidth;
        finalMaxContentHeight = std::max(finalMaxContentHeight, currentGroup.totalHeight);
    }

    // --- 4. Final Pop-up Dimensions ---
    float finalPopupWidth = finalTotalContentWidth + 2 * actualPadding;
    float finalPopupHeight = finalMaxContentHeight + 2 * actualPadding;

    // Cap overall popup size
    finalPopupWidth = std::min(finalPopupWidth, maxPopupDesignWidth);
    finalPopupHeight = std::min(finalPopupHeight, maxPopupDesignHeight);

    float popupX = (static_cast<float>(REF_W) - finalPopupWidth) / 2.f;
    float popupY = (static_cast<float>(REF_H) - finalPopupHeight) / 2.f;

    // --- 5. Draw Background ---
    RoundedRectangleShape popupBackground({ finalPopupWidth, finalPopupHeight }, S(this, POPUP_CORNER_RADIUS_BASE), 10);
    popupBackground.setPosition({ popupX, popupY });
    popupBackground.setFillColor(m_currentTheme.menuBg);
    popupBackground.setOutlineColor(m_currentTheme.menuButtonHover);
    popupBackground.setOutlineThickness(S(this, 1.5f));
    target.draw(popupBackground);

    // --- 6. Draw Content ---
    sf::Text drawTextObj(m_font, "", 10);
    float currentMajorColX = popupX + actualPadding;

    for (const auto& group : majorGroups) {
        if (currentMajorColX + group.totalWidth > popupX + finalPopupWidth - actualPadding + 1.f) break; // Stop if no more horizontal space

        float currentDrawY = popupY + actualPadding;

        // Draw Title for Major Group
        drawTextObj.setString(group.title);
        drawTextObj.setCharacterSize(finalTitleFontSize);
        drawTextObj.setFillColor(m_currentTheme.menuTitleText);
        sf::FloatRect titleB_draw = drawTextObj.getLocalBounds();
        drawTextObj.setPosition({ currentMajorColX - titleB_draw.position.x, currentDrawY - titleB_draw.position.y });
        target.draw(drawTextObj);
        currentDrawY += group.titleSize.y + actualTitleBottomMargin;

        // Draw Minor Columns for this Major Group
        float currentMinorColX = currentMajorColX;
        float maxWordWidthInThisGroup = 0; // Find max word width for this group for minor col spacing
        for (const auto& minorCol : group.minorColumns) {
            for (const auto& item : minorCol) {
                if (!item.isTitle) maxWordWidthInThisGroup = std::max(maxWordWidthInThisGroup, item.width);
            }
        }


        for (const auto& minorCol : group.minorColumns) {
            float minorColDrawY = currentDrawY; // Each minor col starts below the title
            if (currentMinorColX + maxWordWidthInThisGroup > popupX + finalPopupWidth - actualPadding + 1.f && &minorCol != &group.minorColumns[0]) break; // Stop if no space for this minor col

            for (const auto& item : minorCol) {
                if (minorColDrawY + item.height > popupY + finalPopupHeight - actualPadding + 1.f) break; // Vertical clipping

                drawTextObj.setString(item.textDisplay);
                drawTextObj.setCharacterSize(finalWordFontSize); // All items in minor cols are words
                drawTextObj.setFillColor(item.color);
                sf::FloatRect itemTextBounds = drawTextObj.getLocalBounds();
                drawTextObj.setPosition({
                    currentMinorColX - itemTextBounds.position.x,
                    minorColDrawY - itemTextBounds.position.y
                    });
                target.draw(drawTextObj);
                minorColDrawY += item.height + actualWordLineSpacing;
            }
            currentMinorColX += maxWordWidthInThisGroup + actualMinorColSpacing;
        }
        currentMajorColX += group.totalWidth + actualMajorColSpacing;
    }
}
