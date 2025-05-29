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

namespace { // Anonymous namespace for helper functions
    void centerTextOnShape_General(sf::Text& text, const sf::Shape& shape) {
        // 1. Set the text's origin to its own visual center.
        sf::FloatRect textLocalBounds = text.getLocalBounds();
        text.setOrigin(sf::Vector2f( // Pass a single sf::Vector2f
            textLocalBounds.position.x + textLocalBounds.size.x / 2.f,
            textLocalBounds.position.y + textLocalBounds.size.y / 2.f
        ));

        // 2. Get the shape's global (visual) bounding box on the screen.
        sf::FloatRect shapeGlobalBounds = shape.getGlobalBounds();

        // 3. Calculate the visual center of the shape on the screen.
        sf::Vector2f shapeVisualCenter(
            shapeGlobalBounds.position.x + shapeGlobalBounds.size.x / 2.f,
            shapeGlobalBounds.position.y + shapeGlobalBounds.size.y / 2.f
        );

        // 4. Position the text (whose origin is now its center) at the shape's visual center.
        text.setPosition(shapeVisualCenter); // setPosition also takes a single sf::Vector2f
    }
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
    m_window(),                              
    m_font(),                                
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
    m_hintFrameSprites(4),
    m_hintIndicatorLightSprs(4),
    m_hintClickableRegions(4),
    m_hoveredHintIndex(-1),
    m_hintPopupBackground(sf::Vector2f(200.f, 100.f), 8.f, 10),
    m_scrambleTex(), m_sapphireTex(), m_rubyTex(), m_diamondTex(),
    m_selectBuffer(), m_placeBuffer(), m_winBuffer(), m_clickBuffer(), m_hintUsedBuffer(), m_errorWordBuffer(),
    m_selectSound(nullptr), m_placeSound(nullptr), m_winSound(nullptr), m_clickSound(nullptr), m_hintUsedSound(nullptr), m_errorWordSound(nullptr),
    m_backgroundMusic(),
    m_scrambleSpr(nullptr), m_sapphireSpr(nullptr), m_rubySpr(nullptr), m_diamondSpr(nullptr),
    m_contTxt(nullptr), m_scoreLabelText(nullptr), m_scoreValueText(nullptr), m_hintCountTxt(nullptr),
    m_mainMenuTitle(nullptr), m_casualButtonText(nullptr), m_competitiveButtonText(nullptr), m_quitButtonText(nullptr),
    m_casualMenuTitle(nullptr), m_easyButtonText(nullptr), m_mediumButtonText(nullptr), m_hardButtonText(nullptr), m_returnButtonText(nullptr),
    m_guessDisplay_Text(nullptr),
    m_progressMeterText(nullptr),
    m_mainBackgroundSpr(nullptr),
    m_returnToMenuButtonText(nullptr),
    m_hintFrameClickAnimTimers(4, 0.f), 
    m_hintFrameClickColor(sf::Color::Green),
    m_hintFrameNormalColor(sf::Color::White),
    m_contBtn(sf::Vector2f(200.f, 50.f), 10.f, 10),
    m_solvedOverlay(sf::Vector2f(100.f, 50.f), 10.f, 10),
    m_scoreBar(sf::Vector2f(100.f, 30.f), 10.f, 10),
    m_guessDisplay_Bg(sf::Vector2f(50.f, 30.f), 5.f, 10),
    m_debugDrawCircleMode(false),
    m_needsLayoutUpdate(false),
    m_lastKnownSize(sf::Vector2u(0, 0)), 
    m_mainMenuBg(sf::Vector2f(300.f, 300.f), 15.f, 10),
    m_casualButtonShape(sf::Vector2f(250.f, 50.f), 10.f, 10),
    m_competitiveButtonShape(sf::Vector2f(250.f, 50.f), 10.f, 10),
    m_quitButtonShape(sf::Vector2f(250.f, 50.f), 10.f, 10),
    m_casualMenuBg(sf::Vector2f(300.f, 400.f), 15.f, 10),
    m_easyButtonShape(sf::Vector2f(250.f, 50.f), 10.f, 10),
    m_mediumButtonShape(sf::Vector2f(250.f, 50.f), 10.f, 10),
    m_hardButtonShape(sf::Vector2f(250.f, 50.f), 10.f, 10),
    m_returnButtonShape(sf::Vector2f(250.f, 50.f), 10.f, 10),
    m_progressMeterBg(sf::Vector2f(100.f, 20.f)),
    m_progressMeterFill(sf::Vector2f(100.f, 20.f)),
    m_returnToMenuButtonShape(sf::Vector2f(100.f, 40.f), 8.f, 10),
    m_isHoveringHintPointsText(false),
    m_bonusWordsCacheIsValid(false),
    m_showDebugZones(false),
    m_bonusListCompleteEffectActive(false),
    m_bonusListCompleteAnimTimer(0.f),
    m_bonusListCompletePopupDisplayTimer(0.f),
    m_bonusListCompletePointsAwarded(0),
    m_bonusListCompletePopupText(m_font, "", 0),
    m_bonusListCompleteAnimatingPointsText(m_font, "", 0),
    m_firstFrame(true)
{
    const sf::Vector2u desiredInitialSize{ 1000u, 800u };
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    unsigned int initialWidth = std::min(desiredInitialSize.x, desktop.size.x);
    unsigned int initialHeight = std::min(desiredInitialSize.y, desktop.size.y);

    m_window.create(sf::VideoMode({ initialWidth, initialHeight }), "Word Puzzle", sf::Style::Default); // Pass sf::Vector2u directly
    m_window.setFramerateLimit(60);
    m_window.setVerticalSyncEnabled(true);

    m_loadResources();

    if (m_selectBuffer.getSampleCount() > 0) m_selectSound = std::make_unique<sf::Sound>(m_selectBuffer);
    if (m_placeBuffer.getSampleCount() > 0) m_placeSound = std::make_unique<sf::Sound>(m_placeBuffer);
    if (m_winBuffer.getSampleCount() > 0) m_winSound = std::make_unique<sf::Sound>(m_winBuffer);
    if (m_clickBuffer.getSampleCount() > 0) m_clickSound = std::make_unique<sf::Sound>(m_clickBuffer);
    if (m_hintUsedBuffer.getSampleCount() > 0) m_hintUsedSound = std::make_unique<sf::Sound>(m_hintUsedBuffer);
    if (m_errorWordBuffer.getSampleCount() > 0) m_errorWordSound = std::make_unique<sf::Sound>(m_errorWordBuffer);

    if (m_scrambleTex.getSize().x > 0) m_scrambleSpr = std::make_unique<sf::Sprite>(m_scrambleTex);
    if (m_sapphireTex.getSize().x > 0) m_sapphireSpr = std::make_unique<sf::Sprite>(m_sapphireTex);
    if (m_rubyTex.getSize().x > 0) m_rubySpr = std::make_unique<sf::Sprite>(m_rubyTex);
    if (m_diamondTex.getSize().x > 0) m_diamondSpr = std::make_unique<sf::Sprite>(m_diamondTex);

    // Setup new hint panel sprites (plural)
    if (m_hintFrameTexture.getSize().x > 0) { // Check if the frame texture loaded
        for (size_t i = 0; i < m_hintFrameSprites.size(); ++i) { // Should be 4
            m_hintFrameSprites[i] = std::make_unique<sf::Sprite>(m_hintFrameTexture);
        }
    }

    // Setup hint indicator light sprites (remains the same)
    if (m_hintIndicatorLightTex.getSize().x > 0) {
        for (size_t i = 0; i < m_hintIndicatorLightSprs.size(); ++i) {
            m_hintIndicatorLightSprs[i] = std::make_unique<sf::Sprite>(m_hintIndicatorLightTex);
        }
    }

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

    sf::Color debugColor = sf::Color(255, 0, 0, 100);
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

    m_updateView(m_window.getSize());
    m_rebuild();
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
        if (!m_font.openFromFile("E:/UdemyCoursesProjects/WordPuzzle/SFML_TestProject/fonts/arialbd.ttf")) { // Fallback path
            std::cerr << "FATAL Error loading font. Exiting.\n"; exit(1);
        }
    }

    // --- Load Hint Frame Texture ---
    if (!m_hintFrameTexture.loadFromFile("assets/HintButtonFrame.png")) {
        std::cerr << "CRITICAL ERROR: Could not load hint frame texture (assets/HintButtonFrame.png)!" << std::endl;
    }
    else {
        m_hintFrameTexture.setSmooth(true);
    }

    // --- Load Hint Indicator Light Texture --- (remains the same)
    if (!m_hintIndicatorLightTex.loadFromFile("assets/LightOn_small.png")) {
        std::cerr << "CRITICAL ERROR: Could not load hint indicator light texture (assets/LightOn_small.png)!" << std::endl;
    }
    else {
        m_hintIndicatorLightTex.setSmooth(true);
    }

    // --- Load New Main Background Texture ---
    if (!m_mainBackgroundTex.loadFromFile("assets/BackgroundandFrame.png")) {
        std::cerr << "CRITICAL ERROR: Could not load main background texture!" << std::endl;
        exit(1);
    }
    m_mainBackgroundTex.setSmooth(true);

    // Load scramble button texture
    if (!m_scrambleTex.loadFromFile("assets/ScrambleButton.png")) {
        std::cerr << "Error loading scramble texture!" << std::endl;
    }
    else {
        m_scrambleTex.setSmooth(true);
    }

    // --- Create Text Objects FIRST (as some might be used by other resource setups) ---
    // (Moved some text creation higher as good practice, though not strictly necessary for this error)
    m_returnToMenuButtonText = std::make_unique<sf::Text>(m_font, "Menu", 20);
    m_progressMeterText = std::make_unique<sf::Text>(m_font, "", 16);
    m_progressMeterText->setFillColor(sf::Color::White);
    m_guessDisplay_Text = std::make_unique<sf::Text>(m_font, "", 30);
    m_guessDisplay_Text->setFillColor(sf::Color::White);
    m_contTxt = std::make_unique<sf::Text>(m_font, "Continue", 24);
    m_scoreLabelText = std::make_unique<sf::Text>(m_font, "SCORE:", SCORE_ZONE_LABEL_FONT_SIZE);
    m_scoreValueText = std::make_unique<sf::Text>(m_font, "0", SCORE_ZONE_VALUE_FONT_SIZE);
    m_hintCountTxt = std::make_unique<sf::Text>(m_font, "", 20); // Potentially redundant with m_hintPointsText
    m_mainMenuTitle = std::make_unique<sf::Text>(m_font, "Main Menu", 36);
    m_casualButtonText = std::make_unique<sf::Text>(m_font, "Casual", 24);
    m_competitiveButtonText = std::make_unique<sf::Text>(m_font, "Competitive", 24);
    m_quitButtonText = std::make_unique<sf::Text>(m_font, "Quit", 24);
    m_hintRevealFirstButtonText = std::make_unique<sf::Text>(m_font, "Letter", 18);
    m_hintRevealFirstButtonText->setFillColor(sf::Color::White);
    m_hintPointsText = std::make_unique<sf::Text>(m_font, "Points: 0", 20);
    m_hintPointsText->setFillColor(sf::Color::White);
    m_hintRevealFirstCostText = std::make_unique<sf::Text>(m_font, "Cost: " + std::to_string(HINT_COST_REVEAL_FIRST), 16);
    m_hintRevealFirstCostText->setFillColor(sf::Color::White);
    m_hintRevealRandomButtonText = std::make_unique<sf::Text>(m_font, "Random", 18);
    m_hintRevealRandomButtonText->setFillColor(sf::Color::White);
    m_hintRevealRandomCostText = std::make_unique<sf::Text>(m_font, "Cost: " + std::to_string(HINT_COST_REVEAL_RANDOM), 16);
    m_hintRevealRandomCostText->setFillColor(sf::Color::White);
    m_hintRevealLastButtonText = std::make_unique<sf::Text>(m_font, "Full Word", 18);
    m_hintRevealLastButtonText->setFillColor(sf::Color::White);
    m_hintRevealLastCostText = std::make_unique<sf::Text>(m_font, "Cost: " + std::to_string(HINT_COST_REVEAL_LAST), 16);
    m_hintRevealLastCostText->setFillColor(sf::Color::White);
    m_hintRevealFirstOfEachButtonText = std::make_unique<sf::Text>(m_font, "1st of Each", 18);
    m_hintRevealFirstOfEachButtonText->setFillColor(sf::Color::White);
    m_hintRevealFirstOfEachCostText = std::make_unique<sf::Text>(m_font, "Cost: " + std::to_string(HINT_COST_REVEAL_FIRST_OF_EACH), 16);
    m_hintRevealFirstOfEachCostText->setFillColor(sf::Color::White);
    m_casualMenuTitle = std::make_unique<sf::Text>(m_font, "Casual", 36);
    m_easyButtonText = std::make_unique<sf::Text>(m_font, "Easy", 24);
    m_mediumButtonText = std::make_unique<sf::Text>(m_font, "Medium", 24);
    m_hardButtonText = std::make_unique<sf::Text>(m_font, "Hard", 24);
    m_returnButtonText = std::make_unique<sf::Text>(m_font, "Return", 24);
    m_bonusWordsInHintZoneText = std::make_unique<sf::Text>(m_font, "Bonus Words: 0/0", 18.f); // Example size
    if (m_bonusWordsInHintZoneText) m_bonusWordsInHintZoneText->setFillColor(sf::Color(230, 230, 230, 220));

    // --- NEW: Hint Hover Pop-up Texts ---
    m_popupAvailablePointsText = std::make_unique<sf::Text>(m_font, "Points: 0", 16);
    m_popupHintCostText = std::make_unique<sf::Text>(m_font, "Cost: 0", 16);
    m_popupHintDescriptionText = std::make_unique<sf::Text>(m_font, "Description here", 14); // Example size
    if (m_popupAvailablePointsText) m_popupAvailablePointsText->setFillColor(sf::Color::White);
    if (m_popupHintCostText) m_popupHintCostText->setFillColor(sf::Color::White);
    if (m_popupHintDescriptionText) m_popupHintDescriptionText->setFillColor(sf::Color(200, 200, 200));

    if (m_contTxt) m_contTxt->setFillColor(sf::Color::White); // Example of setting color after creation
    if (m_returnToMenuButtonText) m_returnToMenuButtonText->setFillColor(sf::Color::White);


    // Gem Textures
    if (!m_sapphireTex.loadFromFile("assets/emerald.png")) {
        std::cerr << "Error loading sapphire texture (assets/emerald.png)!" << std::endl;
    }
    else { m_sapphireTex.setSmooth(true); }
    if (!m_rubyTex.loadFromFile("assets/ruby.png")) {
        std::cerr << "Error loading ruby texture (assets/ruby.png)!" << std::endl;
    }
    else { m_rubyTex.setSmooth(true); }
    if (!m_diamondTex.loadFromFile("assets/diamond.png")) {
        std::cerr << "Error loading diamond texture (assets/diamond.png)!" << std::endl;
    }
    else { m_diamondTex.setSmooth(true); }

    // Sound Buffers
    bool selectLoaded = m_selectBuffer.loadFromFile("assets/sounds/select_letter.wav");
    if (!selectLoaded) { std::cerr << "Error loading select_letter sound\n"; }
    bool placeLoaded = m_placeBuffer.loadFromFile("assets/sounds/place_letter.wav");
    if (!placeLoaded) { std::cerr << "Error loading place_letter sound\n"; }
    bool winLoaded = m_winBuffer.loadFromFile("assets/sounds/puzzle_solved.wav");
    if (!winLoaded) { std::cerr << "Error loading puzzle_solved sound\n"; }
    bool clickLoaded = m_clickBuffer.loadFromFile("assets/sounds/button_click.wav");
    if (!clickLoaded) { std::cerr << "Error loading button_click sound\n"; }
    bool hintUsedLoaded = m_hintUsedBuffer.loadFromFile("assets/sounds/button_click.wav"); // Same as click for now
    if (!hintUsedLoaded) { std::cerr << "Error loading hint_used sound\n"; }
    bool errorWordLoaded = m_errorWordBuffer.loadFromFile("assets/sounds/hint_used.mp3"); // This was hint_used.mp3, maybe error.wav?
    if (!errorWordLoaded) { std::cerr << "Error loading error_word sound (hint_used.mp3)\n"; }


    // Create Sounds (Link Buffers)
    if (selectLoaded) { m_selectSound = std::make_unique<sf::Sound>(m_selectBuffer); }
    if (placeLoaded) { m_placeSound = std::make_unique<sf::Sound>(m_placeBuffer); }
    if (winLoaded) { m_winSound = std::make_unique<sf::Sound>(m_winBuffer); }
    if (clickLoaded) { m_clickSound = std::make_unique<sf::Sound>(m_clickBuffer); }
    if (hintUsedLoaded) { m_hintUsedSound = std::make_unique<sf::Sound>(m_hintUsedBuffer); }
    if (errorWordLoaded) { m_errorWordSound = std::make_unique<sf::Sound>(m_errorWordBuffer); }

    // Create Sprites (Link Textures)
    // Note: m_newHintPanelSpr and m_hintIndicatorLightSprs are created in the constructor
    // *after* m_loadResources is called, using the now-loaded m_newHintPanelTex and m_hintIndicatorLightTex.
    if (m_scrambleTex.getSize().x > 0) m_scrambleSpr = std::make_unique<sf::Sprite>(m_scrambleTex);
    if (m_sapphireTex.getSize().x > 0) m_sapphireSpr = std::make_unique<sf::Sprite>(m_sapphireTex);
    if (m_rubyTex.getSize().x > 0) m_rubySpr = std::make_unique<sf::Sprite>(m_rubyTex);
    if (m_diamondTex.getSize().x > 0) m_diamondSpr = std::make_unique<sf::Sprite>(m_diamondTex);

    if (m_mainBackgroundTex.getSize().x > 0) { // Ensure texture loaded before creating sprite
        m_mainBackgroundSpr = std::make_unique<sf::Sprite>(m_mainBackgroundTex);
        if (m_mainBackgroundSpr) {
            // m_mainBackgroundSpr->setTexture(m_mainBackgroundTex); // Already done by constructor
            m_mainBackgroundSpr->setPosition(sf::Vector2f(0.f, 0.f)); // SFML3: Pass sf::Vector2f
        }
        else {
            std::cerr << "ERROR: Failed to create m_mainBackgroundSpr unique_ptr!" << std::endl;
            exit(1);
        }
    }
    else {
        std::cerr << "ERROR: Main background texture not loaded, cannot create sprite." << std::endl;
        exit(1);
    }


    // Set initial Sprite properties (Example - some of these are already handled in m_updateLayout or constructor)
    if (m_sapphireSpr && m_sapphireTex.getSize().y > 0) { // Check sprite ptr and tex
        float desiredGemHeight_load = TILE_SIZE * 0.60f; // Use local var name
        float gemScale_load = desiredGemHeight_load / static_cast<float>(m_sapphireTex.getSize().y);
        m_sapphireSpr->setScale(sf::Vector2f(gemScale_load, gemScale_load));
        m_sapphireSpr->setOrigin(sf::Vector2f(static_cast<float>(m_sapphireTex.getSize().x) / 2.f, static_cast<float>(m_sapphireTex.getSize().y) / 2.f));
    }
    // ... similar for ruby and diamond ...
    if (m_scrambleSpr && m_scrambleTex.getSize().y > 0) { // Check sprite ptr and tex
        const float scrambleBtnHeight_load = SCRAMBLE_BTN_HEIGHT;
        float scrambleScale_load = scrambleBtnHeight_load / static_cast<float>(m_scrambleTex.getSize().y);
        m_scrambleSpr->setScale(sf::Vector2f(scrambleScale_load, scrambleScale_load));
    }

    // Load Music Files List
    m_musicFiles = { "assets/music/track1.mp3", "assets/music/track2.mp3", "assets/music/track3.mp3", "assets/music/track4.mp3", "assets/music/track5.mp3" };
    m_backgroundMusic.setVolume(40.f);

    // Load Word List
    m_fullWordList = Words::loadProcessedWordList("words_processed.csv");
    if (m_fullWordList.empty()) { std::cerr << "Failed to load word list or list is empty. Exiting." << std::endl; exit(1); }
    m_roots.clear();
    std::vector<int> potentialBaseLengths = { 4, 5, 6, 7 };
    for (int len : potentialBaseLengths) {
        std::vector<WordInfo> wordsOfLength = Words::withLength(m_fullWordList, len);
        m_roots.insert(m_roots.end(), wordsOfLength.begin(), wordsOfLength.end());
    }
    if (m_roots.empty()) { std::cerr << "No suitable root words found in list. Exiting." << std::endl; exit(1); }
    std::cout << "DEBUG: Populated m_roots with " << m_roots.size() << " potential base words (lengths 4-7)." << std::endl;

    // Load Color Themes
    m_themes.clear();
    m_themes = loadThemes();
    if (m_themes.empty()) {
        std::cerr << "CRITICAL Warning: loadThemes() returned empty vector. Using fallback default theme.\n";
        m_themes.push_back({});
    }
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

    // --- Update Hint Frame Click Animation Timers ---
    for (size_t i = 0; i < m_hintFrameClickAnimTimers.size(); ++i) {
        if (m_hintFrameClickAnimTimers[i] > 0.f) {
            m_hintFrameClickAnimTimers[i] -= deltaSeconds;
            if (m_hintFrameClickAnimTimers[i] < 0.f) {
                m_hintFrameClickAnimTimers[i] = 0.f;
            }
        }
    }

    if (m_currentScreen == GameScreen::Playing || m_currentScreen == GameScreen::GameOver) {
        sf::Vector2f mappedMousePos = m_window.mapPixelToCoords(sf::Mouse::getPosition(m_window));

        // For Hint Button Pop-ups
        m_hoveredHintIndex = -1; // Reset
        for (int i = 0; i < static_cast<int>(m_hintClickableRegions.size()); ++i) {
            if (m_hintClickableRegions[i].contains(mappedMousePos)) {
                m_hoveredHintIndex = i;
                break;
            }
        }

        // --- NEW/RESTORED: For Bonus Words List Pop-up (m_renderBonusWordsPopup) ---
        if (m_bonusWordsInHintZoneText) { // Check if the text object exists
            m_isHoveringHintPointsText = m_bonusWordsInHintZoneText->getGlobalBounds().contains(mappedMousePos);
        }
        else {
            m_isHoveringHintPointsText = false;
        }
        // --- END NEW/RESTORED ---

    }
    else {
        m_hoveredHintIndex = -1;
        m_isHoveringHintPointsText = false; // Ensure this is also reset if not on relevant screens
    }
    
    // Update Bonus List Complete Effect 
    m_updateBonusListCompleteEffect(deltaSeconds);



    // Update game elements based on screen
    if (m_currentScreen == GameScreen::Playing || m_currentScreen == GameScreen::GameOver) {
        m_updateAnims(deltaSeconds);
        m_updateScoreFlourishes(deltaSeconds);
        m_updateHintPointAnims(deltaSeconds);
    }
    else if (m_currentScreen == GameScreen::SessionComplete) {
        m_updateCelebrationEffects(deltaSeconds);
    }
    
}

// --- Render ---
void Game::m_render() {
    m_window.clear(m_currentTheme.winBg);

    // --- DRAW NEW MAIN BACKGROUND SPRITE FIRST ---
    if (m_mainBackgroundSpr && m_mainBackgroundTex.getSize().x > 0) { 
        m_window.draw(*m_mainBackgroundSpr); 
    }
    // --- END DRAW NEW MAIN BACKGROUND ---


    //m_decor.draw(m_window); // Draw background decor first

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

    // Render Bonus List Complete Effect
    m_renderBonusListCompleteEffect(m_window);

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
    std::cout << "DEBUG: Final list for grid (m_solutions, ALL " << m_solutions.size() << " words):" << std::endl;
    for (size_t i = 0; i < m_solutions.size(); ++i) { // Loop up to the actual size of m_solutions
        std::cout << "  - '" << m_solutions[i].text
            << "' (Len=" << m_solutions[i].text.length()
            << ", Rarity=" << m_solutions[i].rarity << ")" << std::endl;
    }
    if (!m_sorted.empty()) {
        std::cout << "DEBUG: m_rebuild - First sorted word for grid display: '" << m_sorted[0].text << "'" << std::endl;
    }
    else { std::cout << "DEBUG: m_rebuild - No words selected for the grid." << std::endl; }

    // DEBUG BLOCK FOR BONUS WORDS *****
    std::cout << "DEBUG: Potential Bonus Words (" << m_allPotentialSolutions.size() - m_solutions.size() << " expected):" << std::endl;
    int actualBonusCount = 0;
    if (!m_allPotentialSolutions.empty()) {
        for (const auto& potentialSolutionInfo : m_allPotentialSolutions) {
            // Check if this potential solution is NOT in the main m_solutions list
            bool isGridWord = false;
            for (const auto& gridSolutionInfo : m_solutions) {
                if (potentialSolutionInfo.text == gridSolutionInfo.text) {
                    isGridWord = true;
                    break;
                }
            }

            if (!isGridWord) {
                // It's a bonus word
                std::cout << "  - BONUS: '" << potentialSolutionInfo.text
                    << "' (Len=" << potentialSolutionInfo.text.length()
                    << ", Rarity=" << potentialSolutionInfo.rarity << ")" << std::endl;
                actualBonusCount++;
            }
        }
    }
    if (actualBonusCount == 0 && (m_allPotentialSolutions.size() - m_solutions.size() > 0)) {
        std::cout << "  - (No distinct bonus words found, or all subwords are grid words)" << std::endl;
    }
    else if (actualBonusCount == 0) {
        std::cout << "  - (No bonus words for this puzzle)" << std::endl;
    }
    std::cout << "DEBUG: Actual distinct bonus words found and listed: " << actualBonusCount << std::endl;
    

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
    if (m_currentPuzzleIndex == 0 && m_isInSession) {
        m_currentScore = 0;
        m_wordsSolvedSinceHint = 0;
        //m_hintPoints = 0; Want player to keep points they've earned
        std::cout << "DEBUG: First puzzle of session - Resetting score, hint points, and words solved count." << std::endl;
    }
    else if (m_isInSession) {
        m_wordsSolvedSinceHint = 0;
        std::cout << "DEBUG: Subsequent puzzle - Resetting words solved count for next hint." << std::endl;
    }

    if (m_scoreValueText) m_scoreValueText->setString(std::to_string(m_currentScore));

    if (m_hintPointsText) {
        m_hintPointsText->setString("Points: " + std::to_string(m_hintPoints));
    }
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
void Game::m_updateLayout(sf::Vector2u windowSize) {

    // 1. Calculate Global UI Scale
    m_uiScale = std::min(static_cast<float>(windowSize.x) / static_cast<float>(REF_W),
        static_cast<float>(windowSize.y) / static_cast<float>(REF_H));
    m_uiScale = std::clamp(m_uiScale, 0.65f, 1.6f);

    // 2. Define Design Space References
    const float designW = static_cast<float>(REF_W);
    const float designH = static_cast<float>(REF_H);
    // const sf::Vector2f designCenter = sf::Vector2f(designW / 2.f, designH / 2.f); // Defined in original but not used in direct copy

    bool sizeChanged = (windowSize != m_lastLayoutSize);
    if (sizeChanged) {
        std::cout << "--- Layout Update (" << windowSize.x << "x" << windowSize.y << ") ---" << std::endl;
    }

    // --- Update Main Background Sprite ---
    if (m_mainBackgroundSpr && m_mainBackgroundTex.getSize().x > 0) {
        m_mainBackgroundSpr->setPosition(sf::Vector2f(0.f, 0.f));
        float texOriginalWidth = static_cast<float>(m_mainBackgroundTex.getSize().x);
        float texOriginalHeight = static_cast<float>(m_mainBackgroundTex.getSize().y);
        float scaleX = designW / texOriginalWidth;
        float scaleY = designH / texOriginalHeight;
        float designAspect = designW / designH;
        float bgAspect = texOriginalWidth / texOriginalHeight;
        if (bgAspect > designAspect) {
            m_mainBackgroundSpr->setScale(sf::Vector2f(scaleY, scaleY));
            m_mainBackgroundSpr->setPosition(sf::Vector2f((designW - texOriginalWidth * scaleY) / 2.f, 0.f));
        }
        else {
            m_mainBackgroundSpr->setScale(sf::Vector2f(scaleX, scaleX));
            m_mainBackgroundSpr->setPosition(sf::Vector2f(0.f, (designH - texOriginalHeight * scaleX) / 2.f));
        }
    }

    // 3. Position Top Elements (Score Bar, Progress Meter, Return Button)
    // --- Score Zone Elements ---
    if (m_scoreLabelText && m_scoreValueText) {
        const float zoneX_score = SCORE_ZONE_RECT_DESIGN.position.x;
        const float zoneY_score = SCORE_ZONE_RECT_DESIGN.position.y;
        const float zoneWidth_score = SCORE_ZONE_RECT_DESIGN.size.x;
        const float scaledPaddingY_score = S(this, SCORE_ZONE_PADDING_Y_DESIGN);
        const float scaledLabelValueGap_score = S(this, SCORE_LABEL_VALUE_GAP_DESIGN);

        m_scoreLabelText->setFont(m_font);
        m_scoreLabelText->setString("SCORE:");
        m_scoreLabelText->setCharacterSize(static_cast<unsigned int>(S(this, SCORE_ZONE_LABEL_FONT_SIZE)));
        m_scoreLabelText->setFillColor(GLOWING_TUBE_TEXT_COLOR);
        sf::FloatRect labelBounds_score = m_scoreLabelText->getLocalBounds();
        m_scoreLabelText->setOrigin(sf::Vector2f(labelBounds_score.position.x + labelBounds_score.size.x / 2.f,
            labelBounds_score.position.y));
        m_scoreLabelText->setPosition(sf::Vector2f(zoneX_score + zoneWidth_score / 2.f,
            zoneY_score + scaledPaddingY_score));

        m_scoreValueText->setFont(m_font);
        m_scoreValueText->setCharacterSize(static_cast<unsigned int>(S(this, SCORE_ZONE_VALUE_FONT_SIZE)));
        m_scoreValueText->setFillColor(GLOWING_TUBE_TEXT_COLOR);
        // String is set in render, so getLocalBounds for origin might be tricky here if string changes width a lot.
        // For now, assume it's handled by re-centering in render or that this initial origin is okay.
        sf::FloatRect valueBounds_score = m_scoreValueText->getLocalBounds();
        m_scoreValueText->setOrigin(sf::Vector2f(valueBounds_score.position.x + valueBounds_score.size.x / 2.f,
            valueBounds_score.position.y));
        m_scoreValueText->setPosition(sf::Vector2f(zoneX_score + zoneWidth_score / 2.f,
            m_scoreLabelText->getPosition().y +
            (labelBounds_score.position.y + labelBounds_score.size.y) -
            m_scoreLabelText->getOrigin().y +
            scaledLabelValueGap_score));
    }
    // --- Progress Meter ---
    if (m_progressMeterBg.getPointCount() > 0 && m_progressMeterFill.getPointCount() > 0) {
        const float meterWidth_prog = S(this, TOP_BAR_ZONE_DESIGN.size.x * 0.4f);
        const float meterHeight_prog = S(this, PROGRESS_METER_HEIGHT_DESIGN);
        m_progressMeterBg.setSize(sf::Vector2f(meterWidth_prog, meterHeight_prog));
        m_progressMeterBg.setOrigin(sf::Vector2f(meterWidth_prog / 2.f, meterHeight_prog / 2.f));
        m_progressMeterBg.setPosition(sf::Vector2f(
            TOP_BAR_ZONE_DESIGN.position.x + TOP_BAR_ZONE_DESIGN.size.x / 2.f,
            TOP_BAR_ZONE_DESIGN.position.y + TOP_BAR_ZONE_DESIGN.size.y / 2.f
        ));
        m_progressMeterFill.setSize(sf::Vector2f(0.f, meterHeight_prog));
        m_progressMeterFill.setOrigin(sf::Vector2f(0.f, meterHeight_prog / 2.f));
        m_progressMeterFill.setPosition(sf::Vector2f(
            m_progressMeterBg.getPosition().x - meterWidth_prog / 2.f,
            m_progressMeterBg.getPosition().y
        ));
        if (m_progressMeterText) { // Text positioning is usually in render to update string
            m_progressMeterText->setCharacterSize(static_cast<unsigned int>(S(this, 16.f))); // Example size
            // Centering logic would be similar to other texts, typically after string is set
        }
    }
    // --- Return to Menu Button ---
    if (m_returnToMenuButtonShape.getPointCount() > 0 && m_returnToMenuButtonText) {
        const float btnWidth_return = S(this, RETURN_BTN_WIDTH_DESIGN);
        const float btnHeight_return = S(this, RETURN_BTN_HEIGHT_DESIGN);
        m_returnToMenuButtonShape.setSize(sf::Vector2f(btnWidth_return, btnHeight_return));
        m_returnToMenuButtonShape.setRadius(S(this, 8.f));
        m_returnToMenuButtonShape.setOrigin(sf::Vector2f(0.f, 0.f));
        m_returnToMenuButtonShape.setPosition(sf::Vector2f(
            TOP_BAR_ZONE_DESIGN.position.x + S(this, TOP_BAR_PADDING_X_DESIGN),
            TOP_BAR_ZONE_DESIGN.position.y + (TOP_BAR_ZONE_DESIGN.size.y - btnHeight_return) / 2.f
        ));
        m_returnToMenuButtonText->setCharacterSize(static_cast<unsigned int>(S(this, RETURN_BTN_FONT_SIZE_DESIGN)));
        centerTextOnShape_General(*m_returnToMenuButtonText, m_returnToMenuButtonShape);
    }

    // --- 4. Calculate Grid Layout ---
    const float zoneInnerX_grid = GRID_ZONE_RECT_DESIGN.position.x + GRID_ZONE_PADDING_X_DESIGN;
    const float zoneInnerY_grid = GRID_ZONE_RECT_DESIGN.position.y + GRID_ZONE_PADDING_Y_DESIGN;
    const float zoneInnerWidth_grid = GRID_ZONE_RECT_DESIGN.size.x - 2 * GRID_ZONE_PADDING_X_DESIGN;
    const float zoneInnerHeight_grid = GRID_ZONE_RECT_DESIGN.size.y - 2 * GRID_ZONE_PADDING_Y_DESIGN;
    float actualGridFinalHeight = 0.f; // To be calculated

    m_gridStartY = zoneInnerY_grid; // Initial value, might be adjusted by vertical centering
    int numCols = 1;
    int maxRowsPerCol = m_sorted.empty() ? 1 : static_cast<int>(m_sorted.size());
    m_wordCol.clear(); m_wordRow.clear(); m_colMaxLen.clear(); m_colXOffset.clear();
    float gridElementsScaleFactor = 1.0f;

    if (!m_sorted.empty()) {
        const float st_base_design = TILE_SIZE;
        const float sp_base_design = TILE_PAD;
        const float sc_base_design = COL_PAD;
        const float stph_base_design = st_base_design + sp_base_design;
        const float stpw_base_design = st_base_design + sp_base_design;

        int narrowestOverallCols = 1; int narrowestOverallRows = static_cast<int>(m_sorted.size()); float minWidthOverall = std::numeric_limits<float>::max();
        int maxPossibleCols = std::min(8, std::max(1, static_cast<int>(m_sorted.size())));

        for (int tryCols = 1; tryCols <= maxPossibleCols; ++tryCols) {
            int rowsNeeded = (static_cast<int>(m_sorted.size()) + tryCols - 1) / tryCols; if (rowsNeeded <= 0) rowsNeeded = 1;
            std::vector<int> currentTryColMaxLen(tryCols, 0); float currentTryWidthUnscaled = 0;
            for (size_t w = 0; w < m_sorted.size(); ++w) { int c = static_cast<int>(w) / rowsNeeded; if (c >= 0 && c < tryCols) { currentTryColMaxLen[c] = std::max<int>(currentTryColMaxLen[c], static_cast<int>(m_sorted[w].text.length())); } }
            for (int len : currentTryColMaxLen) { currentTryWidthUnscaled += static_cast<float>(len) * stpw_base_design - (len > 0 ? sp_base_design : 0.f); }
            currentTryWidthUnscaled += static_cast<float>(std::max(0, tryCols - 1)) * sc_base_design; if (currentTryWidthUnscaled < 0) currentTryWidthUnscaled = 0;
            if (currentTryWidthUnscaled < minWidthOverall) { minWidthOverall = currentTryWidthUnscaled; narrowestOverallCols = tryCols; narrowestOverallRows = rowsNeeded; }
        }
        int chosenCols = narrowestOverallCols;
        int chosenRows = narrowestOverallRows;
        const int MAX_ROWS_LIMIT_GRID = 5; // Using unique name from original
        if (chosenRows > MAX_ROWS_LIMIT_GRID) {
            chosenRows = MAX_ROWS_LIMIT_GRID;
            chosenCols = (static_cast<int>(m_sorted.size()) + chosenRows - 1) / chosenRows;
            if (chosenCols <= 0) chosenCols = 1;
        }
        numCols = chosenCols;
        maxRowsPerCol = chosenRows;

        m_colMaxLen.assign(numCols, 0);
        for (size_t w = 0; w < m_sorted.size(); ++w) {
            int c = static_cast<int>(w) / maxRowsPerCol; if (c >= numCols) c = numCols - 1;
            if (c >= 0 && static_cast<size_t>(c) < m_colMaxLen.size()) { m_colMaxLen[c] = std::max<int>(m_colMaxLen[c], static_cast<int>(m_sorted[w].text.length())); }
        }

        float totalRequiredWidthUnscaled = 0;
        for (int c = 0; c < numCols; ++c) {
            int len = (c >= 0 && static_cast<size_t>(c) < m_colMaxLen.size()) ? m_colMaxLen[c] : 0;
            float colWidth_unscaled = static_cast<float>(len) * stpw_base_design - (len > 0 ? sp_base_design : 0.f);
            if (colWidth_unscaled < 0) colWidth_unscaled = 0;
            totalRequiredWidthUnscaled += colWidth_unscaled;
        }
        totalRequiredWidthUnscaled += static_cast<float>(std::max(0, numCols - 1)) * sc_base_design;
        if (totalRequiredWidthUnscaled < 0) totalRequiredWidthUnscaled = 0;

        float totalRequiredHeightUnscaled = static_cast<float>(maxRowsPerCol) * stph_base_design - (maxRowsPerCol > 0 ? sp_base_design : 0.f);
        if (totalRequiredHeightUnscaled < 0) totalRequiredHeightUnscaled = 0;

        float scaleToFitX = 1.0f;
        if (totalRequiredWidthUnscaled > zoneInnerWidth_grid && totalRequiredWidthUnscaled > 0) {
            scaleToFitX = zoneInnerWidth_grid / totalRequiredWidthUnscaled;
        }
        float scaleToFitY = 1.0f;
        if (totalRequiredHeightUnscaled > zoneInnerHeight_grid && totalRequiredHeightUnscaled > 0) {
            scaleToFitY = zoneInnerHeight_grid / totalRequiredHeightUnscaled;
        }
        gridElementsScaleFactor = std::min(scaleToFitX, scaleToFitY);

        if (sizeChanged) {
            std::cout << "  GRID ZONE: Inner W=" << zoneInnerWidth_grid << ", Inner H=" << zoneInnerHeight_grid << std::endl;
            // ... other grid logging
        }

        const float st_final = TILE_SIZE * gridElementsScaleFactor;
        const float sp_final = TILE_PAD * gridElementsScaleFactor;
        const float sc_final = COL_PAD * gridElementsScaleFactor;
        const float stph_final = st_final + sp_final;
        const float stpw_final = st_final + sp_final;

        float currentX_final_scaled = 0;
        m_colXOffset.resize(numCols);
        for (int c = 0; c < numCols; ++c) {
            m_colXOffset[c] = currentX_final_scaled;
            int len = (c >= 0 && static_cast<size_t>(c) < m_colMaxLen.size()) ? m_colMaxLen[c] : 0;
            float colWidth_final_scaled = static_cast<float>(len) * stpw_final - (len > 0 ? sp_final : 0.f);
            if (colWidth_final_scaled < 0) colWidth_final_scaled = 0;
            currentX_final_scaled += colWidth_final_scaled + sc_final;
        }
        m_totalGridW = currentX_final_scaled - (numCols > 0 ? sc_final : 0.f);
        if (m_totalGridW < 0) m_totalGridW = 0;

        actualGridFinalHeight = static_cast<float>(maxRowsPerCol) * stph_final - (maxRowsPerCol > 0 ? sp_final : 0.f);
        if (actualGridFinalHeight < 0) actualGridFinalHeight = 0;

        m_gridStartX = zoneInnerX_grid + (zoneInnerWidth_grid - m_totalGridW) / 2.f;
        m_gridStartY = zoneInnerY_grid + (zoneInnerHeight_grid - actualGridFinalHeight) / 2.f;

        for (int c = 0; c < numCols; ++c) {
            if (static_cast<size_t>(c) < m_colXOffset.size()) { m_colXOffset[c] += m_gridStartX; }
        }

        m_wordCol.resize(m_sorted.size()); m_wordRow.resize(m_sorted.size());
        for (size_t w = 0; w < m_sorted.size(); ++w) { int c = static_cast<int>(w) / maxRowsPerCol; int r = static_cast<int>(w) % maxRowsPerCol; if (c >= numCols) c = numCols - 1; m_wordCol[w] = c; m_wordRow[w] = r; }
        m_currentGridLayoutScale = gridElementsScaleFactor;
        // actualGridFinalHeight = static_cast<float>(maxRowsPerCol) * stph_final - (maxRowsPerCol > 0 ? sp_final : 0.f); // Recalculate just in case, though should be same
    }
    else {
        m_gridStartX = zoneInnerX_grid + zoneInnerWidth_grid / 2.f;
        m_gridStartY = zoneInnerY_grid + zoneInnerHeight_grid / 2.f;
        m_totalGridW = 0; actualGridFinalHeight = 0;
        m_currentGridLayoutScale = 1.0f;
    }
    float calculatedGridActualBottomY = m_gridStartY + actualGridFinalHeight;


    // 5. Determine Final Wheel Size & Position
    const float wheelZoneInnerX_val = WHEEL_ZONE_RECT_DESIGN.position.x + WHEEL_ZONE_PADDING_DESIGN;
    const float wheelZoneInnerY_val = WHEEL_ZONE_RECT_DESIGN.position.y + WHEEL_ZONE_PADDING_DESIGN;
    const float wheelZoneInnerWidth_val = WHEEL_ZONE_RECT_DESIGN.size.x - 2 * WHEEL_ZONE_PADDING_DESIGN;
    const float wheelZoneInnerHeight_val = WHEEL_ZONE_RECT_DESIGN.size.y - 2 * WHEEL_ZONE_PADDING_DESIGN;
    // const float scaledGridWheelGap = S(this, GRID_WHEEL_GAP); // From original, ensure GRID_WHEEL_GAP is float
    // float gridAreaLimitY = calculatedGridActualBottomY + scaledGridWheelGap; // From original

    if (sizeChanged) {
        std::cout << "  WHEEL ZONE: Inner X=" << wheelZoneInnerX_val << ", Y=" << wheelZoneInnerY_val
            << ", W=" << wheelZoneInnerWidth_val << ", H=" << wheelZoneInnerHeight_val << std::endl;
    }

    m_wheelX = wheelZoneInnerX_val + wheelZoneInnerWidth_val / 2.f;
    m_wheelY = wheelZoneInnerY_val + wheelZoneInnerHeight_val / 2.f;
    float maxRadiusForZone = std::min(wheelZoneInnerWidth_val / 2.f, wheelZoneInnerHeight_val / 2.f);
    m_currentWheelRadius = std::min(maxRadiusForZone, WHEEL_R); // WHEEL_R from Constants.h
    float minSensibleRadius = LETTER_R * 1.5f; // LETTER_R from Constants.h
    m_currentWheelRadius = std::max(m_currentWheelRadius, minSensibleRadius);

    if (sizeChanged) {
        std::cout << "  WHEEL FINAL: ZoneMaxR=" << maxRadiusForZone << ", DesignR=" << WHEEL_R
            << ", Clamped CurrentR=" << m_currentWheelRadius << std::endl;
        std::cout << "  WHEEL FINAL: Center X=" << m_wheelX << ", Y=" << m_wheelY << std::endl;
    }

    // 6. Calculate Final Wheel Letter Positions & Visual Background Radius
    if (!m_base.empty()) {
        float radiusBasedOnCount = (PI * m_currentWheelRadius) / static_cast<float>(m_base.size());
        radiusBasedOnCount *= 0.75f;
        float scaleFactorFromDesign = 1.0f;
        if (WHEEL_R > 0.1f) { // WHEEL_R from Constants
            scaleFactorFromDesign = m_currentWheelRadius / WHEEL_R;
        }
        float radiusBasedOnOverallScale = LETTER_R_BASE_DESIGN * scaleFactorFromDesign; // LETTER_R_BASE_DESIGN from Constants
        m_currentLetterRenderRadius = std::min(radiusBasedOnCount, radiusBasedOnOverallScale);
        float minAbsRadius = m_currentWheelRadius * MIN_LETTER_RADIUS_FACTOR; // Constants
        float maxAbsRadius = m_currentWheelRadius * MAX_LETTER_RADIUS_FACTOR; // Constants
        m_currentLetterRenderRadius = std::clamp(m_currentLetterRenderRadius, minAbsRadius, maxAbsRadius);
        m_currentLetterRenderRadius = std::max(m_currentLetterRenderRadius, 5.f); // Min practical size
    }
    else {
        m_currentLetterRenderRadius = LETTER_R_BASE_DESIGN; // Constant
    }
    m_letterPositionRadius = m_currentWheelRadius; // Start with this (from original logic)
    m_letterPositionRadius = std::max(m_letterPositionRadius, m_currentLetterRenderRadius * 0.5f);
    m_letterPositionRadius = std::max(m_letterPositionRadius, m_currentWheelRadius * 0.3f);

    float letterSizeRatioForPadding = (LETTER_R_BASE_DESIGN > 0.01f) ? (m_currentLetterRenderRadius / LETTER_R_BASE_DESIGN) : 1.0f;
    m_visualBgRadius = this->m_letterPositionRadius + m_currentLetterRenderRadius +
        (WHEEL_BG_PADDING_AROUND_LETTERS_DESIGN * letterSizeRatioForPadding); // Constant

    m_wheelLetterRenderPos.resize(m_base.size());
    if (!m_base.empty()) {
        float angleStep = (2.f * PI) / static_cast<float>(m_base.size());
        for (size_t i = 0; i < m_base.size(); ++i) {
            float ang = static_cast<float>(i) * angleStep - PI / 2.f; // Start at top
            m_wheelLetterRenderPos[i] = sf::Vector2f(
                m_wheelX + this->m_letterPositionRadius * std::cos(ang),
                m_wheelY + this->m_letterPositionRadius * std::sin(ang)
            );
        }
    }
    if (sizeChanged || true) { // Original had 'true' for constant logging here
        std::cout << "  WHEEL PATH: m_currentWheelRadius = " << m_currentWheelRadius << std::endl;
        // ... other wheel logging from original
    }

    // 7. Other UI Element Positions (Scramble, Continue, Guess Display)
    if (m_scrambleSpr && m_scrambleTex.getSize().y > 0) {
        // SCRAMBLE_BTN_HEIGHT is the desired height in DESIGN units (e.g., from Constants.h)
        float desired_design_height = static_cast<float>(SCRAMBLE_BTN_HEIGHT); // Use the raw design height

        float texHeight_scramble = static_cast<float>(m_scrambleTex.getSize().y);
        if (texHeight_scramble > 0.1f) {
            float s_scramble_scale = desired_design_height / texHeight_scramble;
            m_scrambleSpr->setScale(sf::Vector2f(s_scramble_scale, s_scramble_scale));
        }
        m_scrambleSpr->setOrigin(sf::Vector2f(0.f, texHeight_scramble / 2.f)); // Origin based on texture

        // Your new positioning code (as you modified it):
        m_scrambleSpr->setPosition(sf::Vector2f(
            m_wheelX + SCRAMBLE_BTN_OFFSET_X, 
            m_wheelY + SCRAMBLE_BTN_OFFSET_Y  
        ));
    }

    if (m_contTxt && m_contBtn.getPointCount() > 0) {
        sf::Vector2f s_size_contBtn = sf::Vector2f(S(this, 200.f), S(this, 50.f)); // Renamed local
        m_contBtn.setSize(s_size_contBtn);
        m_contBtn.setRadius(S(this, 10.f));
        m_contBtn.setOrigin(sf::Vector2f(s_size_contBtn.x / 2.f, 0.f));
        m_contBtn.setPosition(sf::Vector2f(m_wheelX,
            m_wheelY + m_visualBgRadius + S(this, CONTINUE_BTN_OFFSET_Y))); // Constant
        const unsigned int bf_cont_font_val = 24; // Renamed local
        unsigned int sf_cont_font_val = static_cast<unsigned int>(std::max(10.0f, S(this, static_cast<float>(bf_cont_font_val))));
        m_contTxt->setCharacterSize(sf_cont_font_val);
        sf::FloatRect tb_cont_textbounds = m_contTxt->getLocalBounds(); // Renamed local
        m_contTxt->setOrigin(sf::Vector2f(tb_cont_textbounds.position.x + tb_cont_textbounds.size.x / 2.f,
            tb_cont_textbounds.position.y + tb_cont_textbounds.size.y / 2.f));
        m_contTxt->setPosition(m_contBtn.getPosition() + sf::Vector2f(0.f, s_size_contBtn.y / 2.f));
    }

    if (m_guessDisplay_Text) {
        const unsigned int bf_guess_font_val = 30; // Renamed local
        unsigned int sf_guess_font_val = static_cast<unsigned int>(std::max(10.0f, S(this, static_cast<float>(bf_guess_font_val))));
        m_guessDisplay_Text->setCharacterSize(sf_guess_font_val);
    }
    if (m_guessDisplay_Bg.getPointCount() > 0) {
        m_guessDisplay_Bg.setRadius(S(this, 8.f));
        m_guessDisplay_Bg.setOutlineThickness(S(this, 1.f));
    }
    // HUD Logging from original
    const float designBottomEdge_val = designH; // Local copy
    const float scaledHudOffsetY_val = S(this, HUD_TEXT_OFFSET_Y);
    float calculatedHudStartY_val = m_wheelY + m_visualBgRadius + scaledHudOffsetY_val;
    float visualWheelTopEdgeY_val = m_wheelY - m_visualBgRadius;
    if (sizeChanged) {
        std::cout << "  WHEEL/HUD INFO (updateLayout): Visual Wheel BG Top Edge Y = " << visualWheelTopEdgeY_val << std::endl;
        if (actualGridFinalHeight > 0 && visualWheelTopEdgeY_val < calculatedGridActualBottomY - 0.1f) {
            std::cout << "  WHEEL/HUD WARNING (updateLayout): Visual Wheel BG (Y=" << visualWheelTopEdgeY_val
                << ") overlaps Grid Bottom (Y=" << calculatedGridActualBottomY << ")!" << std::endl;
        }
        if (calculatedHudStartY_val > designBottomEdge_val + 0.1f) {
            std::cout << "  WHEEL/HUD WARNING (updateLayout): Calculated HUD Start Y (" << calculatedHudStartY_val
                << ") is below Design Bottom Edge (" << designBottomEdge_val << ")" << std::endl;
        }
    }

    // 8. Menu Layouts (Copied from original, with SFML3 API changes for Rect and setOrigin/setPosition)
    sf::Vector2f windowCenterPix_menu = sf::Vector2f(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)) / 2.f;
    sf::Vector2f mappedWindowCenter_menu = m_window.mapPixelToCoords(sf::Vector2i(static_cast<int>(windowCenterPix_menu.x), static_cast<int>(windowCenterPix_menu.y)));
    const float scaledMenuPadding_menu = S(this, 40.f);
    const float scaledButtonSpacing_menu = S(this, 20.f);
    const unsigned int scaledTitleSize_menu = static_cast<unsigned int>(std::max(12.0f, S(this, 36.f)));
    const unsigned int scaledButtonFontSize_menu = static_cast<unsigned int>(std::max(10.0f, S(this, 24.f)));
    const sf::Vector2f scaledButtonSize_menu_vec = sf::Vector2f(S(this, 250.f), S(this, 50.f));
    const float scaledButtonRadius_menu_val = S(this, 10.f);
    const float scaledMenuRadius_menu_val = S(this, 15.f);
    auto centerTextOnButton_lambda_menu = [&](const std::unique_ptr<sf::Text>& textPtr, const RoundedRectangleShape& button) {
        if (!textPtr) return; sf::Text* text = textPtr.get(); sf::FloatRect tb_lambda_menu = text->getLocalBounds();
        text->setOrigin(sf::Vector2f(tb_lambda_menu.position.x + tb_lambda_menu.size.x / 2.f, tb_lambda_menu.position.y + tb_lambda_menu.size.y / 2.f));
        text->setPosition(button.getPosition() + sf::Vector2f(0.f, button.getSize().y / 2.f));
        };
    if (m_mainMenuTitle && m_casualButtonShape.getPointCount() > 0) {
        m_mainMenuTitle->setCharacterSize(scaledTitleSize_menu);
        m_casualButtonText->setCharacterSize(scaledButtonFontSize_menu); m_competitiveButtonText->setCharacterSize(scaledButtonFontSize_menu); m_quitButtonText->setCharacterSize(scaledButtonFontSize_menu);
        m_casualButtonShape.setSize(scaledButtonSize_menu_vec); m_competitiveButtonShape.setSize(scaledButtonSize_menu_vec); m_quitButtonShape.setSize(scaledButtonSize_menu_vec);
        m_casualButtonShape.setRadius(scaledButtonRadius_menu_val); m_competitiveButtonShape.setRadius(scaledButtonRadius_menu_val); m_quitButtonShape.setRadius(scaledButtonRadius_menu_val);
        sf::FloatRect titleBounds_main_menu = m_mainMenuTitle->getLocalBounds();
        float sths_main_menu = titleBounds_main_menu.size.y + titleBounds_main_menu.position.y + scaledButtonSpacing_menu;
        float tbh_main_menu = 3 * scaledButtonSize_menu_vec.y + 2 * scaledButtonSpacing_menu;
        float smmh_main_menu = scaledMenuPadding_menu + sths_main_menu + tbh_main_menu + scaledMenuPadding_menu;
        float smmw_main_menu = std::max(scaledButtonSize_menu_vec.x, titleBounds_main_menu.size.x + titleBounds_main_menu.position.x) + 2 * scaledMenuPadding_menu;
        m_mainMenuBg.setSize(sf::Vector2f(smmw_main_menu, smmh_main_menu)); m_mainMenuBg.setRadius(scaledMenuRadius_menu_val);
        m_mainMenuBg.setOrigin(sf::Vector2f(smmw_main_menu / 2.f, smmh_main_menu / 2.f)); m_mainMenuBg.setPosition(mappedWindowCenter_menu);
        sf::Vector2f mbp_main_menu_pos = m_mainMenuBg.getPosition(); float mty_main_menu_pos = mbp_main_menu_pos.y - smmh_main_menu / 2.f;
        m_mainMenuTitle->setOrigin(sf::Vector2f(titleBounds_main_menu.position.x + titleBounds_main_menu.size.x / 2.f, titleBounds_main_menu.position.y));
        m_mainMenuTitle->setPosition(sf::Vector2f(mbp_main_menu_pos.x, mty_main_menu_pos + scaledMenuPadding_menu));
        float currentY_main_menu = mty_main_menu_pos + scaledMenuPadding_menu + sths_main_menu;
        m_casualButtonShape.setOrigin(sf::Vector2f(scaledButtonSize_menu_vec.x / 2.f, 0.f)); m_casualButtonShape.setPosition(sf::Vector2f(mbp_main_menu_pos.x, currentY_main_menu));
        centerTextOnButton_lambda_menu(m_casualButtonText, m_casualButtonShape); currentY_main_menu += scaledButtonSize_menu_vec.y + scaledButtonSpacing_menu;
        m_competitiveButtonShape.setOrigin(sf::Vector2f(scaledButtonSize_menu_vec.x / 2.f, 0.f)); m_competitiveButtonShape.setPosition(sf::Vector2f(mbp_main_menu_pos.x, currentY_main_menu));
        centerTextOnButton_lambda_menu(m_competitiveButtonText, m_competitiveButtonShape); currentY_main_menu += scaledButtonSize_menu_vec.y + scaledButtonSpacing_menu;
        m_quitButtonShape.setOrigin(sf::Vector2f(scaledButtonSize_menu_vec.x / 2.f, 0.f)); m_quitButtonShape.setPosition(sf::Vector2f(mbp_main_menu_pos.x, currentY_main_menu));
        centerTextOnButton_lambda_menu(m_quitButtonText, m_quitButtonShape);
    }
    if (m_casualMenuTitle && m_easyButtonShape.getPointCount() > 0) {
        m_casualMenuTitle->setCharacterSize(scaledTitleSize_menu);
        m_easyButtonText->setCharacterSize(scaledButtonFontSize_menu); m_mediumButtonText->setCharacterSize(scaledButtonFontSize_menu); m_hardButtonText->setCharacterSize(scaledButtonFontSize_menu); m_returnButtonText->setCharacterSize(scaledButtonFontSize_menu);
        m_easyButtonShape.setSize(scaledButtonSize_menu_vec); m_mediumButtonShape.setSize(scaledButtonSize_menu_vec); m_hardButtonShape.setSize(scaledButtonSize_menu_vec); m_returnButtonShape.setSize(scaledButtonSize_menu_vec);
        m_easyButtonShape.setRadius(scaledButtonRadius_menu_val); m_mediumButtonShape.setRadius(scaledButtonRadius_menu_val); m_hardButtonShape.setRadius(scaledButtonRadius_menu_val); m_returnButtonShape.setRadius(scaledButtonRadius_menu_val);
        sf::FloatRect ctb_casual_menu = m_casualMenuTitle->getLocalBounds();
        float sths_c_casual_menu = ctb_casual_menu.size.y + ctb_casual_menu.position.y + scaledButtonSpacing_menu;
        float tbh_c_casual_menu = 4 * scaledButtonSize_menu_vec.y + 3 * scaledButtonSpacing_menu;
        float scmh_casual_menu = scaledMenuPadding_menu + sths_c_casual_menu + tbh_c_casual_menu + scaledMenuPadding_menu;
        float scmw_casual_menu = std::max(scaledButtonSize_menu_vec.x, ctb_casual_menu.size.x + ctb_casual_menu.position.x) + 2 * scaledMenuPadding_menu;
        m_casualMenuBg.setSize(sf::Vector2f(scmw_casual_menu, scmh_casual_menu)); m_casualMenuBg.setRadius(scaledMenuRadius_menu_val);
        m_casualMenuBg.setOrigin(sf::Vector2f(scmw_casual_menu / 2.f, scmh_casual_menu / 2.f)); m_casualMenuBg.setPosition(mappedWindowCenter_menu);
        sf::Vector2f cmbp_casual_menu_pos = m_casualMenuBg.getPosition(); float cmty_casual_menu_pos = cmbp_casual_menu_pos.y - scmh_casual_menu / 2.f;
        m_casualMenuTitle->setOrigin(sf::Vector2f(ctb_casual_menu.position.x + ctb_casual_menu.size.x / 2.f, ctb_casual_menu.position.y));
        m_casualMenuTitle->setPosition(sf::Vector2f(cmbp_casual_menu_pos.x, cmty_casual_menu_pos + scaledMenuPadding_menu));
        float ccy_casual_menu = cmty_casual_menu_pos + scaledMenuPadding_menu + sths_c_casual_menu;
        m_easyButtonShape.setOrigin(sf::Vector2f(scaledButtonSize_menu_vec.x / 2.f, 0.f)); m_easyButtonShape.setPosition(sf::Vector2f(cmbp_casual_menu_pos.x, ccy_casual_menu));
        centerTextOnButton_lambda_menu(m_easyButtonText, m_easyButtonShape); ccy_casual_menu += scaledButtonSize_menu_vec.y + scaledButtonSpacing_menu;
        m_mediumButtonShape.setOrigin(sf::Vector2f(scaledButtonSize_menu_vec.x / 2.f, 0.f)); m_mediumButtonShape.setPosition(sf::Vector2f(cmbp_casual_menu_pos.x, ccy_casual_menu));
        centerTextOnButton_lambda_menu(m_mediumButtonText, m_mediumButtonShape); ccy_casual_menu += scaledButtonSize_menu_vec.y + scaledButtonSpacing_menu;
        m_hardButtonShape.setOrigin(sf::Vector2f(scaledButtonSize_menu_vec.x / 2.f, 0.f)); m_hardButtonShape.setPosition(sf::Vector2f(cmbp_casual_menu_pos.x, ccy_casual_menu));
        centerTextOnButton_lambda_menu(m_hardButtonText, m_hardButtonShape); ccy_casual_menu += scaledButtonSize_menu_vec.y + scaledButtonSpacing_menu;
        m_returnButtonShape.setOrigin(sf::Vector2f(scaledButtonSize_menu_vec.x / 2.f, 0.f)); m_returnButtonShape.setPosition(sf::Vector2f(cmbp_casual_menu_pos.x, ccy_casual_menu));
        centerTextOnButton_lambda_menu(m_returnButtonText, m_returnButtonShape);
    }

    // --- 9. REVISED Stacked Hint UI Layout ---
    const sf::FloatRect hintZone = HINT_ZONE_RECT_DESIGN; // From Constants.h
    float currentY_for_buttons = hintZone.position.y; // Initialize starting Y for button stack

    // --- Position "Bonus Words: X/Y" Text at the top of Hint Zone ---
    if (m_bonusWordsInHintZoneText) {
        const float bonusTextPaddingTop = 5.f;
        unsigned int bonusTextFontSize = 20.f;

        m_bonusWordsInHintZoneText->setCharacterSize(bonusTextFontSize);
        m_bonusWordsInHintZoneText->setString("Bonus Words: 00/00"); // Representative string for layout
        sf::FloatRect btBounds = m_bonusWordsInHintZoneText->getLocalBounds();

        // Center it horizontally in the hint zone
        float bonusWordsTextX = (hintZone.position.x + (hintZone.size.x - (btBounds.position.x + btBounds.size.x)) / 2.f) + 10.f;
        float bonusWordsTextY = hintZone.position.y + bonusTextPaddingTop;

        // Origin should be consistent with how you handle text (e.g., top-left from its local bounds)
        m_bonusWordsInHintZoneText->setOrigin(sf::Vector2f(btBounds.position.x, btBounds.position.y));
        m_bonusWordsInHintZoneText->setPosition(sf::Vector2f(bonusWordsTextX, bonusWordsTextY));

        // Update starting Y for hint buttons to be below this text
        // Using text's global bounds after positioning and scaling ensures accuracy
        sf::FloatRect positionedBtGlobalBounds = m_bonusWordsInHintZoneText->getGlobalBounds();
        currentY_for_buttons = positionedBtGlobalBounds.position.y + positionedBtGlobalBounds.size.y + S(this, 5.f); // Add some padding
    }

    // --- Layout for Hint Buttons (using currentY_for_buttons) ---
    if (m_hintFrameTexture.getSize().x > 0 && !m_hintFrameSprites.empty()) {
        const float frameTexOriginalWidth = static_cast<float>(m_hintFrameTexture.getSize().x);
        const float frameTexOriginalHeight = static_cast<float>(m_hintFrameTexture.getSize().y);
        const int numHintFrames = 4;
        const float verticalSpacingBetweenFrames = S(this, 2.f);

        float availableWidthForFrame = hintZone.size.x - S(this, 4.f);
        float panelScale = availableWidthForFrame / frameTexOriginalWidth;

        float scaledFrameHeight = frameTexOriginalHeight * panelScale;
        float scaledFrameWidth = frameTexOriginalWidth * panelScale;

        float totalRequiredStackHeight = (numHintFrames * scaledFrameHeight) + ((numHintFrames - 1) * verticalSpacingBetweenFrames);
        float availableVerticalSpaceForButtons = (hintZone.position.y + hintZone.size.y) - currentY_for_buttons - S(this, 5.f);

        if (totalRequiredStackHeight > availableVerticalSpaceForButtons && availableVerticalSpaceForButtons > 0) {
            float newScaledFrameHeight = (availableVerticalSpaceForButtons - ((numHintFrames - 1) * verticalSpacingBetweenFrames)) / numHintFrames;
            if (newScaledFrameHeight < S(this, 20.f)) newScaledFrameHeight = S(this, 20.f);

            panelScale = newScaledFrameHeight / frameTexOriginalHeight;
            scaledFrameWidth = frameTexOriginalWidth * panelScale;
            scaledFrameHeight = newScaledFrameHeight;
        }

        const float lightLensCenterXRel = 0.15f;
        const float lightLensCenterYRel = 0.46f;
        const float lightDiameterRel = 0.55f;
        const float labelStartXRel = 0.33f;
        const float labelCenterYRel = 0.50f;
        const unsigned int labelBaseFontSizeOnFrame = 75;

        std::vector<std::unique_ptr<sf::Text>*> hintLabelTexts = {
            &m_hintRevealFirstButtonText, &m_hintRevealRandomButtonText,
            &m_hintRevealLastButtonText, &m_hintRevealFirstOfEachButtonText
        };

        for (int i = 0; i < numHintFrames; ++i) {
            if (i >= static_cast<int>(m_hintFrameSprites.size()) || !m_hintFrameSprites[i]) continue;

            float frameX = hintZone.position.x + (hintZone.size.x - scaledFrameWidth) / 2.f;
            m_hintFrameSprites[i]->setScale(sf::Vector2f(panelScale, panelScale));
            m_hintFrameSprites[i]->setPosition(sf::Vector2f(frameX, currentY_for_buttons));

            if (i < static_cast<int>(m_hintClickableRegions.size())) {
                // Use sf::Vector2f for position and size for FloatRect constructor
                m_hintClickableRegions[i] = sf::FloatRect({ frameX, currentY_for_buttons }, { scaledFrameWidth, scaledFrameHeight });
            }

            sf::Vector2f frameTopLeftPos = m_hintFrameSprites[i]->getPosition();

            if (i < static_cast<int>(m_hintIndicatorLightSprs.size()) && m_hintIndicatorLightSprs[i]) {
                const sf::Texture* lightTex = &m_hintIndicatorLightSprs[i]->getTexture();
                if (lightTex && lightTex->getSize().x > 0) { // Assuming getTexture() returns a valid pointer
                    float lightOriginalTexSize = static_cast<float>(lightTex->getSize().x);
                    float desiredLightScreenDiameter = frameTexOriginalHeight * lightDiameterRel * panelScale;
                    float lightSpriteScale = desiredLightScreenDiameter / lightOriginalTexSize;

                    m_hintIndicatorLightSprs[i]->setScale(sf::Vector2f(lightSpriteScale, lightSpriteScale));
                    m_hintIndicatorLightSprs[i]->setOrigin(sf::Vector2f(lightOriginalTexSize / 2.f, lightOriginalTexSize / 2.f));

                    float lightScreenCenterX = frameTopLeftPos.x + (frameTexOriginalWidth * lightLensCenterXRel * panelScale);
                    float lightScreenCenterY = frameTopLeftPos.y + (frameTexOriginalHeight * lightLensCenterYRel * panelScale);
                    m_hintIndicatorLightSprs[i]->setPosition(sf::Vector2f(lightScreenCenterX, lightScreenCenterY));
                }
            }

            if (i < static_cast<int>(hintLabelTexts.size()) && hintLabelTexts[i] && hintLabelTexts[i]->get()) {
                sf::Text* label = hintLabelTexts[i]->get();
                unsigned int scaledLabelFontSize = static_cast<unsigned int>(std::max(8.0f, static_cast<float>(labelBaseFontSizeOnFrame) * panelScale));
                label->setCharacterSize(scaledLabelFontSize);
                label->setFillColor(sf::Color(220, 220, 220));

                sf::FloatRect labelBounds = label->getLocalBounds();
                label->setOrigin(sf::Vector2f(labelBounds.position.x, labelBounds.position.y + labelBounds.size.y / 2.f));

                float labelScreenX = frameTopLeftPos.x + (frameTexOriginalWidth * labelStartXRel * panelScale);
                float labelScreenY = frameTopLeftPos.y + (frameTexOriginalHeight * labelCenterYRel * panelScale);
                label->setPosition(sf::Vector2f(labelScreenX, labelScreenY));
            }
            currentY_for_buttons += scaledFrameHeight + verticalSpacingBetweenFrames;
        }
    }

    // --- NEW: Layout for Hint Pop-up ---
    const sf::FloatRect gridZone = GRID_ZONE_RECT_DESIGN; // Target zone for centering

    const float popupPadding = S(this, 10.f);
    // Define base dimensions for the hint detail pop-up. These might need adjustment
    // based on the longest description or if you implement dynamic sizing.
    const float popupDesignWidth = S(this, 240.f);  // Base width in design units
    const float popupDesignHeight = S(this, 120.f); // Base height for 3-4 lines of text

    // Center the pop-up within GRID_ZONE_RECT_DESIGN
    float gridZoneCenterX = gridZone.position.x + gridZone.size.x / 2.f;
    float gridZoneCenterY = gridZone.position.y + gridZone.size.y / 2.f;

    float popupX = gridZoneCenterX - popupDesignWidth / 2.f;
    float popupY = gridZoneCenterY - popupDesignHeight / 2.f;

    // Optional: Clamp to screen edges if necessary, though centering in grid usually handles this.
    // If GRID_ZONE_RECT_DESIGN can be very small, this pop-up might appear too large *for it*,
    // but it will be centered *relative to it*.
    popupX = std::max(popupX, S(this, 5.f)); // Min screen padding from left
    popupY = std::max(popupY, S(this, 5.f)); // Min screen padding from top
    if (popupX + popupDesignWidth > static_cast<float>(REF_W) - S(this, 5.f)) {
        popupX = static_cast<float>(REF_W) - popupDesignWidth - S(this, 5.f);
    }
    if (popupY + popupDesignHeight > static_cast<float>(REF_H) - S(this, 5.f)) {
        popupY = static_cast<float>(REF_H) - popupDesignHeight - S(this, 5.f);
    }


    m_hintPopupBackground.setSize({ popupDesignWidth, popupDesignHeight });
    m_hintPopupBackground.setRadius(S(this, 8.f));
    m_hintPopupBackground.setPosition({ popupX, popupY });
    m_hintPopupBackground.setFillColor(sf::Color(40, 45, 60, 240)); // Darker, more opaque
    m_hintPopupBackground.setOutlineColor(sf::Color(150, 150, 180, 220));
    m_hintPopupBackground.setOutlineThickness(S(this, 1.5f));

    // Position texts within the popup background (top-left aligned within popup padding)
    float textCurrentY_popup = popupY + popupPadding;
    float textStartX_popup = popupX + popupPadding;
    float textMaxWidth_popup = popupDesignWidth - 2 * popupPadding; // For potential text wrapping

    unsigned int popupLineFontSize = static_cast<unsigned int>(S(this, 16.f));
    unsigned int popupDescFontSize = static_cast<unsigned int>(S(this, 14.f));
    float lineSpacing_popup = S(this, 7.f); // Slightly increased spacing for readability

    if (m_popupAvailablePointsText) {
        m_popupAvailablePointsText->setCharacterSize(popupLineFontSize);
        m_popupAvailablePointsText->setString("Available Points: 0000"); // Placeholder for layout
        sf::FloatRect textBounds = m_popupAvailablePointsText->getLocalBounds();
        m_popupAvailablePointsText->setOrigin({ textBounds.position.x, textBounds.position.y });
        m_popupAvailablePointsText->setPosition({ textStartX_popup, textCurrentY_popup });
        textCurrentY_popup += m_popupAvailablePointsText->getGlobalBounds().size.y + lineSpacing_popup;
    }
    if (m_popupHintCostText) {
        m_popupHintCostText->setCharacterSize(popupLineFontSize);
        m_popupHintCostText->setString("Cost: 000"); // Placeholder
        sf::FloatRect textBounds = m_popupHintCostText->getLocalBounds();
        m_popupHintCostText->setOrigin({ textBounds.position.x, textBounds.position.y });
        m_popupHintCostText->setPosition({ textStartX_popup, textCurrentY_popup });
        textCurrentY_popup += m_popupHintCostText->getGlobalBounds().size.y + lineSpacing_popup;
    }
    if (m_popupHintDescriptionText) {
        m_popupHintDescriptionText->setCharacterSize(popupDescFontSize);
        m_popupHintDescriptionText->setString("This is a sample hint description for layout purposes."); // Placeholder
        sf::FloatRect textBounds = m_popupHintDescriptionText->getLocalBounds();
        m_popupHintDescriptionText->setOrigin({ textBounds.position.x, textBounds.position.y });
        m_popupHintDescriptionText->setPosition({ textStartX_popup, textCurrentY_popup });
        // Note: For actual rendering, Game::m_renderGameScreen updates the string content.
        // If descriptions are long, you'll need text wrapping logic here or in render.
        // The current popupDesignHeight might need to be dynamic if descriptions vary greatly.
    }

    // --- Update DEBUG Zone Shapes ---
    if (m_showDebugZones) {
        float scaledOutlineThickness = S(this, 2.0f);

        m_debugGridZoneShape.setPosition(sf::Vector2f(GRID_ZONE_RECT_DESIGN.position.x, GRID_ZONE_RECT_DESIGN.position.y));
        m_debugGridZoneShape.setSize(sf::Vector2f(GRID_ZONE_RECT_DESIGN.size.x, GRID_ZONE_RECT_DESIGN.size.y));
        m_debugGridZoneShape.setOutlineThickness(scaledOutlineThickness);

        m_debugHintZoneShape.setPosition(sf::Vector2f(HINT_ZONE_RECT_DESIGN.position.x, HINT_ZONE_RECT_DESIGN.position.y));
        m_debugHintZoneShape.setSize(sf::Vector2f(HINT_ZONE_RECT_DESIGN.size.x, HINT_ZONE_RECT_DESIGN.size.y));
        m_debugHintZoneShape.setOutlineThickness(scaledOutlineThickness);

        m_debugWheelZoneShape.setPosition(sf::Vector2f(WHEEL_ZONE_RECT_DESIGN.position.x, WHEEL_ZONE_RECT_DESIGN.position.y));
        m_debugWheelZoneShape.setSize(sf::Vector2f(WHEEL_ZONE_RECT_DESIGN.size.x, WHEEL_ZONE_RECT_DESIGN.size.y));
        m_debugWheelZoneShape.setOutlineThickness(scaledOutlineThickness);

        m_debugScoreZoneShape.setPosition(sf::Vector2f(SCORE_ZONE_RECT_DESIGN.position.x, SCORE_ZONE_RECT_DESIGN.position.y));
        m_debugScoreZoneShape.setSize(sf::Vector2f(SCORE_ZONE_RECT_DESIGN.size.x, SCORE_ZONE_RECT_DESIGN.size.y));
        m_debugScoreZoneShape.setOutlineThickness(scaledOutlineThickness);

        m_debugTopBarZoneShape.setPosition(sf::Vector2f(TOP_BAR_ZONE_DESIGN.position.x, TOP_BAR_ZONE_DESIGN.position.y));
        m_debugTopBarZoneShape.setSize(sf::Vector2f(TOP_BAR_ZONE_DESIGN.size.x, TOP_BAR_ZONE_DESIGN.size.y));
        m_debugTopBarZoneShape.setOutlineThickness(scaledOutlineThickness);
    }

    m_lastLayoutSize = windowSize;
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
    if (event.is<sf::Event::Closed>()) { // SFML3: event.is<T>()
        m_window.close();
        return;
    }

    // Using getIf<T>() for specific event types in SFML 3
    if (const auto* rsEv = event.getIf<sf::Event::Resized>()) {
        sf::Vector2u currentSize = rsEv->size; // rsEv->size is sf::Vector2u
        // Your original clamping logic for MIN_WINDOW_WIDTH/HEIGHT would go here if needed
        // For example:
        // if (currentSize.x < MIN_WINDOW_WIDTH) currentSize.x = MIN_WINDOW_WIDTH;
        // if (currentSize.y < MIN_WINDOW_HEIGHT) currentSize.y = MIN_WINDOW_HEIGHT;
        // if (currentSize != rsEv->size) m_window.setSize(currentSize); // If clamped

        m_updateView(currentSize);
        m_updateLayout(currentSize);
        return;
    }

    if (m_gameState != GState::Playing) {
        return;
    }

    if (const auto* mb_pressed = event.getIf<sf::Event::MouseButtonPressed>()) { // Renamed local
        if (mb_pressed->button == sf::Mouse::Button::Left) {
            sf::Vector2f mp = m_window.mapPixelToCoords(mb_pressed->position); // Use mb_pressed->position

            if (m_returnToMenuButtonShape.getGlobalBounds().contains(mp)) {
                if (m_clickSound) m_clickSound->play();
                m_currentScreen = GameScreen::MainMenu;
                m_backgroundMusic.stop();
                m_isInSession = false;
                m_selectedDifficulty = DifficultyLevel::None;
                m_clearDragState(); // Ensure this resets guess, path, dragging
                return;
            }
            if (m_scrambleSpr && m_scrambleSpr->getGlobalBounds().contains(mp)) {
                if (m_clickSound) m_clickSound->play();
                std::shuffle(m_base.begin(), m_base.end(), Rng()); // Ensure Rng() is defined and seeded
                m_clearDragState();
                return;
            }

            bool hintButtonClicked = false;
            const int HINT_COSTS_EVENT_ARR[] = { HINT_COST_REVEAL_FIRST, HINT_COST_REVEAL_RANDOM, HINT_COST_REVEAL_LAST, HINT_COST_REVEAL_FIRST_OF_EACH }; // Renamed
            const HintType HINT_TYPES_EVENT_ARR[] = { HintType::RevealFirst, HintType::RevealRandom, HintType::RevealLast, HintType::RevealFirstOfEach }; // Renamed

            for (int i = 0; i < 4; ++i) {
                if (i < m_hintClickableRegions.size() && m_hintClickableRegions[i].contains(mp)) {

                    if (i < m_hintFrameClickAnimTimers.size()) {
                        m_hintFrameClickAnimTimers[i] = HINT_FRAME_CLICK_DURATION;
                    }

                    if (m_hintPoints >= HINT_COSTS_EVENT_ARR[i]) {
                        std::cout << "DEBUG: Clicked Hint " << (i + 1) << " (Type: " << static_cast<int>(HINT_TYPES_EVENT_ARR[i]) << ")" << std::endl;
                        m_hintPoints -= HINT_COSTS_EVENT_ARR[i];
                        m_activateHint(HINT_TYPES_EVENT_ARR[i]);
                    }
                    else {
                        std::cout << "DEBUG: Clicked Hint " << (i + 1) << ", but cannot afford." << std::endl;
                        if (m_errorWordSound) m_errorWordSound->play();
                    }
                    hintButtonClicked = true;
                    break;
                }
            }
            if (hintButtonClicked) return;

            // Letter Wheel Click (from original)
            for (std::size_t i = 0; i < m_base.size(); ++i) {
                if (i < m_wheelLetterRenderPos.size() && distSq(mp, m_wheelLetterRenderPos[i]) < m_currentLetterRenderRadius * m_currentLetterRenderRadius) {
                    m_dragging = true;
                    m_path.clear();
                    m_path.push_back(static_cast<int>(i));
                    m_currentGuess += static_cast<char>(std::toupper(m_base[i]));
                    if (m_selectSound) m_selectSound->play();
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

                        int baseScore = static_cast<int>(m_currentGuess.length()) * 10;
                        int rarityBonus = (m_sorted[w].rarity > 1) ? (m_sorted[w].rarity * 25) : 0;
                        int wordScoreForThisWord = baseScore + rarityBonus;

                        m_currentScore += wordScoreForThisWord;
                        m_spawnScoreFlourish(wordScoreForThisWord, static_cast<int>(w));

                        if (m_scoreValueText) m_scoreValueText->setString(std::to_string(m_currentScore));

                        // m_wordsSolvedSinceHint++; // This was for gaining free hints, hints are now bought with points.
                        // if (m_wordsSolvedSinceHint >= WORDS_PER_HINT) {
                        //     m_hintsAvailable++; m_wordsSolvedSinceHint = 0;
                        //     if (m_hintCountTxt) m_hintCountTxt->setString("Hints: " + std::to_string(m_hintsAvailable));
                        // }

                        for (std::size_t c = 0; c < m_currentGuess.length(); ++c) {
                            if (c < m_path.size()) {
                                int pathNodeIdx = m_path[c];
                                if (pathNodeIdx >= 0 && static_cast<size_t>(pathNodeIdx) < m_wheelLetterRenderPos.size() &&
                                    static_cast<size_t>(pathNodeIdx) < m_base.size()) {

                                    sf::Vector2f startPos = m_wheelLetterRenderPos[pathNodeIdx];
                                    sf::Vector2f endPos = m_tilePos(wordIndexMatched, static_cast<int>(c));

                                    float finalRenderTileSize = TILE_SIZE * m_currentGridLayoutScale;
                                    endPos.x += finalRenderTileSize / 2.f;
                                    endPos.y += finalRenderTileSize / 2.f;

                                    m_anims.push_back({
                                        m_currentGuess[c],
                                        startPos,
                                        endPos,
                                        0.f - (c * 0.03f),
                                        wordIndexMatched,
                                        static_cast<int>(c),
                                        AnimTarget::Grid
                                        });
                                }
                            }
                        }
                        std::cout << "GRID Word: " << m_currentGuess << " | Rarity: " << m_sorted[wordIndexMatched].rarity << " | Len: " << m_currentGuess.length() << " | Rarity Bonus: " << rarityBonus << " | BasePts: " << baseScore << " | Current Game Score: " << m_currentScore << std::endl;

                        if (m_found.size() == m_solutions.size()) {
                            std::cout << "DEBUG: All grid words found! Puzzle solved." << std::endl;
                            if (m_winSound) m_winSound->play();
                            m_gameState = GState::Solved;
                            m_currentScreen = GameScreen::GameOver;
                            m_updateLayout(m_window.getSize());
                        }
                        actionTaken = true;
                    }
                    goto process_outcome;
                }
            } // --- End Grid Check Loop ---


            // --- Phase 2: Check against BONUS words (only if no grid match occurred) ---
            std::cout << "DEBUG: Checking for BONUS word..." << std::endl;
            for (const auto& potentialWordInfo : m_allPotentialSolutions) {
                const std::string& bonusWordOriginalCase = potentialWordInfo.text;
                if (m_found.count(bonusWordOriginalCase)) { continue; }

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

                        int hintPointsAwarded = 0;
                        size_t len = bonusWordOriginalCase.length();
                        if (len == 3) hintPointsAwarded = 1;
                        else if (len == 4) hintPointsAwarded = 2;
                        else if (len == 5) hintPointsAwarded = 3;
                        else if (len == 6) hintPointsAwarded = 4;
                        else if (len == 7) hintPointsAwarded = 5;

                        if (hintPointsAwarded > 0) {
                            m_hintPoints += hintPointsAwarded;
                            std::cout << "DEBUG: Hint Points increased by " << hintPointsAwarded << ". New Total Hint Points: " << m_hintPoints << std::endl;
                            if (m_hintPointsText) m_hintPointsText->setString("Points: " + std::to_string(m_hintPoints));

                            float bonusTextApproxY = m_wheelY + (m_currentWheelRadius + S(this, 30.f))
                                + S(this, HUD_TEXT_OFFSET_Y) + S(this, 20.f) + S(this, HUD_LINE_SPACING) + S(this, 10.f);
                            sf::FloatRect bonusSummaryBounds = m_bonusWordsInHintZoneText->getGlobalBounds();
                            sf::Vector2f hintAnimStartPos = { bonusSummaryBounds.position.x + bonusSummaryBounds.size.x / 2.f,
                                                              bonusSummaryBounds.position.y + bonusSummaryBounds.size.y / 2.f };
                            m_spawnHintPointAnimation(hintAnimStartPos, hintPointsAwarded);
                        }

                        std::cout << "BONUS Word: " << m_currentGuess << " (Length: " << len << ") | Hint Points Awarded: " << hintPointsAwarded << std::endl;

                        //m_bonusTextFlourishTimer = BONUS_TEXT_FLOURISH_DURATION;
                        if (m_placeSound) m_placeSound->play();
                        actionTaken = true;

                        // ***** NEW: Check for Full Bonus List Completion *****
                        int totalPossibleBonus = m_calculateTotalPossibleBonusWords();
                        if (totalPossibleBonus > 0 && m_foundBonusWords.size() == static_cast<size_t>(totalPossibleBonus)) {
                            std::cout << "DEBUG: *** ENTIRE BONUS LIST COMPLETED! ***" << std::endl;

                            // Calculate points for bonus list completion
                            int rawCompletionValue = 0;
                            // Pn_complete values (points per word length for this specific bonus)
                            const int p3c = 5, p4c = 10, p5c = 20, p6c = 35, p7c = 50;
                            // Iterate through m_cachedBonusWords (which should be populated by now)
                            // Or, more robustly, iterate m_allPotentialSolutions and check if non-grid
                            for (const auto& bWordInfo : m_allPotentialSolutions) {
                                if (!isGridSolution(bWordInfo.text)) { // It's a bonus word
                                    size_t bLen = bWordInfo.text.length();
                                    if (bLen == 3) rawCompletionValue += p3c;
                                    else if (bLen == 4) rawCompletionValue += p4c;
                                    else if (bLen == 5) rawCompletionValue += p5c;
                                    else if (bLen == 6) rawCompletionValue += p6c;
                                    else if (bLen == 7) rawCompletionValue += p7c;
                                }
                            }

                            int flatFullClearBonus = 150; // Example
                            float difficultyMultiplier = 1.0f;
                            if (m_selectedDifficulty == DifficultyLevel::Medium) difficultyMultiplier = 1.25f;
                            else if (m_selectedDifficulty == DifficultyLevel::Hard) difficultyMultiplier = 1.5f;

                            int finalBonusListScore = static_cast<int>((static_cast<float>(rawCompletionValue) + static_cast<float>(flatFullClearBonus)) * difficultyMultiplier);

                            m_triggerBonusListCompleteEffect(finalBonusListScore);
                            // Note: The points are added to m_currentScore inside m_updateBonusListCompleteEffect 
                            // when the animation timer finishes, to sync with the visual effect.
                        }
                        // ***** END NEW CHECK *****
                    }
                    goto process_outcome;
                }
            } // --- End Bonus Check Loop ---


            // --- Phase 3: Incorrect Word ---
            std::cout << "Word '" << m_currentGuess << "' is not valid for this puzzle." << std::endl;
            if (m_errorWordSound) m_errorWordSound->play();
            actionTaken = true;


        process_outcome:;

            m_clearDragState();
        }
    } // --- End Mouse Button Released ---

    // Also, ensure that when a hint is used, the m_hintPointsText is updated immediately:
    if (const auto* mb_pressed_for_hint = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mb_pressed_for_hint->button == sf::Mouse::Button::Left) {
            sf::Vector2f mp = m_window.mapPixelToCoords(mb_pressed_for_hint->position);
            // ... (existing button checks like return to menu, scramble) ...

            bool hintButtonClickedThisPress = false; // Use a local flag for this press
            const int HINT_COSTS_EVENT_ARR_LOCAL[] = { HINT_COST_REVEAL_FIRST, HINT_COST_REVEAL_RANDOM, HINT_COST_REVEAL_LAST, HINT_COST_REVEAL_FIRST_OF_EACH };
            const HintType HINT_TYPES_EVENT_ARR_LOCAL[] = { HintType::RevealFirst, HintType::RevealRandom, HintType::RevealLast, HintType::RevealFirstOfEach };

            for (int i = 0; i < 4; ++i) {
                if (i < m_hintClickableRegions.size() && m_hintClickableRegions[i].contains(mp)) {

                    if (i < m_hintFrameClickAnimTimers.size()) {
                        m_hintFrameClickAnimTimers[i] = HINT_FRAME_CLICK_DURATION;
                    }

                    if (m_hintPoints >= HINT_COSTS_EVENT_ARR_LOCAL[i]) {
                        m_hintPoints -= HINT_COSTS_EVENT_ARR_LOCAL[i];
                        if (m_hintPointsText) m_hintPointsText->setString("Points: " + std::to_string(m_hintPoints)); // Update display
                        m_activateHint(HINT_TYPES_EVENT_ARR_LOCAL[i]);
                        // m_hintUsedSound will be played in m_activateHint if successful
                    }
                    else {
                        if (m_errorWordSound) m_errorWordSound->play();
                    }
                    hintButtonClickedThisPress = true;
                    break;
                }
            }
            if (hintButtonClickedThisPress) return; // If a hint button was processed, return
            // ... (rest of mouse button pressed logic for wheel) ...
        }
    }
    
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

    // --- Scaled values (ensure S function is S(this, value)) ---
    // (These are mostly from your original full code, ensure they are correct for your needs)
    const float scaledWheelOutlineThickness = S(this, 3.f);
    const float scaledLetterCircleOutline = S(this, 2.f);
    const float scaledPathThickness = S(this, 5.0f);
    const float scaledGuessDisplayGap = S(this, GUESS_DISPLAY_GAP);
    const float scaledGuessDisplayPadX = S(this, 15.f);
    const float scaledGuessDisplayPadY = S(this, 5.f);
    const float scaledGuessDisplayRadius = S(this, 8.f);
    const float scaledGuessDisplayOutline = S(this, 1.f);
    const float scaledHudOffsetY = S(this, HUD_TEXT_OFFSET_Y); // Used for old HUD elements below wheel
    // const float scaledHudLineSpacing = S(this, HUD_LINE_SPACING); // Used for old HUD elements

    const unsigned int scaledGridLetterFontSize = static_cast<unsigned int>(std::max(8.0f, S(this, 20.f) * m_currentGridLayoutScale));
    const unsigned int scaledFlyingLetterFontSize = static_cast<unsigned int>(std::max(8.0f, S(this, 20.f)));
    const unsigned int scaledGuessDisplayFontSize = static_cast<unsigned int>(std::max(8.0f, S(this, 30.f)));
    const unsigned int scaledSolvedFontSize = static_cast<unsigned int>(std::max(8.0f, S(this, 26.f)));
    const unsigned int scaledContinueFontSize = static_cast<unsigned int>(std::max(8.0f, S(this, 24.f)));

    // --- Progress Meter (Top Bar) ---
    if (m_isInSession && m_progressMeterBg.getPointCount() > 0 && m_progressMeterFill.getPointCount() > 0 && m_progressMeterText) {
        m_progressMeterBg.setFillColor(sf::Color(50, 50, 50, 150));
        m_progressMeterBg.setOutlineColor(sf::Color(150, 150, 150));
        m_progressMeterBg.setOutlineThickness(S(this, PROGRESS_METER_OUTLINE));
        m_progressMeterFill.setFillColor(sf::Color(0, 180, 0, 200));

        float progressRatio = 0.f;
        if (m_puzzlesPerSession > 0) {
            progressRatio = static_cast<float>(m_currentPuzzleIndex + 1) / static_cast<float>(m_puzzlesPerSession);
        }
        float fillWidth = m_progressMeterBg.getSize().x * progressRatio;
        m_progressMeterFill.setSize(sf::Vector2f(fillWidth, m_progressMeterBg.getSize().y));

        m_window.draw(m_progressMeterBg);
        m_window.draw(m_progressMeterFill);

        const unsigned int scaledProgressFontSize = static_cast<unsigned int>(std::max(8.0f, S(this, 16.f)));
        m_progressMeterText->setCharacterSize(scaledProgressFontSize);
        std::string progressStr = std::to_string(m_currentPuzzleIndex + 1) + "/" + std::to_string(m_puzzlesPerSession);
        m_progressMeterText->setString(progressStr);
        // Recenter text on its shape (m_progressMeterBg)
        centerTextOnShape_General(*m_progressMeterText, m_progressMeterBg); // Use your general centering helper
        m_window.draw(*m_progressMeterText);
    }

    // --- Return to Menu Button (Top Bar) ---
    if ((m_currentScreen == GameScreen::Playing || m_currentScreen == GameScreen::GameOver) && m_returnToMenuButtonShape.getPointCount() > 0 && m_returnToMenuButtonText) {
        bool returnHover = m_returnToMenuButtonShape.getGlobalBounds().contains(mousePos);
        m_returnToMenuButtonShape.setFillColor(returnHover ? m_currentTheme.menuButtonHover : m_currentTheme.menuButtonNormal);
        m_window.draw(m_returnToMenuButtonShape);
        m_returnToMenuButtonText->setFillColor(m_currentTheme.menuButtonText);
        centerTextOnShape_General(*m_returnToMenuButtonText, m_returnToMenuButtonShape);
        m_window.draw(*m_returnToMenuButtonText);
    }

    // --- Score Zone Elements (Right Side) ---
    if (m_scoreLabelText) { m_window.draw(*m_scoreLabelText); }
    if (m_scoreValueText) {
        m_scoreValueText->setString(std::to_string(m_currentScore));
        // Ensure origin is correct for flourish (center of text)
        sf::FloatRect valBounds = m_scoreValueText->getLocalBounds();
        m_scoreValueText->setOrigin({ valBounds.position.x + valBounds.size.x  / 2.f , valBounds.position.y + valBounds.size.y / 2.f});
        // Keep original position, adjust origin for scaling effect
        sf::Vector2f valOriginalPos = m_scoreValueText->getPosition(); // This was set in m_updateLayout
        sf::Vector2f valOriginalScale = m_scoreValueText->getScale(); // Should be {1,1} unless already scaled

        if (m_scoreFlourishTimer > 0.f) {
            float scaleFactor = 1.0f + SCORE_FLOURISH_SCALE * std::sin((SCORE_FLOURISH_DURATION - m_scoreFlourishTimer) / SCORE_FLOURISH_DURATION * PI);
            m_scoreValueText->setScale(sf::Vector2f(scaleFactor, scaleFactor));
        }
        m_window.draw(*m_scoreValueText);
        if (m_scoreFlourishTimer > 0.f) { // Restore scale
            m_scoreValueText->setScale(valOriginalScale);
        }
    }
    // "Bonus Words: X/Y" text is NO LONGER drawn in the Score Zone.

    // --- Draw Letter Grid ---
    if (!m_sorted.empty() && !m_grid.empty()) {
        const float finalRenderTileSize = TILE_SIZE * m_currentGridLayoutScale;
        const float finalRenderTileRadius = finalRenderTileSize * 0.18f; // Example radius for RoundedRect
        const float tileOutlineRenderThickness = S(this, 1.0f); // Scaled outline

        RoundedRectangleShape tileBackground(sf::Vector2f(finalRenderTileSize, finalRenderTileSize), finalRenderTileRadius, 10);
        tileBackground.setOutlineThickness(tileOutlineRenderThickness);
        sf::Text letterText_grid(m_font, "", scaledGridLetterFontSize);

        for (std::size_t w = 0; w < m_sorted.size(); ++w) {
            if (w >= m_grid.size()) continue; // Safety check
            int wordRarity = m_sorted[w].rarity;
            for (std::size_t c = 0; c < m_sorted[w].text.length(); ++c) {
                if (c >= m_grid[w].size()) continue; // Safety check

                sf::Vector2f p_tile = m_tilePos(static_cast<int>(w), static_cast<int>(c));
                bool isFilled = (m_grid[w][c] != '_');
                tileBackground.setPosition(p_tile);
                tileBackground.setFillColor(isFilled ? m_currentTheme.gridFilledTile : m_currentTheme.gridEmptyTile);
                tileBackground.setOutlineColor(isFilled ? m_currentTheme.gridEmptyTile : m_currentTheme.gridEmptyTile); // Use distinct outline colors
                m_window.draw(tileBackground);

                if (!isFilled) { // Draw gem if tile is empty and word has rarity
                    sf::Sprite* gemSprite = nullptr;
                    const sf::Texture* gemTexture = nullptr;
                    if (wordRarity == 2 && m_sapphireSpr) { gemSprite = m_sapphireSpr.get(); gemTexture = &m_sapphireTex; }
                    else if (wordRarity == 3 && m_rubySpr) { gemSprite = m_rubySpr.get(); gemTexture = &m_rubyTex; }
                    else if (wordRarity == 4 && m_diamondSpr) { gemSprite = m_diamondSpr.get(); gemTexture = &m_diamondTex; }

                    if (gemSprite && gemTexture && gemTexture->getSize().y > 0) {
                        float desiredGemHeight_grid = finalRenderTileSize * 0.60f;
                        float gemScale_grid = desiredGemHeight_grid / static_cast<float>(gemTexture->getSize().y);
                        gemSprite->setScale(sf::Vector2f(gemScale_grid, gemScale_grid));
                        gemSprite->setOrigin(sf::Vector2f(static_cast<float>(gemTexture->getSize().x) / 2.f, static_cast<float>(gemTexture->getSize().y) / 2.f));
                        gemSprite->setPosition(p_tile + sf::Vector2f(finalRenderTileSize / 2.f, finalRenderTileSize / 2.f));
                        m_window.draw(*gemSprite);
                    }
                }
                else { // Draw letter if tile is filled
                    bool isAnimatingToTile = false;
                    for (const auto& anim : m_anims) {
                        if (anim.target == AnimTarget::Grid && anim.wordIdx == static_cast<int>(w) && anim.charIdx == static_cast<int>(c) && anim.t < 1.0f) {
                            isAnimatingToTile = true;
                            break;
                        }
                    }
                    if (!isAnimatingToTile) {
                        float currentFlourishScale = 1.0f;
                        for (const auto& flourish : m_gridFlourishes) {
                            if (flourish.wordIdx == static_cast<int>(w) && flourish.charIdx == static_cast<int>(c)) {
                                float progress = (GRID_FLOURISH_DURATION - flourish.timer) / GRID_FLOURISH_DURATION;
                                currentFlourishScale = 1.0f + 0.4f * std::sin(progress * PI); // Example flourish
                                break;
                            }
                        }
                        letterText_grid.setString(std::string(1, m_grid[w][c]));
                        letterText_grid.setFillColor(m_currentTheme.gridLetter);
                        sf::FloatRect b_grid_letter = letterText_grid.getLocalBounds();
                        letterText_grid.setOrigin(sf::Vector2f(b_grid_letter.position.x + b_grid_letter.size.x / 2.f, b_grid_letter.position.y + b_grid_letter.size.y / 2.f));
                        letterText_grid.setPosition(p_tile + sf::Vector2f(finalRenderTileSize / 2.f, finalRenderTileSize / 2.f));
                        letterText_grid.setScale(sf::Vector2f(currentFlourishScale, currentFlourishScale));
                        m_window.draw(letterText_grid);
                        if (currentFlourishScale != 1.0f) letterText_grid.setScale(sf::Vector2f(1.f, 1.f)); // Reset scale
                    }
                }
            }
        }
    }

    // --- Draw Path Lines (for word selection on wheel) ---
    if (m_dragging && !m_path.empty() && !m_wheelLetterRenderPos.empty()) {
        // ... (Existing path line drawing logic using TriangleStrip - ensure this is SFML 3.0 compatible if it had specific API uses) ...
        // This was:
        // if (m_path.size() >= 2) { sf::VertexArray finalPathStrip ... m_window.draw(finalPathStrip); }
        // if (m_path.back() >= 0 && ...) { sf::VertexArray rubberBandStrip ... m_window.draw(rubberBandStrip); }
        // This logic itself should be fine with SFML 3 as VertexArray is still core.
        // Just ensure any Vector math or Rect usage was updated if it existed there.
        // For brevity, assuming this detailed block is correct from previous.
        const float halfThickness = scaledPathThickness / 2.0f;
        const sf::Color pathColor = m_currentTheme.dragLine;

        if (m_path.size() >= 2) {
            sf::VertexArray finalPathStrip(sf::PrimitiveType::TriangleStrip);
            for (size_t i = 0; i < m_path.size() - 1; ++i) {
                if (static_cast<size_t>(m_path[i]) < m_wheelLetterRenderPos.size() && static_cast<size_t>(m_path[i + 1]) < m_wheelLetterRenderPos.size()) {
                    sf::Vector2f p1 = m_wheelLetterRenderPos[m_path[i]];
                    sf::Vector2f p2 = m_wheelLetterRenderPos[m_path[i + 1]];
                    sf::Vector2f direction = p2 - p1;
                    float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
                    if (length < 0.1f) continue;
                    sf::Vector2f unitPerpendicular = { -direction.y / length, direction.x / length };
                    sf::Vector2f offset = unitPerpendicular * halfThickness;
                    finalPathStrip.append(sf::Vertex(p1 - offset, pathColor));
                    finalPathStrip.append(sf::Vertex(p1 + offset, pathColor));
                    finalPathStrip.append(sf::Vertex(p2 - offset, pathColor));
                    finalPathStrip.append(sf::Vertex(p2 + offset, pathColor));
                }
            }
            if (finalPathStrip.getVertexCount() > 0) m_window.draw(finalPathStrip);
        }
        if (static_cast<size_t>(m_path.back()) < m_wheelLetterRenderPos.size()) {
            sf::Vector2f p1 = m_wheelLetterRenderPos[m_path.back()];
            sf::Vector2f p2 = mousePos;
            sf::Vector2f direction = p2 - p1;
            float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
            if (length > 0.1f) {
                sf::Vector2f unitPerpendicular = { -direction.y / length, direction.x / length };
                sf::Vector2f offset = unitPerpendicular * halfThickness;
                sf::VertexArray rubberBandStrip(sf::PrimitiveType::TriangleStrip, 4);
                rubberBandStrip[0].position = p1 - offset; rubberBandStrip[0].color = pathColor;
                rubberBandStrip[1].position = p1 + offset; rubberBandStrip[1].color = pathColor;
                rubberBandStrip[2].position = p2 - offset; rubberBandStrip[2].color = pathColor;
                rubberBandStrip[3].position = p2 + offset; rubberBandStrip[3].color = pathColor;
                m_window.draw(rubberBandStrip);
            }
        }
    }


    // --- Draw Wheel Letters ---
    if (!m_base.empty() && !m_wheelLetterRenderPos.empty()) {
        float fontScaleRatio = 1.f;
        if (LETTER_R_BASE_DESIGN > 0.1f && m_currentLetterRenderRadius > 0.1f) {
            fontScaleRatio = m_currentLetterRenderRadius / LETTER_R_BASE_DESIGN;
        }
        fontScaleRatio = std::clamp(fontScaleRatio, 0.5f, 1.5f);
        unsigned int actualScaledWheelLetterFontSize = static_cast<unsigned int>(
            std::max(8.0f, S(this, WHEEL_LETTER_FONT_SIZE_BASE_DESIGN) * fontScaleRatio)
            );
        sf::Text chTxt_wheel(m_font, "", actualScaledWheelLetterFontSize);

        for (std::size_t i = 0; i < m_base.size(); ++i) {
            if (i >= m_wheelLetterRenderPos.size()) continue;
            bool isHilited = std::find(m_path.begin(), m_path.end(), static_cast<int>(i)) != m_path.end();
            sf::Vector2f renderPos_wheel = m_wheelLetterRenderPos[i];

            sf::CircleShape letterCircle(m_currentLetterRenderRadius); // Radius already scaled by layout
            letterCircle.setOrigin(sf::Vector2f(m_currentLetterRenderRadius, m_currentLetterRenderRadius));
            letterCircle.setPosition(renderPos_wheel);
            letterCircle.setFillColor(isHilited ? m_currentTheme.wheelOutline : m_currentTheme.letterCircleNormal);
            letterCircle.setOutlineColor(m_currentTheme.wheelOutline);
            letterCircle.setOutlineThickness(scaledLetterCircleOutline);
            m_window.draw(letterCircle);

            chTxt_wheel.setString(std::string(1, static_cast<char>(std::toupper(m_base[i]))));
            chTxt_wheel.setFillColor(isHilited ? m_currentTheme.letterTextHighlight : m_currentTheme.letterTextNormal);
            sf::FloatRect txtBounds_ch_wheel = chTxt_wheel.getLocalBounds();
            chTxt_wheel.setOrigin(sf::Vector2f(txtBounds_ch_wheel.position.x + txtBounds_ch_wheel.size.x / 2.f, txtBounds_ch_wheel.position.y + txtBounds_ch_wheel.size.y / 2.f));
            chTxt_wheel.setPosition(renderPos_wheel);
            m_window.draw(chTxt_wheel);
        }
    }

    // --- Draw Flying Letter Animations ---
    if (!m_anims.empty()) {
        sf::Text flyingLetterText(m_font, "", scaledFlyingLetterFontSize);
        sf::Color flyColorBase = m_currentTheme.gridLetter; // Or a generic flying letter color
        for (const auto& a : m_anims) {
            sf::Color currentFlyColor = (a.target == AnimTarget::Score) ? sf::Color::Yellow : flyColorBase;
            float alpha_ratio = (a.t > 0.7f) ? std::max(0.0f, (1.0f - a.t) / 0.3f) : 1.0f;
            currentFlyColor.a = static_cast<std::uint8_t>(255.f * alpha_ratio);
            flyingLetterText.setFillColor(currentFlyColor);

            float eased_t = a.t * a.t * (3.f - 2.f * a.t); // Simple ease-out cubic
            sf::Vector2f p_anim = a.start + (a.end - a.start) * eased_t;

            flyingLetterText.setString(std::string(1, a.ch));
            sf::FloatRect bounds_fly = flyingLetterText.getLocalBounds();
            flyingLetterText.setOrigin(sf::Vector2f(bounds_fly.position.x + bounds_fly.size.x / 2.f, bounds_fly.position.y + bounds_fly.size.y / 2.f));
            flyingLetterText.setPosition(p_anim);
            m_window.draw(flyingLetterText);
        }
    }

    // --- Draw Score Flourishes & Hint Point Animations ---
    m_renderScoreFlourishes(m_window);  // Pass target (m_window)
    m_renderHintPointAnims(m_window); // Pass target (m_window)

    // --- Draw Guess Display (above wheel) ---
    if (m_gameState == GState::Playing && !m_currentGuess.empty() && m_guessDisplay_Text && m_guessDisplay_Bg.getPointCount() > 0) {
        // Scaled values needed for this section (ensure these are defined at the top of m_renderGameScreen or are members)
        const float scaledGuessDisplayPadX = S(this, 15.f);
        const float scaledGuessDisplayPadY = S(this, 5.f);
        const float scaledGuessDisplayGap = S(this, GUESS_DISPLAY_GAP); // From Constants.h
        // scaledGuessDisplayFontSize is already defined at the top of m_renderGameScreen

        m_guessDisplay_Text->setCharacterSize(scaledGuessDisplayFontSize);
        m_guessDisplay_Text->setString(m_currentGuess);

        // 1. Calculate dynamic size for the background based on current text
        sf::FloatRect textBounds = m_guessDisplay_Text->getLocalBounds();
        sf::Vector2f bgSize = {
            textBounds.position.x + textBounds.size.x + 2 * scaledGuessDisplayPadX,
            textBounds.position.y + textBounds.size.y + 2 * scaledGuessDisplayPadY
        };
        m_guessDisplay_Bg.setSize(bgSize);
        // Radius and outline thickness are usually set in m_updateLayout, but ensure they are reasonable
        // m_guessDisplay_Bg.setRadius(S(this, 8.f));
        // m_guessDisplay_Bg.setOutlineThickness(S(this, 1.f));

        // 2. Calculate position for the background and text
        // m_wheelX, m_wheelY, and m_visualBgRadius are member variables updated in m_updateLayout
        float wheelVisualTopY = m_wheelY - m_visualBgRadius; // Top edge of the visual wheel area
        float guessDisplayCenterY = wheelVisualTopY - (bgSize.y / 2.f) - scaledGuessDisplayGap;

        // 3. Position the background
        m_guessDisplay_Bg.setOrigin({ bgSize.x / 2.f, bgSize.y / 2.f }); // Origin to center
        m_guessDisplay_Bg.setPosition({ m_wheelX, guessDisplayCenterY }); // Centered above wheel

        // 4. Position the text on the (now correctly positioned) background
        // centerTextOnShape_General will use m_guessDisplay_Bg's new position
        centerTextOnShape_General(*m_guessDisplay_Text, m_guessDisplay_Bg);

        // 5. Set colors from theme
        m_guessDisplay_Text->setFillColor(m_currentTheme.letterTextNormal);
        m_guessDisplay_Bg.setFillColor(m_currentTheme.gridFilledTile);
        m_guessDisplay_Bg.setOutlineColor(m_currentTheme.gridFilledTile);

        // 6. Draw
        m_window.draw(m_guessDisplay_Bg);
        m_window.draw(*m_guessDisplay_Text);
    }

    // --- Hint Zone UI (Left Side) ---
    // 1. Draw "Bonus Words: X/Y" at the top of the Hint Zone
    if (m_bonusWordsInHintZoneText) {
        int totalPossibleBonus = m_calculateTotalPossibleBonusWords();
        std::string bonusTextStr = "Bonus Words: " + std::to_string(m_foundBonusWords.size()) + "/" + std::to_string(totalPossibleBonus);
        m_bonusWordsInHintZoneText->setString(bonusTextStr);
        m_bonusWordsInHintZoneText->setFillColor(GLOWING_TUBE_TEXT_COLOR); // Or your theme color

        // Store original state (position is what layout set, origin usually top-left from layout)
        sf::Vector2f originalPosition = m_bonusWordsInHintZoneText->getPosition();
        sf::Vector2f originalOrigin = m_bonusWordsInHintZoneText->getOrigin();
        sf::Vector2f originalScale = m_bonusWordsInHintZoneText->getScale(); // Should be {1,1}

        if (m_bonusTextFlourishTimer > 0.f) {
            float progress = (BONUS_TEXT_FLOURISH_DURATION - m_bonusTextFlourishTimer) / BONUS_TEXT_FLOURISH_DURATION;
            float scaleFactor = 1.0f + 0.4f * std::sin(progress * PI); // Flourish scale

            // Get bounds and calculate visual center *before* changing origin or scale
            sf::FloatRect localBounds = m_bonusWordsInHintZoneText->getLocalBounds();

            // Calculate the current visual center of the text on the screen
            // This considers the originalOrigin and originalPosition
            sf::Vector2f visualCenterOnScreen = {
                originalPosition.x - originalOrigin.x * originalScale.x + (localBounds.position.x + localBounds.size.x / 2.f) * originalScale.x,
                originalPosition.y - originalOrigin.y * originalScale.y + (localBounds.position.y + localBounds.size.y / 2.f) * originalScale.y
            };

            // Set origin to local center of the text
            sf::Vector2f newLocalCenterOrigin = {
                localBounds.position.x + localBounds.size.x / 2.f,
                localBounds.position.y + localBounds.size.y / 2.f
            };
            m_bonusWordsInHintZoneText->setOrigin(newLocalCenterOrigin);

            // Set position to the previously calculated visualCenterOnScreen
            // Now, scaling will happen around this new origin, which is at the visual center.
            m_bonusWordsInHintZoneText->setPosition(visualCenterOnScreen);
            m_bonusWordsInHintZoneText->setScale({ scaleFactor, scaleFactor });
        }

        m_window.draw(*m_bonusWordsInHintZoneText);

        // Restore original state if it was changed for flourish
        if (m_bonusTextFlourishTimer > 0.f) {
            m_bonusWordsInHintZoneText->setOrigin(originalOrigin);
            m_bonusWordsInHintZoneText->setPosition(originalPosition);
            m_bonusWordsInHintZoneText->setScale(originalScale);
        }
    }

    // 2. Draw Hint Buttons (Frames with click feedback, Lights, Labels)
    const int HINT_COSTS_FOR_LIGHTS[] = { HINT_COST_REVEAL_FIRST, HINT_COST_REVEAL_RANDOM, HINT_COST_REVEAL_LAST, HINT_COST_REVEAL_FIRST_OF_EACH };
    std::vector<std::unique_ptr<sf::Text>*> hintLabels_render = {
       &m_hintRevealFirstButtonText, &m_hintRevealRandomButtonText,
       &m_hintRevealLastButtonText, &m_hintRevealFirstOfEachButtonText
    };

    for (size_t i = 0; i < m_hintFrameSprites.size(); ++i) {
        if (m_hintFrameSprites[i]) {
            // Click feedback color
            if (i < m_hintFrameClickAnimTimers.size() && m_hintFrameClickAnimTimers[i] > 0.f) {
                m_hintFrameSprites[i]->setColor(m_hintFrameClickColor);
            }
            else {
                m_hintFrameSprites[i]->setColor(m_hintFrameNormalColor); // sf::Color::White for normal texture
            }
            m_window.draw(*m_hintFrameSprites[i]);

            // Draw Indicator Light on top of the frame
            if (i < m_hintIndicatorLightSprs.size() && m_hintIndicatorLightSprs[i]) {
                bool canAfford = (m_hintPoints >= HINT_COSTS_FOR_LIGHTS[i]);
                m_hintIndicatorLightSprs[i]->setColor(canAfford ? sf::Color::White : sf::Color(70, 70, 70, 180)); // Dim if cannot afford
                m_window.draw(*m_hintIndicatorLightSprs[i]);
            }

            // Draw Hint Label Text on top of the frame
            if (i < hintLabels_render.size() && hintLabels_render[i] && hintLabels_render[i]->get()) {
                hintLabels_render[i]->get()->setFillColor(m_currentTheme.scoreTextLabel); // Theme color
                m_window.draw(*(hintLabels_render[i]->get()));
            }
        }
    }
    // Old m_hintPointsText and individual cost texts are NOT drawn here anymore.

    // 3. Draw Hint Hover Pop-up
    if (m_hoveredHintIndex != -1 && m_popupAvailablePointsText && m_popupHintCostText && m_popupHintDescriptionText) {
        m_popupAvailablePointsText->setString("Available Points: " + std::to_string(m_hintPoints));

        int cost = 0;
        std::string desc = "";
        // HintType typeForPopup = HintType::RevealFirst; // Not strictly needed for display

        switch (m_hoveredHintIndex) {
        case 0: cost = HINT_COST_REVEAL_FIRST; desc = HINT_DESC_REVEAL_FIRST; break;
        case 1: cost = HINT_COST_REVEAL_RANDOM; desc = HINT_DESC_REVEAL_RANDOM; break;
        case 2: cost = HINT_COST_REVEAL_LAST; desc = HINT_DESC_REVEAL_LAST; break;
        case 3: cost = HINT_COST_REVEAL_FIRST_OF_EACH; desc = HINT_DESC_REVEAL_FIRST_OF_EACH; break;
        }
        m_popupHintCostText->setString("Cost: " + std::to_string(cost));
        m_popupHintDescriptionText->setString(desc);

        // Set colors for pop-up text (can be themed)
        m_popupAvailablePointsText->setFillColor(sf::Color(220, 220, 220));
        m_popupHintCostText->setFillColor(sf::Color(220, 220, 200));
        m_popupHintDescriptionText->setFillColor(sf::Color(180, 190, 200));

        // Background and text positions are set in m_updateLayout.
        // Re-set origin for description text if it needs to wrap and you want to align.
        // For now, assuming top-left origin and short enough descriptions.
        sf::FloatRect descBounds = m_popupHintDescriptionText->getLocalBounds();
        m_popupHintDescriptionText->setOrigin(sf::Vector2f(descBounds.position.x, descBounds.position.y)); // Ensure top-left for potentially wrapped text

        m_window.draw(m_hintPopupBackground);
        m_window.draw(*m_popupAvailablePointsText);
        m_window.draw(*m_popupHintCostText);
        m_window.draw(*m_popupHintDescriptionText);
    }

    // --- Scramble Button (Bottom near wheel) ---
    if (m_gameState == GState::Playing && m_scrambleSpr) {
        bool scrambleHover = m_scrambleSpr->getGlobalBounds().contains(mousePos);
        m_scrambleSpr->setColor(scrambleHover ? sf::Color::White : sf::Color(255, 255, 255, 200)); // Hover effect
        m_window.draw(*m_scrambleSpr);
    }

    // --- Draw Solved State Overlay (if game over) ---
    if (m_currentScreen == GameScreen::GameOver && m_contBtn.getPointCount() > 0 && m_contTxt) {
        // m_solvedOverlay, winTxt, m_contBtn, m_contTxt positions are set in m_updateLayout
        // or should be if they are specific to this screen state.
        // For now, assume m_updateLayout handles their general game-over positions.

        sf::Text winTxt(m_font, "Puzzle Solved!", scaledSolvedFontSize);
        winTxt.setFillColor(m_currentTheme.hudTextSolved);
        winTxt.setStyle(sf::Text::Bold);

        // Simplified overlay positioning for this example - ideally done in layout
        sf::FloatRect winTxtBounds = winTxt.getLocalBounds();
        float overlayWidth = std::max(winTxtBounds.size.x, m_contBtn.getSize().x) + S(this, 50.f);
        float overlayHeight = winTxtBounds.size.y + m_contBtn.getSize().y + S(this, 70.f);

        m_solvedOverlay.setSize(sf::Vector2f(overlayWidth, overlayHeight));
        m_solvedOverlay.setRadius(S(this, 15.f));
        m_solvedOverlay.setFillColor(m_currentTheme.solvedOverlayBg);
        // Center overlay on screen (design space)
        sf::Vector2f screenCenter(static_cast<float>(REF_W) / 2.f, static_cast<float>(REF_H) / 2.f);
        m_solvedOverlay.setOrigin(sf::Vector2f(overlayWidth / 2.f, overlayHeight / 2.f));
        m_solvedOverlay.setPosition(screenCenter);

        // Position "Puzzle Solved!" text
        winTxt.setOrigin(sf::Vector2f(winTxtBounds.position.x + winTxtBounds.size.x / 2.f, winTxtBounds.position.y + winTxtBounds.size.y / 2.f));
        winTxt.setPosition(sf::Vector2f(screenCenter.x, screenCenter.y - overlayHeight / 2.f + winTxtBounds.size.y / 2.f + S(this, 20.f)));

        // Position Continue Button
        m_contBtn.setOrigin(sf::Vector2f(m_contBtn.getSize().x / 2.f, 0.f)); // Origin top-center for easier stacking
        m_contBtn.setPosition(sf::Vector2f(screenCenter.x, winTxt.getPosition().y + winTxt.getGlobalBounds().size.y / 2.f + S(this, 15.f)));

        bool contHover = m_contBtn.getGlobalBounds().contains(mousePos);
        m_contBtn.setFillColor(contHover ? adjustColorBrightness(m_currentTheme.continueButton, 1.2f) : m_currentTheme.continueButton);

        m_contTxt->setCharacterSize(scaledContinueFontSize);
        centerTextOnShape_General(*m_contTxt, m_contBtn); // Center text on button
        m_contTxt->setFillColor(m_currentTheme.letterTextNormal);

        m_window.draw(m_solvedOverlay);
        m_window.draw(winTxt);
        m_window.draw(m_contBtn);
        m_window.draw(*m_contTxt);
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

void Game::m_spawnHintPointAnimation(const sf::Vector2f& actualStartPosition, int pointsAwarded) {
    if (!m_bonusWordsInHintZoneText) { // Target text for points animation must exist
        std::cerr << "ERROR: m_spawnHintPointAnimation - m_bonusWordsInHintZoneText is null! Cannot set target for hint point animation." << std::endl;
        return;
    }

    HintPointAnimParticle particle;
    particle.startPosition = actualStartPosition; // Where the "+X" appears initially

    particle.textString = "+" + std::to_string(pointsAwarded);
    particle.color = sf::Color(255, 215, 0, 255); // Gold-like color

    // Target position: Center of m_bonusWordsInHintZoneText
    sf::FloatRect bonusSummaryBounds = m_bonusWordsInHintZoneText->getGlobalBounds();
    particle.targetPosition = {
        bonusSummaryBounds.position.x + bonusSummaryBounds.size.x / 2.f,
        (bonusSummaryBounds.position.y + bonusSummaryBounds.size.y / 2.f) - 100.f
    };

    particle.speed = HINT_POINT_ANIM_SPEED; // Adjust for desired duration
    particle.t = 0.f;

    m_hintPointAnims.push_back(particle);

    // --- NEW: Trigger the flourish of m_bonusWordsInHintZoneText immediately ---
    // Ensure BONUS_TEXT_FLOURISH_DURATION is defined (e.g., in Constants.h or Game.h)
    m_bonusTextFlourishTimer = BONUS_TEXT_FLOURISH_DURATION;
    // --- END NEW ---
}

void Game::m_updateHintPointAnims(float dt) {
    m_hintPointAnims.erase(
        std::remove_if(m_hintPointAnims.begin(), m_hintPointAnims.end(),
            [&](HintPointAnimParticle& p) { // Capture 'this' if S() or other members are needed
                p.t += dt * p.speed; // Advance animation based on speed

                if (p.t >= 1.f) {
                    p.t = 1.f; // Clamp to ensure precise end
                    return true; // Mark for removal
                }

                // Update alpha for fade-out effect (e.g., in the last 30% of the animation)
                if (p.t > 0.7f) {
                    float fadeRatio = (1.0f - p.t) / 0.3f; // Ranges from 1.0 (at t=0.7) down to 0.0 (at t=1.0)
                    p.color.a = static_cast<std::uint8_t>(std::max(0.f, std::min(255.f, fadeRatio * 255.f)));
                }
                else {
                    p.color.a = 255; // Fully visible for the first 70%
                }
                // The particle's currentPosition is calculated during rendering based on p.t

                return false; // Keep particle active
            }),
        m_hintPointAnims.end()
    );
}

void Game::m_renderHintPointAnims(sf::RenderTarget& target) {
    if (m_font.getInfo().family.empty()) { // Check if font is valid
        return;
    }

    unsigned int scaledCharSize = static_cast<unsigned int>(std::max(8.0f, HINT_POINT_ANIM_FONT_SIZE_DESIGN));
    sf::Text renderText(m_font, "", scaledCharSize);
    renderText.setStyle(sf::Text::Bold); // Optional

    for (const auto& p : m_hintPointAnims) {
        renderText.setString(p.textString);
        renderText.setFillColor(p.color); // Color now includes alpha for fading

        // Calculate current interpolated position
        // You can add easing here if desired. For example, ease-out-quad:
        float eased_t = 1.f - (1.f - p.t) * (1.f - p.t);
        // Or linear: float eased_t = p.t;

        sf::Vector2f currentPosition = p.startPosition + (p.targetPosition - p.startPosition) * eased_t;
        renderText.setPosition(currentPosition);

        // Center the text
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
    float textRenderScale = 1.0f;
    const sf::FloatRect gridZone = GRID_ZONE_RECT_DESIGN;
    float maxPopupDesignWidth = gridZone.size.x * POPUP_MAX_WIDTH_DESIGN_RATIO;
    float maxPopupDesignHeight = gridZone.size.y * POPUP_MAX_HEIGHT_DESIGN_RATIO;
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

    // Cap overall popup size to what's allowed *within* the grid zone ratio
    finalPopupWidth = std::min(finalPopupWidth, maxPopupDesignWidth);
    finalPopupHeight = std::min(finalPopupHeight, maxPopupDesignHeight);

    // --- NEW: Center the pop-up within GRID_ZONE_RECT_DESIGN ---
    float gridZoneCenterX = gridZone.position.x + gridZone.size.x / 2.f;
    float gridZoneCenterY = gridZone.position.y + gridZone.size.y / 2.f;

    float popupX = gridZoneCenterX - finalPopupWidth / 2.f;
    float popupY = gridZoneCenterY - finalPopupHeight / 2.f;

    // Optional: Ensure the pop-up (if very large) doesn't start outside the main screen,
    // though centering in grid zone usually makes this less of an issue unless grid zone is off-center.
    popupX = std::max(popupX, S(this, 5.f)); // Min screen padding
    popupY = std::max(popupY, S(this, 5.f));
    if (popupX + finalPopupWidth > static_cast<float>(REF_W) - S(this, 5.f)) {
        popupX = static_cast<float>(REF_W) - finalPopupWidth - S(this, 5.f);
    }
    if (popupY + finalPopupHeight > static_cast<float>(REF_H) - S(this, 5.f)) {
        popupY = static_cast<float>(REF_H) - finalPopupHeight - S(this, 5.f);
    }

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

void Game::centerTextOnShape(sf::Text& text, const sf::Shape& shape) {
    sf::FloatRect textBounds = text.getLocalBounds();
    text.setOrigin({ textBounds.position.x + textBounds.size.x / 2.f,
        textBounds.position.y + textBounds.size.y / 2.f });

    // Assumes shape's origin is top-left for getPosition()
    // Uses shape's local bounds for its size, good if shape isn't scaled weirdly
    sf::FloatRect shapeLocalBounds = shape.getLocalBounds();
    text.setPosition({ shape.getPosition().x + shapeLocalBounds.size.x / 2.f,
        shape.getPosition().y + shapeLocalBounds.size.y / 2.f });

}

int Game::m_calculateTotalPossibleBonusWords() const {
    if (m_base == "ERROR" || m_allPotentialSolutions.empty()) {
        return 0;
    }
    int count = 0;
    for (const auto& wordInfo : m_allPotentialSolutions) {
        if (!isGridSolution(wordInfo.text)) { // isGridSolution helper is already defined
            count++;
        }
    }
    return count;
}

void Game::m_triggerBonusListCompleteEffect(int pointsAwarded) {
    if (m_bonusListCompleteEffectActive) return; // Don't trigger if already active

    std::cout << "DEBUG: Triggering Bonus List Complete Effect for " << pointsAwarded << " points." << std::endl;

    m_bonusListCompleteEffectActive = true;
    m_bonusListCompletePointsAwarded = pointsAwarded;
    m_bonusListCompleteAnimTimer = 0.f;
    m_bonusListCompletePopupDisplayTimer = 3.0f; // Display "Bonus List Complete: +XXX" for 3 seconds

    // --- Setup the main popup text ---
    m_bonusListCompletePopupText.setFont(m_font);
    m_bonusListCompletePopupText.setString("Bonus List Complete: +" + std::to_string(pointsAwarded));
    // Scaled font size (adjust base size as needed)
    unsigned int popupFontSize = static_cast<unsigned int>(std::max(12.0f, S(this, 28.f)));
    m_bonusListCompletePopupText.setCharacterSize(popupFontSize);
    m_bonusListCompletePopupText.setFillColor(sf::Color::Yellow); // Or a theme color
    m_bonusListCompletePopupText.setStyle(sf::Text::Bold);

    sf::FloatRect popupBounds = m_bonusListCompletePopupText.getLocalBounds();
    m_bonusListCompletePopupText.setOrigin(sf::Vector2f(popupBounds.position.x + popupBounds.size.x / 2.f,
        popupBounds.position.y + popupBounds.size.y / 2.f));
    // Position it in the center of the screen (design space)
    m_bonusListCompletePopupText.setPosition(sf::Vector2f(static_cast<float>(REF_W) / 2.f,
        static_cast<float>(REF_H) / 2.f));

    m_bonusListCompleteAnimStartPos = m_bonusListCompletePopupText.getPosition();

    // --- Setup the animating points text (initially same as popup) ---
    m_bonusListCompleteAnimatingPointsText = m_bonusListCompletePopupText; // Copy properties
    m_bonusListCompleteAnimatingPointsText.setString("+" + std::to_string(pointsAwarded)); // Ensure it's just the points

    // Target for animation: center of the m_scoreValueText
    if (m_scoreValueText) {
        sf::FloatRect scoreTextBounds = m_scoreValueText->getGlobalBounds(); // Use global for screen coords
        m_bonusListCompleteAnimEndPos = {
            scoreTextBounds.position.x + scoreTextBounds.size.x / 2.f,
            scoreTextBounds.position.y + scoreTextBounds.size.y / 2.f
        };
    }
    else {
        // Fallback if score text isn't available (shouldn't happen)
        m_bonusListCompleteAnimEndPos = { static_cast<float>(REF_W) * 0.8f, static_cast<float>(REF_H) * 0.1f };
    }

    if (m_winSound) {
        m_winSound->play();
    }
}

void Game::m_updateBonusListCompleteEffect(float dt) {
    if (!m_bonusListCompleteEffectActive) return;

    m_bonusListCompletePopupDisplayTimer -= dt;

    // Animation of points flying to score
    const float animDuration = 1.5f; // How long the points take to fly
    m_bonusListCompleteAnimTimer += dt;

    if (m_bonusListCompleteAnimTimer <= animDuration) {
        float t = m_bonusListCompleteAnimTimer / animDuration;
        // Simple easing (ease out quad)
        t = 1.f - (1.f - t) * (1.f - t);

        sf::Vector2f currentPos = m_bonusListCompleteAnimStartPos +
            (m_bonusListCompleteAnimEndPos - m_bonusListCompleteAnimStartPos) * t;
        m_bonusListCompleteAnimatingPointsText.setPosition(currentPos);

        // Fade out the animating points text as it nears the target
        if (t > 0.7f) {
            float alphaRatio = (1.0f - t) / 0.3f; // Fades from t=0.7 to t=1.0
            sf::Color c = m_bonusListCompleteAnimatingPointsText.getFillColor();
            c.a = static_cast<std::uint8_t>(std::max(0.f, std::min(255.f, alphaRatio * 255.f)));
            m_bonusListCompleteAnimatingPointsText.setFillColor(c);
        }

    }
    else {
        // Animation finished
        if (m_bonusListCompleteEffectActive && m_bonusListCompleteAnimTimer > animDuration) { // Ensure this only happens once
            // Add points to score and flourish (if not already done)
            // This ensures points are added when animation *finishes*
            if (m_bonusListCompletePointsAwarded > 0) { // Check to avoid adding 0 if something went wrong
                m_currentScore += m_bonusListCompletePointsAwarded;
                if (m_scoreValueText) m_scoreValueText->setString(std::to_string(m_currentScore));
                m_scoreFlourishTimer = SCORE_FLOURISH_DURATION; // Trigger main score flourish
                m_bonusListCompletePointsAwarded = 0; // Prevent re-adding
            }
        }
    }

    // Deactivate effect once popup and animation are done (or popup timer expires)
    if (m_bonusListCompletePopupDisplayTimer <= 0.f && m_bonusListCompleteAnimTimer > animDuration) {
        m_bonusListCompleteEffectActive = false;
    }
}

void Game::m_renderBonusListCompleteEffect(sf::RenderTarget& target) {
    if (!m_bonusListCompleteEffectActive) return;

    // Draw the main "Bonus List Complete: +XXX" popup while its timer is active
    if (m_bonusListCompletePopupDisplayTimer > 0.f) {
        // Optional: Fade out the main popup towards the end of its display time
        if (m_bonusListCompletePopupDisplayTimer < 0.5f) { // Last 0.5 seconds
            sf::Color c = m_bonusListCompletePopupText.getFillColor();
            c.a = static_cast<std::uint8_t>((m_bonusListCompletePopupDisplayTimer / 0.5f) * 255.f);
            m_bonusListCompletePopupText.setFillColor(c);
        }
        else {
            sf::Color c = m_bonusListCompletePopupText.getFillColor();
            c.a = 255;
            m_bonusListCompletePopupText.setFillColor(c);
        }
        target.draw(m_bonusListCompletePopupText);
    }

    // Draw the animating points text if it's still within its animation duration
    const float animDuration = 1.5f;
    if (m_bonusListCompleteAnimTimer <= animDuration) {
        target.draw(m_bonusListCompleteAnimatingPointsText);
    }
}