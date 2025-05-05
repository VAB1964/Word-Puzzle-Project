#include "Theme.h"
#include <utility>
#include <cstdint>
#include "Constants.h"
#include "ThemeData.h"
#include "GameData.h"
#include "Game.h" // Should ideally include everything needed by the Game class declaration
#include "Words.h"
#include "Utils.h"
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp> 
#include <SFML/Window/VideoMode.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Window/Mouse.hpp>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iostream> // For error messages
#include <stdexcept> // For std::stof, std::stoi exceptions
#include <algorithm>
#include <string>
#include <vector> // Ensure needed standard headers are here
#include <set>
#include <functional>
#include <cmath>
#include <numeric>
#include <random> // For std::mt19937, std::uniform_real_distribution, etc.
#include <ctime>
#include <memory> // For unique_ptr    
#include <limits>
#include <cctype>
#include <map>

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
    m_celebrationEffectTimer(0.f),
    m_currentScreen(GameScreen::MainMenu),
    m_gameState(GState::Playing),
    m_hintsAvailable(INITIAL_HINTS),         // Use constant directly here
    m_wordsSolvedSinceHint(0),
    m_currentScore(0),
    m_scoreFlourishTimer(0.f),
    m_bonusTextFlourishTimer(0.f),
    m_dragging(false),
    m_decor(10),                             // Initialize DecorLayer
    m_selectedDifficulty(DifficultyLevel::None),
    m_puzzlesPerSession(0),
    m_currentPuzzleIndex(0),
    m_isInSession(false),
    m_uiScale(1.f),

    // Resource Handles (default construct textures/buffers - loaded later)
    m_scrambleTex(), m_hintTex(), m_sapphireTex(), m_rubyTex(), m_diamondTex(),
    m_selectBuffer(), m_placeBuffer(), m_winBuffer(), m_clickBuffer(), m_hintUsedBuffer(), m_errorWordBuffer(),
    // Sounds (will be unique_ptr, initialize to nullptr or default construct)
    m_selectSound(nullptr), m_placeSound(nullptr), m_winSound(nullptr), m_clickSound(nullptr), m_hintUsedSound(nullptr), m_errorWordSound(nullptr),
    // Music (default construct - file loaded later)
    m_backgroundMusic(),
    // Sprites (will be unique_ptr, initialize to nullptr or default construct)
    m_scrambleSpr(nullptr), m_hintSpr(nullptr), m_sapphireSpr(nullptr), m_rubySpr(nullptr), m_diamondSpr(nullptr),
    // Texts (will be unique_ptr, initialize to nullptr or default construct)
    m_contTxt(nullptr), m_scoreLabelText(nullptr), m_scoreValueText(nullptr), m_hintCountTxt(nullptr),
    m_mainMenuTitle(nullptr), m_casualButtonText(nullptr), m_competitiveButtonText(nullptr), m_quitButtonText(nullptr),
    m_casualMenuTitle(nullptr), m_easyButtonText(nullptr), m_mediumButtonText(nullptr), m_hardButtonText(nullptr), m_returnButtonText(nullptr),
    m_guessDisplay_Text(nullptr),
    m_progressMeterText(nullptr),
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
    m_returnToMenuButtonShape({ 100.f, 40.f }, 8.f, 10), // Added for return button
    m_firstFrame(true) // Initialize first frame flag
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
    if (m_hintTex.getSize().x > 0) m_hintSpr = std::make_unique<sf::Sprite>(m_hintTex);
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
    if (m_hintSpr && m_hintTex.getSize().y > 0) {
        float hintScale = HINT_BTN_HEIGHT / static_cast<float>(m_hintTex.getSize().y);
        m_hintSpr->setScale({ hintScale, hintScale });
    }

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
                    // Update Grid
                    if (a.wordIdx >= 0 && a.wordIdx < m_grid.size() &&
                        a.charIdx >= 0 && a.charIdx < m_grid[a.wordIdx].size())
                    {
                        m_grid[a.wordIdx][a.charIdx] = a.ch;
                    }
                    else { /* Optional Error Log */ }

                    if (m_placeSound) m_placeSound->play(); // Grid placement sound

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

    // Load hint button texture (Example - adjust path as needed)
    if (!m_hintTex.loadFromFile("assets/hint_icon.png")) {
        std::cerr << "Error loading hint texture!" << std::endl;
        // Consider exiting or using a fallback visual
    }
    else {
        m_hintTex.setSmooth(true);
    }

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
    m_hintSpr = std::make_unique<sf::Sprite>(m_hintTex);
    m_sapphireSpr = std::make_unique<sf::Sprite>(m_sapphireTex);
    m_rubySpr = std::make_unique<sf::Sprite>(m_rubyTex);
    m_diamondSpr = std::make_unique<sf::Sprite>(m_diamondTex);

    // --- Create Text Objects (Link Font, Set Properties) --- *** MOVED HERE ***
    m_contTxt = std::make_unique<sf::Text>(m_font, "Continue", 24);
    m_scoreLabelText = std::make_unique<sf::Text>(m_font, "SCORE:", 24);
    m_scoreValueText = std::make_unique<sf::Text>(m_font, "0", 24);
    m_hintCountTxt = std::make_unique<sf::Text>(m_font, "", 20);
    m_mainMenuTitle = std::make_unique<sf::Text>(m_font, "Main Menu", 36);
    m_casualButtonText = std::make_unique<sf::Text>(m_font, "Casual", 24);
    m_competitiveButtonText = std::make_unique<sf::Text>(m_font, "Competitive", 24);
    m_quitButtonText = std::make_unique<sf::Text>(m_font, "Quit", 24);

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
    float hintScale = hintBtnHeight / static_cast<float>(m_hintTex.getSize().y); m_hintSpr->setScale({ hintScale, hintScale });


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
    // Clear any potential default theme added erroneously earlier if necessary
    m_themes.clear();
    m_themes = loadThemes();

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
    }
    else if (m_currentScreen == GameScreen::SessionComplete) {
        m_updateCelebrationEffects(deltaSeconds);
    }
    // (No need to call m_updateScoreAnims separately if it's part of m_updateAnims or not used)
}

// --- Render ---
void Game::m_render() {
    m_window.clear(m_currentTheme.winBg);
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
    m_found.clear(); m_foundBonusWords.clear(); m_anims.clear(); m_scoreAnims.clear();
    m_clearDragState(); m_gameState = GState::Playing;

    // --- Reset Score/Hints ---
    if (m_currentPuzzleIndex == 0 && m_isInSession) { /* ... reset score/hints ... */
        m_currentScore = 0; m_hintsAvailable = INITIAL_HINTS; m_wordsSolvedSinceHint = 0;
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


void Game::m_updateLayout(sf::Vector2u windowSize) {

    // 1. Calculate Global UI Scale (No Modifier Applied Yet)
    m_uiScale = std::min(windowSize.x / static_cast<float>(REF_W),
        windowSize.y / static_cast<float>(REF_H));
    m_uiScale = std::clamp(m_uiScale, 0.65f, 1.6f); // Clamp to reasonable limits

    // 2. Define Design Space References & Sections
    const float designW = static_cast<float>(REF_W);
    const float designH = static_cast<float>(REF_H);
    const sf::Vector2f designCenter = { designW / 2.f, designH / 2.f };
    const float designTopEdge = 0.f;
    const float designBottomEdge = designH;
    const float topSectionHeight = designH * 0.15f;
    const float wheelSectionHeight = designH * 0.35f;
    const float gridSectionHeight = designH - topSectionHeight - wheelSectionHeight;
    const float topSectionBottomY = designTopEdge + topSectionHeight;
    const float gridSectionTopY = topSectionBottomY;
    const float gridSectionBottomY = gridSectionTopY + gridSectionHeight;
    const float wheelSectionTopY = gridSectionBottomY;
    const float wheelSectionBottomY = designBottomEdge;

    // --- Start Logging ---
    std::cout << "--- Layout Update (" << windowSize.x << "x" << windowSize.y << ") ---" << std::endl;
    std::cout << "Base UI Scale (m_uiScale): " << m_uiScale << std::endl;
    std::cout << "Design Space: " << designW << "x" << designH << " (Center: " << designCenter.x << "," << designCenter.y << ")" << std::endl;

    // 3. Position Top Elements (Uses Base UI Scale)
    const float scaledScoreBarWidth = S(this, SCORE_BAR_WIDTH); const float scaledScoreBarHeight = S(this, SCORE_BAR_HEIGHT); const float scaledScoreBarBottomMargin = S(this, SCORE_BAR_BOTTOM_MARGIN);
    const float scoreBarX_design = designCenter.x; const float scoreBarY_design = topSectionBottomY - scaledScoreBarBottomMargin - scaledScoreBarHeight / 2.f;
    m_scoreBar.setSize({ scaledScoreBarWidth, scaledScoreBarHeight }); m_scoreBar.setRadius(S(this, 10.f)); m_scoreBar.setOrigin({ scaledScoreBarWidth / 2.f, scaledScoreBarHeight / 2.f }); m_scoreBar.setPosition({ scoreBarX_design, scoreBarY_design }); m_scoreBar.setOutlineThickness(S(this, 1.f));
    const float scaledMeterHeight = S(this, PROGRESS_METER_HEIGHT); const float scaledMeterWidth = S(this, PROGRESS_METER_WIDTH); const float scaledMeterScoreGap = S(this, METER_SCORE_GAP);
    const float meterX_design = designCenter.x; const float meterY_design = scoreBarY_design - scaledScoreBarHeight / 2.f - scaledMeterScoreGap - scaledMeterHeight / 2.f;
    m_progressMeterBg.setSize({ scaledMeterWidth, scaledMeterHeight }); m_progressMeterBg.setOrigin({ scaledMeterWidth / 2.f, scaledMeterHeight / 2.f }); m_progressMeterBg.setPosition({ meterX_design, meterY_design }); m_progressMeterBg.setOutlineThickness(S(this, PROGRESS_METER_OUTLINE));
    m_progressMeterFill.setOrigin({ 0.f, scaledMeterHeight / 2.f }); m_progressMeterFill.setPosition({ meterX_design - scaledMeterWidth / 2.f, meterY_design });
    if (m_progressMeterText) { const unsigned int bf = 16; unsigned int sf = (unsigned int)std::max(8.0f, S(this, (float)bf)); m_progressMeterText->setCharacterSize(sf); }
    const float scaledScoreTextOffset = S(this, 5.f);
    if (m_scoreValueText) { const unsigned int bf = 24; unsigned int sf = (unsigned int)std::max(10.0f, S(this, (float)bf)); m_scoreValueText->setCharacterSize(sf); sf::FloatRect vb = m_scoreValueText->getLocalBounds(); sf::Vector2f o = { 0.f,vb.position.y + vb.size.y / 2.f }; sf::Vector2f p = { scoreBarX_design + scaledScoreTextOffset, scoreBarY_design }; m_scoreValueText->setOrigin(o); m_scoreValueText->setPosition(p); }
    if (m_scoreLabelText) { const unsigned int bf = 24; unsigned int sf = (unsigned int)std::max(10.0f, S(this, (float)bf)); m_scoreLabelText->setCharacterSize(sf); sf::FloatRect lb = m_scoreLabelText->getLocalBounds(); sf::Vector2f o = { lb.position.x + lb.size.x, lb.position.y + lb.size.y / 2.f }; sf::Vector2f p = { scoreBarX_design - scaledScoreTextOffset, scoreBarY_design }; m_scoreLabelText->setOrigin(o); m_scoreLabelText->setPosition(p); }


    // --- 4. Calculate Grid Layout ---

    // --- 4a. Initial Setup & Variables ---
    m_gridStartY = gridSectionTopY;
    const float availableGridHeight = gridSectionHeight;
    int numCols = 1;
    int maxRowsPerCol = m_sorted.empty() ? 1 : (int)m_sorted.size();
    float actualGridHeight = 0; // Declare here, calculate later
    m_wordCol.clear(); m_wordRow.clear(); m_colMaxLen.clear(); m_colXOffset.clear();
    m_currentGridLayoutScale = 1.0f; // Default to no scaling

    if (!m_sorted.empty()) {
        // --- 4b. Heuristic: Find best column/row count using BASE scale ---
        const float st_base = S(this, TILE_SIZE); const float sp_base = S(this, TILE_PAD); const float sc_base = S(this, COL_PAD);
        const float stph_base = st_base + sp_base; const float stpw_base = st_base + sp_base;
        int bestFitCols = 1; int bestFitRows = (int)m_sorted.size(); float minWidthVertFit = std::numeric_limits<float>::max(); bool foundVerticalFit = false;
        int narrowestOverallCols = 1; int narrowestOverallRows = (int)m_sorted.size(); float minWidthOverall = std::numeric_limits<float>::max();
        int maxPossibleCols = std::min(8, std::max(1, (int)m_sorted.size()));
        for (int tryCols = 1; tryCols <= maxPossibleCols; ++tryCols) {
            int rowsNeeded = ((int)m_sorted.size() + tryCols - 1) / tryCols; if (rowsNeeded <= 0) rowsNeeded = 1;
            std::vector<int> currentTryColMaxLen(tryCols, 0); float currentTryWidth = 0;
            for (size_t w = 0; w < m_sorted.size(); ++w) { int c = (int)w / rowsNeeded; if (c >= 0 && c < tryCols) { currentTryColMaxLen[c] = std::max<int>(currentTryColMaxLen[c], (int)m_sorted[w].text.length()); } }
            for (int len : currentTryColMaxLen) { currentTryWidth += (float)len * stpw_base - (len > 0 ? sp_base : 0.f); } currentTryWidth += (float)std::max(0, tryCols - 1) * sc_base; if (currentTryWidth < 0) currentTryWidth = 0;
            float currentTryHeight = (float)rowsNeeded * stph_base - (rowsNeeded > 0 ? sp_base : 0.f); if (currentTryHeight < 0) currentTryHeight = 0;
            if (currentTryWidth < minWidthOverall) { minWidthOverall = currentTryWidth; narrowestOverallCols = tryCols; narrowestOverallRows = rowsNeeded; }
            if (currentTryHeight <= availableGridHeight) { if (!foundVerticalFit || currentTryWidth < minWidthVertFit) { minWidthVertFit = currentTryWidth; bestFitCols = tryCols; bestFitRows = rowsNeeded; foundVerticalFit = true; } }
        }
        int chosenCols = narrowestOverallCols; int chosenRows = narrowestOverallRows;
        if (foundVerticalFit) { chosenCols = bestFitCols; chosenRows = bestFitRows; std::cout << "  GRID INIT: Heuristic found Vertical Fit: Cols=" << chosenCols << ", RowsPerCol=" << chosenRows << std::endl; }
        else { std::cout << "  GRID INIT: Heuristic using Narrowest Overall: Cols=" << chosenCols << ", RowsPerCol=" << chosenRows << std::endl; }

        // --- 4c. Apply MAX_ROWS_LIMIT override BEFORE width check ---
        const int MAX_ROWS_LIMIT = 5;
        if (chosenRows > MAX_ROWS_LIMIT) {
            std::cout << "  GRID OVERRIDE: Heuristic rows (" << chosenRows << ") exceed limit. Forcing Max Rows to " << MAX_ROWS_LIMIT << "." << std::endl;
            chosenRows = MAX_ROWS_LIMIT;
            chosenCols = ((int)m_sorted.size() + chosenRows - 1) / chosenRows;
            if (chosenCols <= 0) chosenCols = 1;
            std::cout << "                 Final Columns: " << chosenCols << std::endl;
        }
        else {
            std::cout << "  GRID INFO: Heuristic rows (" << chosenRows << ") within limit. Final Columns: " << chosenCols << std::endl;
        }

        // --- Finalize grid structure ---
        numCols = chosenCols;
        maxRowsPerCol = chosenRows;

        // --- 4d. Calculate Max Length per Column (using final numCols) ---
        m_colMaxLen.assign(numCols, 0);
        for (size_t w = 0; w < m_sorted.size(); ++w) {
            int c = (int)w / maxRowsPerCol; if (c >= numCols) c = numCols - 1;
            if (c >= 0 && c < m_colMaxLen.size()) { m_colMaxLen[c] = std::max<int>(m_colMaxLen[c], (int)m_sorted[w].text.length()); }
        }

        // --- 4e. Calculate Initial Width based on FINAL structure using BASE scale ---
        float currentX_base = 0;
        for (int c = 0; c < numCols; ++c) {
            int len = (c >= 0 && c < m_colMaxLen.size()) ? m_colMaxLen[c] : 0;
            float colWidth_base = (float)len * stpw_base - (len > 0 ? sp_base : 0.f);
            if (colWidth_base < 0) colWidth_base = 0;
            currentX_base += colWidth_base + sc_base;
        }
        float initialTotalGridW = currentX_base - (numCols > 0 ? sc_base : 0.f);
        if (initialTotalGridW < 0) initialTotalGridW = 0;

        // --- 4f. Check if Adjustment is Needed & Set Grid Scale Factor ---
        bool needsAdjustment = (initialTotalGridW > designW);
        m_currentGridLayoutScale = needsAdjustment ? UI_SCALE_MODIFIER : 1.0f;

        if (needsAdjustment) { std::cout << "  GRID WARNING: Final structure width (" << initialTotalGridW << ") exceeds design width (" << designW << "). Applying scale modifier: " << m_currentGridLayoutScale << std::endl; }
        else { std::cout << "  GRID INFO: Final structure width (" << initialTotalGridW << ") fits. No scale modifier needed." << std::endl; m_currentGridLayoutScale = 1.0f; }

        // --- 4g. Calculate Final Grid Dimensions & Positions using Differential Scaling ---
        float tileSpecificScaleFactor = needsAdjustment ? GRID_TILE_RELATIVE_SCALE_WHEN_SHRUNK : 1.0f;
        std::cout << "  GRID INFO: Tile specific scale factor: " << tileSpecificScaleFactor << std::endl;
        const float st_final = S(this, TILE_SIZE) * tileSpecificScaleFactor;
        const float sp_final = S(this, TILE_PAD) * m_currentGridLayoutScale;
        const float sc_final = S(this, COL_PAD) * m_currentGridLayoutScale;
        const float stph_final = st_final + sp_final;
        const float stpw_final = st_final + sp_final;

        float currentX_final = 0;
        m_colXOffset.resize(numCols);
        for (int c = 0; c < numCols; ++c) {
            m_colXOffset[c] = currentX_final;
            int len = (c >= 0 && c < m_colMaxLen.size()) ? m_colMaxLen[c] : 0;
            float colWidth_final = (float)len * stpw_final - (len > 0 ? sp_final : 0.f);
            if (colWidth_final < 0) colWidth_final = 0;
            currentX_final += colWidth_final + sc_final;
        }
        m_totalGridW = currentX_final - (numCols > 0 ? sc_final : 0.f);
        if (m_totalGridW < 0) m_totalGridW = 0;

        m_gridStartX = designCenter.x - m_totalGridW / 2.f;
        for (int c = 0; c < numCols; ++c) {
            if (c < m_colXOffset.size()) { m_colXOffset[c] += m_gridStartX; }
        }
        actualGridHeight = (float)maxRowsPerCol * stph_final - (maxRowsPerCol > 0 ? sp_final : 0.f);
        if (actualGridHeight < 0) actualGridHeight = 0;

        // Assign word rows/cols
        m_wordCol.resize(m_sorted.size()); m_wordRow.resize(m_sorted.size());
        for (size_t w = 0; w < m_sorted.size(); ++w) { int c = (int)w / maxRowsPerCol; int r = (int)w % maxRowsPerCol; if (c >= numCols) c = numCols - 1; m_wordCol[w] = c; m_wordRow[w] = r; }

        std::cout << "  GRID FINAL: Layout Structure: Cols=" << numCols << ", RowsPerCol=" << maxRowsPerCol << std::endl;
        std::cout << "  GRID H/W: Final Calculated Height = " << actualGridHeight << ", Final Calculated Width = " << m_totalGridW << std::endl;

    }
    else { // Grid is empty
        m_gridStartX = designCenter.x; m_totalGridW = 0; actualGridHeight = 0;
        m_currentGridLayoutScale = 1.0f; // Reset scale factor
        std::cout << "  GRID LAYOUT: Grid empty." << std::endl;
    }
    // --- End Grid Calculation Section ---


    // 5. Determine Final Wheel Size & Position (Uses Base UI Scale)---------------
    const float scaledGridWheelGap = S(this, GRID_WHEEL_GAP); const float scaledWheelBottomMargin = S(this, WHEEL_BOTTOM_MARGIN); const float scaledLetterRadius = S(this, LETTER_R); const float scaledHudMinHeight = S(this, HUD_AREA_MIN_HEIGHT);
    float gridActualBottomY = m_gridStartY + actualGridHeight; float gridAreaLimitY = gridActualBottomY + scaledGridWheelGap; float wheelCenterBottomLimit = designBottomEdge - scaledWheelBottomMargin - scaledHudMinHeight;
    std::cout << "--- WHEEL LAYOUT: Start Calculation ---" << std::endl; std::cout << "  Grid Actual Bottom Y: " << gridActualBottomY << ", Scaled Gap: " << scaledGridWheelGap << ", Grid Area Limit Y: " << gridAreaLimitY << std::endl; std::cout << "  Wheel Center Bottom Limit: " << wheelCenterBottomLimit << std::endl;
    float defaultScaledWheelRadius = S(this, WHEEL_R); float availableWheelHeight = std::max(0.f, wheelCenterBottomLimit - gridAreaLimitY); std::cout << "  WHEEL VSPACE: Available Height (GridLimit to CenterBottomLimit) = " << availableWheelHeight << std::endl;
    float finalScaledWheelRadius = defaultScaledWheelRadius; float finalWheelY = 0; float absoluteMinRadius = S(this, WHEEL_R * 0.4f); absoluteMinRadius = std::max(absoluteMinRadius, scaledLetterRadius * 1.2f); std::cout << "  WHEEL MIN/MAX: Absolute Min Radius = " << absoluteMinRadius << ", Default Radius = " << defaultScaledWheelRadius << std::endl;
    float maxRadiusPossible = availableWheelHeight / 2.0f;
    if (maxRadiusPossible >= absoluteMinRadius) { finalScaledWheelRadius = std::min(maxRadiusPossible, defaultScaledWheelRadius); finalScaledWheelRadius = std::max(finalScaledWheelRadius, absoluteMinRadius); std::cout << "  WHEEL LOGIC: Space available. MaxPossible=" << maxRadiusPossible << ", FinalRadius=" << finalScaledWheelRadius << std::endl; }
    else { finalScaledWheelRadius = absoluteMinRadius; std::cout << "  WHEEL LOGIC: VERY TIGHT SPACE. Using absolute min radius: " << finalScaledWheelRadius << std::endl; }
    finalWheelY = gridAreaLimitY + availableWheelHeight / 2.f; float minYPos = gridAreaLimitY + finalScaledWheelRadius; float maxYPos = wheelCenterBottomLimit - finalScaledWheelRadius; std::cout << "  WHEEL Y CLAMP: Initial Center Y = " << finalWheelY << ", Min Allowed Y = " << minYPos << ", Max Allowed Y = " << maxYPos << std::endl;
    if (minYPos > maxYPos) { std::cout << "  WHEEL Y CLAMP WARNING: Min Y > Max Y. Prioritizing grid gap." << std::endl; finalWheelY = minYPos; }
    else { finalWheelY = std::clamp(finalWheelY, minYPos, maxYPos); }
    m_wheelX = designCenter.x; m_wheelY = finalWheelY; m_currentWheelRadius = finalScaledWheelRadius;
    std::cout << "  WHEEL FINAL: Assigned m_wheelX = " << m_wheelX << ", m_wheelY = " << m_wheelY << ", m_currentWheelRadius = " << m_currentWheelRadius << std::endl; std::cout << "--- WHEEL LAYOUT: End Calculation ---" << std::endl;


    // 6. Calculate Final Wheel Letter Positions (Uses Base UI Scale)-----
    m_wheelCentres.resize(m_base.size()); if (!m_base.empty()) { float angleStep = (2.f * PI) / (float)m_base.size(); for (size_t i = 0; i < m_base.size(); ++i) { float ang = (float)i * angleStep - PI / 2.f; m_wheelCentres[i] = { m_wheelX + m_currentWheelRadius * std::cos(ang), m_wheelY + m_currentWheelRadius * std::sin(ang) }; } }
    m_wheelLetterRenderPos.resize(m_base.size()); const float baseWheelRadius = S(this, WHEEL_R); const float scaledWheelPadding = S(this, 30.f); const float visualBgRadius = m_currentWheelRadius + scaledWheelPadding;
    float wheelRadiusRatio = 1.0f; if (baseWheelRadius > 1.0f && m_currentWheelRadius > 0.0f) { wheelRadiusRatio = m_currentWheelRadius / baseWheelRadius; } wheelRadiusRatio = std::clamp(wheelRadiusRatio, 0.7f, 1.0f);
    m_currentLetterRenderRadius = S(this, LETTER_R) * wheelRadiusRatio; const float letterPositionGap = S(this, 5.f); float letterPositionRadius = visualBgRadius - m_currentLetterRenderRadius - letterPositionGap; letterPositionRadius = std::max(letterPositionRadius, m_currentWheelRadius * 0.5f); letterPositionRadius = std::max(letterPositionRadius, m_currentLetterRenderRadius);
    std::cout << "  LAYOUT INFO: LetterRenderRadius=" << m_currentLetterRenderRadius << ", LetterPositionRadius=" << letterPositionRadius << std::endl;
    if (!m_base.empty()) { float angleStep = (2.f * PI) / (float)m_base.size(); for (size_t i = 0; i < m_base.size(); ++i) { float ang = (float)i * angleStep - PI / 2.f; m_wheelLetterRenderPos[i] = { m_wheelX + letterPositionRadius * std::cos(ang), m_wheelY + letterPositionRadius * std::sin(ang) }; } }


    // 7. Other UI Element Positions (Uses Base UI Scale)---------------
    if (m_scrambleSpr && m_scrambleTex.getSize().y > 0) { float h = S(this, SCRAMBLE_BTN_HEIGHT); float s = h / m_scrambleTex.getSize().y; m_scrambleSpr->setScale({ s,s }); m_scrambleSpr->setOrigin({ 0.f,m_scrambleTex.getSize().y / 2.f }); m_scrambleSpr->setPosition({ m_wheelX + visualBgRadius + S(this,SCRAMBLE_BTN_OFFSET_X), m_wheelY + S(this,SCRAMBLE_BTN_OFFSET_Y) }); }
    if (m_hintSpr && m_hintTex.getSize().y > 0) { float h = S(this, HINT_BTN_HEIGHT); float s = h / m_hintTex.getSize().y; m_hintSpr->setScale({ s,s }); m_hintSpr->setOrigin({ (float)m_hintTex.getSize().x,m_hintTex.getSize().y / 2.f }); m_hintSpr->setPosition({ m_wheelX - visualBgRadius - S(this,HINT_BTN_OFFSET_X), m_wheelY + S(this,HINT_BTN_OFFSET_Y) }); }
    if (m_hintCountTxt) { const unsigned int bf = 20; unsigned int sf = (unsigned int)std::max(8.0f, S(this, (float)bf)); m_hintCountTxt->setCharacterSize(sf); }
    if (m_contTxt && m_contBtn.getPointCount() > 0) { sf::Vector2f s = { S(this,200.f),S(this,50.f) }; m_contBtn.setSize(s); m_contBtn.setRadius(S(this, 10.f)); m_contBtn.setOrigin({ s.x / 2.f, 0.f }); m_contBtn.setPosition({ m_wheelX, m_wheelY + visualBgRadius + S(this,CONTINUE_BTN_OFFSET_Y) }); const unsigned int bf = 24; unsigned int sf = (unsigned int)std::max(10.0f, S(this, (float)bf)); m_contTxt->setCharacterSize(sf); sf::FloatRect tb = m_contTxt->getLocalBounds(); m_contTxt->setOrigin({ tb.position.x + tb.size.x / 2.f, tb.position.y + tb.size.y / 2.f }); m_contTxt->setPosition(m_contBtn.getPosition() + sf::Vector2f{ 0.f, s.y / 2.f }); }
    if (m_guessDisplay_Text) { const unsigned int bf = 30; unsigned int sf = (unsigned int)std::max(10.0f, S(this, (float)bf)); m_guessDisplay_Text->setCharacterSize(sf); }
    if (m_guessDisplay_Bg.getPointCount() > 0) { m_guessDisplay_Bg.setRadius(S(this, 8.f)); m_guessDisplay_Bg.setOutlineThickness(S(this, 1.f)); }
    const float scaledHudOffsetY = S(this, HUD_TEXT_OFFSET_Y); float calculatedHudStartY = m_wheelY + visualBgRadius + scaledHudOffsetY; float visualWheelTopEdgeY = m_wheelY - visualBgRadius;
    std::cout << "  WHEEL/HUD INFO: Visual Wheel BG Top Edge Y = " << visualWheelTopEdgeY << std::endl; std::cout << "  WHEEL/HUD INFO: Calculated HUD Start Y = " << calculatedHudStartY << std::endl;
    if (visualWheelTopEdgeY < gridActualBottomY - 0.1f) { std::cout << "  WHEEL/HUD WARNING: Visual Wheel BG (Y=" << visualWheelTopEdgeY << ") overlaps Grid Bottom (Y=" << gridActualBottomY << ")!" << std::endl; }
    if (calculatedHudStartY > designBottomEdge + 0.1f) { std::cout << "  WHEEL/HUD WARNING: Calculated HUD Start Y (" << calculatedHudStartY << ") is below Design Bottom Edge (" << designBottomEdge << ")" << std::endl; }

    // Inside Game::m_updateLayout()

// --- Position Return to Menu Button (Bottom Right) ---

    const float returnBtnPadding = S(this, 10.f); // Padding from corner
    sf::Vector2f returnBtnSize = { S(this, 80.f), S(this, 30.f) }; // Scaled size
    float returnBtnRadius = S(this, 8.f);
    unsigned int returnBtnFontSize = static_cast<unsigned int>(std::max(8.0f, S(this, 20.f)));

    m_returnToMenuButtonShape.setSize(returnBtnSize);
    m_returnToMenuButtonShape.setRadius(returnBtnRadius);
    m_returnToMenuButtonShape.setOrigin(returnBtnSize); // Origin at bottom-right corner of the button
    m_returnToMenuButtonShape.setPosition({ designW - returnBtnPadding, designH - returnBtnPadding }); // Position relative to design bottom-right

    if (m_returnToMenuButtonText) {
        m_returnToMenuButtonText->setCharacterSize(returnBtnFontSize);
        sf::FloatRect txtBounds = m_returnToMenuButtonText->getLocalBounds();
        // Center text origin
        m_returnToMenuButtonText->setOrigin({ txtBounds.position.x + txtBounds.size.x / 2.f,
                                               txtBounds.position.y + txtBounds.size.y / 2.f });
        // Position text in the center of the button
        sf::Vector2f btnCenter = m_returnToMenuButtonShape.getPosition() - returnBtnSize / 2.f;
        m_returnToMenuButtonText->setPosition(btnCenter);
    }

    // 8. Menu Layouts (Uses Base UI Scale) --------
    sf::Vector2f windowCenterPix = sf::Vector2f(windowSize) / 2.f; sf::Vector2f mappedWindowCenter = m_window.mapPixelToCoords(sf::Vector2i(windowCenterPix)); const float scaledMenuPadding = S(this, 40.f); const float scaledButtonSpacing = S(this, 20.f); const unsigned int scaledTitleSize = (unsigned int)std::max(12.0f, S(this, 36.f)); const unsigned int scaledButtonFontSize = (unsigned int)std::max(10.0f, S(this, 24.f)); const sf::Vector2f scaledButtonSize = { S(this,250.f),S(this,50.f) }; const float scaledButtonRadius = S(this, 10.f); const float scaledMenuRadius = S(this, 15.f); auto centerTextOnButton = [&](const std::unique_ptr<sf::Text>& textPtr, const RoundedRectangleShape& button) { if (!textPtr) return; sf::Text* text = textPtr.get(); sf::FloatRect tb = text->getLocalBounds(); text->setOrigin({ tb.position.x + tb.size.x / 2.f,tb.position.y + tb.size.y / 2.f }); text->setPosition(button.getPosition() + sf::Vector2f{ 0.f,button.getSize().y / 2.f }); };
    if (m_mainMenuTitle && m_casualButtonShape.getPointCount() > 0) { m_mainMenuTitle->setCharacterSize(scaledTitleSize); m_casualButtonText->setCharacterSize(scaledButtonFontSize); m_competitiveButtonText->setCharacterSize(scaledButtonFontSize); m_quitButtonText->setCharacterSize(scaledButtonFontSize); m_casualButtonShape.setSize(scaledButtonSize); m_competitiveButtonShape.setSize(scaledButtonSize); m_quitButtonShape.setSize(scaledButtonSize); m_casualButtonShape.setRadius(scaledButtonRadius); m_competitiveButtonShape.setRadius(scaledButtonRadius); m_quitButtonShape.setRadius(scaledButtonRadius); sf::FloatRect titleBounds = m_mainMenuTitle->getLocalBounds(); float sths = titleBounds.size.y + titleBounds.position.y + scaledButtonSpacing; float tbh = 3 * scaledButtonSize.y + 2 * scaledButtonSpacing; float smmh = scaledMenuPadding + sths + tbh + scaledMenuPadding; float smmw = std::max(scaledButtonSize.x, titleBounds.size.x + titleBounds.position.x) + 2 * scaledMenuPadding; m_mainMenuBg.setSize({ smmw,smmh }); m_mainMenuBg.setRadius(scaledMenuRadius); m_mainMenuBg.setOrigin({ smmw / 2.f,smmh / 2.f }); m_mainMenuBg.setPosition(mappedWindowCenter); sf::Vector2f mbp = m_mainMenuBg.getPosition(); float mty = mbp.y - smmh / 2.f; m_mainMenuTitle->setOrigin({ titleBounds.position.x + titleBounds.size.x / 2.f,titleBounds.position.y }); m_mainMenuTitle->setPosition({ mbp.x,mty + scaledMenuPadding }); float currentY = mty + scaledMenuPadding + sths; m_casualButtonShape.setOrigin({ scaledButtonSize.x / 2.f,0.f }); m_casualButtonShape.setPosition({ mbp.x,currentY }); centerTextOnButton(m_casualButtonText, m_casualButtonShape); currentY += scaledButtonSize.y + scaledButtonSpacing; m_competitiveButtonShape.setOrigin({ scaledButtonSize.x / 2.f,0.f }); m_competitiveButtonShape.setPosition({ mbp.x,currentY }); centerTextOnButton(m_competitiveButtonText, m_competitiveButtonShape); currentY += scaledButtonSize.y + scaledButtonSpacing; m_quitButtonShape.setOrigin({ scaledButtonSize.x / 2.f,0.f }); m_quitButtonShape.setPosition({ mbp.x,currentY }); centerTextOnButton(m_quitButtonText, m_quitButtonShape); }
    if (m_casualMenuTitle && m_easyButtonShape.getPointCount() > 0) { m_casualMenuTitle->setCharacterSize(scaledTitleSize); m_easyButtonText->setCharacterSize(scaledButtonFontSize); m_mediumButtonText->setCharacterSize(scaledButtonFontSize); m_hardButtonText->setCharacterSize(scaledButtonFontSize); m_returnButtonText->setCharacterSize(scaledButtonFontSize); m_easyButtonShape.setSize(scaledButtonSize); m_mediumButtonShape.setSize(scaledButtonSize); m_hardButtonShape.setSize(scaledButtonSize); m_returnButtonShape.setSize(scaledButtonSize); m_easyButtonShape.setRadius(scaledButtonRadius); m_mediumButtonShape.setRadius(scaledButtonRadius); m_hardButtonShape.setRadius(scaledButtonRadius); m_returnButtonShape.setRadius(scaledButtonRadius); sf::FloatRect ctb = m_casualMenuTitle->getLocalBounds(); float sths = ctb.size.y + ctb.position.y + scaledButtonSpacing; float tbh = 4 * scaledButtonSize.y + 3 * scaledButtonSpacing; float scmh = scaledMenuPadding + sths + tbh + scaledMenuPadding; float scmw = std::max(scaledButtonSize.x, ctb.size.x + ctb.position.x) + 2 * scaledMenuPadding; m_casualMenuBg.setSize({ scmw,scmh }); m_casualMenuBg.setRadius(scaledMenuRadius); m_casualMenuBg.setOrigin({ scmw / 2.f,scmh / 2.f }); m_casualMenuBg.setPosition(mappedWindowCenter); sf::Vector2f cmbp = m_casualMenuBg.getPosition(); float cmty = cmbp.y - scmh / 2.f; m_casualMenuTitle->setOrigin({ ctb.position.x + ctb.size.x / 2.f,ctb.position.y }); m_casualMenuTitle->setPosition({ cmbp.x,cmty + scaledMenuPadding }); float ccy = cmty + scaledMenuPadding + sths; m_easyButtonShape.setOrigin({ scaledButtonSize.x / 2.f,0.f }); m_easyButtonShape.setPosition({ cmbp.x,ccy }); centerTextOnButton(m_easyButtonText, m_easyButtonShape); ccy += scaledButtonSize.y + scaledButtonSpacing; m_mediumButtonShape.setOrigin({ scaledButtonSize.x / 2.f,0.f }); m_mediumButtonShape.setPosition({ cmbp.x,ccy }); centerTextOnButton(m_mediumButtonText, m_mediumButtonShape); ccy += scaledButtonSize.y + scaledButtonSpacing; m_hardButtonShape.setOrigin({ scaledButtonSize.x / 2.f,0.f }); m_hardButtonShape.setPosition({ cmbp.x,ccy }); centerTextOnButton(m_hardButtonText, m_hardButtonShape); ccy += scaledButtonSize.y + scaledButtonSpacing; m_returnButtonShape.setOrigin({ scaledButtonSize.x / 2.f,0.f }); m_returnButtonShape.setPosition({ cmbp.x,ccy }); centerTextOnButton(m_returnButtonText, m_returnButtonShape); }


    // --- Final Summary Log ---
    std::cout << "--- Overall Layout Summary (Design Coords) ---" << std::endl;
    std::cout << "Grid: StartY=" << m_gridStartY << " ActualH=" << actualGridHeight << " BottomY=" << gridActualBottomY << " StartX=" << m_gridStartX << " Width=" << m_totalGridW << " (Using Scale Mod: " << m_currentGridLayoutScale << ")" << std::endl;
    std::cout << "Wheel: CenterY=" << m_wheelY << " Radius=" << m_currentWheelRadius << " VisualTopY=" << visualWheelTopEdgeY << std::endl;
    std::cout << "HUD: StartY=" << calculatedHudStartY << std::endl;
    std::cout << "---------------------------------------------" << std::endl;

}
// ***** END OF COMPLETE Game::m_updateLayout FUNCTION *****


sf::Vector2f Game::m_tilePos(int wordIdx, int charIdx) {
    sf::Vector2f result = { -1000.f, -1000.f };

    // --- Bounds checks ---
    if (m_sorted.empty() || wordIdx < 0 || wordIdx >= m_wordCol.size() || wordIdx >= m_wordRow.size() || charIdx < 0) {
        return result;
    }
    int c = m_wordCol[wordIdx];
    int r = m_wordRow[wordIdx];
    if (c < 0 || c >= m_colXOffset.size()) {
        return result;
    }

    // --- Calculate Tile/Padding/Step Sizes using Grid-Specific Scale Factors ---

    // Determine the scale factor for the TILE itself based on whether the grid was shrunk
    float tileSpecificScaleFactor = (m_currentGridLayoutScale < 1.0f) ? GRID_TILE_RELATIVE_SCALE_WHEN_SHRUNK : 1.0f;

    // Apply appropriate scaling
    const float scaledTileSize = S(this, TILE_SIZE) * tileSpecificScaleFactor;   // Tile size uses relative factor
    const float scaledTilePad = S(this, TILE_PAD) * m_currentGridLayoutScale; // Padding uses full grid shrink factor

    // Calculate final step width and height
    const float scaledStepWidth = scaledTileSize + scaledTilePad;
    const float scaledStepHeight = scaledTileSize + scaledTilePad; // Assuming square tiles/padding

    // --- Use pre-calculated adjusted offsets and the adjusted step width/height ---
    // m_colXOffset[c] and m_gridStartY were calculated in m_updateLayout using the same logic
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
            sf::Vector2f mp = m_window.mapPixelToCoords(mb->position);

            // Check Return to Menu Button
            if (m_returnToMenuButtonShape.getGlobalBounds().contains(mp)) {
                if (m_clickSound) m_clickSound->play();
                m_currentScreen = GameScreen::MainMenu;
                // Optional: Stop music, reset session state if needed?
                m_backgroundMusic.stop(); // Example
                m_isInSession = false;    // Example
                m_selectedDifficulty = DifficultyLevel::None; // Example
                m_clearDragState(); // Clear guess if returning from playing
                return; // Processed button click
            }

            // Check Scramble Button
            if (m_scrambleSpr && m_scrambleSpr->getGlobalBounds().contains(mp)) {
                if (m_clickSound) m_clickSound->play();
                std::shuffle(m_base.begin(), m_base.end(), Rng());
                m_updateLayout(m_window.getSize()); // Update wheel letter positions graphically
                m_clearDragState(); // Stop any current drag
                return; // Processed button click
            }

            // Check Hint Button
            if (m_hintSpr && m_hintSpr->getGlobalBounds().contains(mp) && m_hintsAvailable > 0) {
                if (m_hintUsedSound) m_hintUsedSound->play();
                m_hintsAvailable--;
                m_wordsSolvedSinceHint = 0; // Reset counter for earning hints
                if (m_hintCountTxt) m_hintCountTxt->setString("Hints: " + std::to_string(m_hintsAvailable)); // Update display immediately

                // --- Hint Logic: Find first blank in first unsolved word ---
                int targetWordIdx = -1;
                int targetCharIdx = -1;
                char targetChar = '_';
                bool foundSpot = false;

                for (std::size_t w = 0; w < m_grid.size() && w < m_sorted.size(); ++w) {
                    const std::string& solutionWord = m_sorted[w].text;
                    bool wordFound = m_found.count(solutionWord); // Check if already found

                    if (!wordFound) { // Only hint for unsolved words
                        for (std::size_t c = 0; c < m_grid[w].size(); ++c) {
                            if (m_grid[w][c] == '_') { // Find the first blank space
                                targetWordIdx = static_cast<int>(w);
                                targetCharIdx = static_cast<int>(c);
                                if (targetCharIdx < solutionWord.length()) {
                                    targetChar = solutionWord[targetCharIdx];
                                }
                                foundSpot = true;
                                break; // Found the blank, stop inner loop
                            }
                        }
                    }
                    if (foundSpot) break; // Found the blank, stop outer loop
                }
                // --- End Hint Logic ---


                if (targetWordIdx != -1 && targetCharIdx != -1 && targetChar != '_') {
                    sf::Vector2f hintEndPos = m_tilePos(targetWordIdx, targetCharIdx);
                    hintEndPos.x += TILE_SIZE / 2.f; // Center of the tile
                    hintEndPos.y += TILE_SIZE / 2.f;

                    // Find the letter on the wheel (assume base contains unique letters)
                    sf::Vector2f hintStartPos = { m_wheelX, m_wheelY }; // Default start
                    for (size_t i = 0; i < m_base.length(); ++i) {
                        // Case-insensitive comparison
                        if (std::toupper(m_base[i]) == std::toupper(targetChar)) {
                            if (i < m_wheelCentres.size()) {
                                hintStartPos = m_wheelCentres[i];
                            }
                            break;
                        }
                    }

                    // Create animation for the hint letter
                    m_anims.push_back({
                        static_cast<char>(std::toupper(targetChar)), 
                        hintStartPos,                                
                        hintEndPos,                                  
                        0.f,                                         
                        targetWordIdx,                               
                        targetCharIdx                                
                        });
                }
                return; // Processed hint button click
            } // End Hint Button Check


           // --- Check Letter Wheel Click ---
            for (std::size_t i = 0; i < m_wheelCentres.size(); ++i) {
                if (distSq(mp, m_wheelLetterRenderPos[i]) < m_currentLetterRenderRadius * m_currentLetterRenderRadius) {
                    m_dragging = true;
                    m_path.clear();
                    m_path.push_back(static_cast<int>(i));
                    m_currentGuess += static_cast<char>(std::toupper(m_base[i]));
                    if (m_selectSound) m_selectSound->play();
                    return; // Started drag
                }
            }
            // --- End Letter Wheel Click ---

        } // End Left Mouse Button Check
    } // End Mouse Button Pressed Check


    // --- Mouse Moved ---
    else if (const auto* mm = event.getIf<sf::Event::MouseMoved>()) {
        if (m_dragging) {
            sf::Vector2f mp = m_window.mapPixelToCoords(mm->position);
            bool actionTaken = false; // Flag to prevent multiple adds/removes per event

            for (std::size_t i = 0; i < m_wheelCentres.size(); ++i) {
                if (actionTaken) break; // Only process the first letter hovered over this frame

                // Check if mouse is inside the circle for letter 'i'
                if (distSq(mp, m_wheelLetterRenderPos[i]) < m_currentLetterRenderRadius * m_currentLetterRenderRadius) {

                    int currentLetterIdx = static_cast<int>(i);

                    // Check if letter 'i' is already in the path
                    auto it = std::find(m_path.begin(), m_path.end(), currentLetterIdx);
                    bool alreadyInPath = (it != m_path.end());

                    if (!alreadyInPath) {
                        // --- Add new letter to path ---
                        m_path.push_back(currentLetterIdx);
                        m_currentGuess += static_cast<char>(std::toupper(m_base[i]));
                        if (m_selectSound) m_selectSound->play();
                        actionTaken = true; // Mark that we added a letter

                    }
                    else {
                        // --- Letter is already in path - Check for backtracking ---
                        // Condition: Path has at least 2 letters AND
                        //            we are hovering over the second-to-last letter added
                        if (m_path.size() >= 2 && m_path[m_path.size() - 2] == currentLetterIdx)
                        {
                            // Remove the *last* element from path and guess
                            m_path.pop_back();
                            if (!m_currentGuess.empty()) { // Safety check
                                m_currentGuess.pop_back();
                            }
                            // Optional: Play an "unselect" or "pop" sound?
                            // if (m_unselectSound) m_unselectSound->play();
                            actionTaken = true; // Mark that we removed a letter
                        }
                        // Else: Hovering over the current last letter, or some other
                        //       letter already in the path (but not the second-to-last)
                        //       -> Do nothing.
                    }
                } // End if mouse is over letter 'i'
            } // End for loop checking wheel letters
        } // End if m_dragging
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
                        m_currentScore += baseScore + rarityBonus;
                        if (m_scoreValueText) m_scoreValueText->setString(std::to_string(m_currentScore));
                        m_wordsSolvedSinceHint++;
                        if (m_wordsSolvedSinceHint >= WORDS_PER_HINT) {
                            m_hintsAvailable++; m_wordsSolvedSinceHint = 0;
                            if (m_hintCountTxt) m_hintCountTxt->setString("Hints: " + std::to_string(m_hintsAvailable));
                        }
                        for (std::size_t c = 0; c < m_currentGuess.length(); ++c) { // Animation Logic
                            if (c < m_path.size()) {
                                int pathNodeIdx = m_path[c];
                                if (pathNodeIdx >= 0 && pathNodeIdx < m_wheelCentres.size()) {
                                    sf::Vector2f startPos = m_wheelCentres[pathNodeIdx];
                                    sf::Vector2f endPos = m_tilePos(wordIndexMatched, static_cast<int>(c));
                                    endPos.x += TILE_SIZE / 2.f; endPos.y += TILE_SIZE / 2.f;
                                    m_anims.push_back({ m_currentGuess[c], startPos, endPos, 0.f - (c * 0.03f), wordIndexMatched, static_cast<int>(c), AnimTarget::Grid });
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
                // Skip if this potential bonus word is actually required for the grid
                if (m_found.count(bonusWordOriginalCase)) { continue; } // Already handled by grid check

                std::string bonusWordUpper = bonusWordOriginalCase;
                std::transform(bonusWordUpper.begin(), bonusWordUpper.end(), bonusWordUpper.begin(), ::toupper);

                if (bonusWordUpper == m_currentGuess) { // Text matches a potential bonus word
                    wordMatched = bonusWordOriginalCase; // Store match

                    if (m_foundBonusWords.count(bonusWordOriginalCase)) {
                        // --- Repeated BONUS Word ---
                        std::cout << "DEBUG: Matched BONUS word '" << bonusWordOriginalCase << "', but already found AS BONUS." << std::endl;
                        m_bonusTextFlourishTimer = BONUS_TEXT_FLOURISH_DURATION; // Trigger bonus text flourish
                        if (m_placeSound) m_placeSound->play(); // Use error/repeat sound
                        actionTaken = true;
                    }
                    else {
                        // --- NEW Bonus Word Found ---
                        std::cout << "DEBUG: Found NEW match for BONUS: '" << bonusWordOriginalCase << "'" << std::endl;
                        m_foundBonusWords.insert(bonusWordOriginalCase);
                        // ... (Bonus Scoring, Letter Animations to Score - keep existing logic) ...
                        int bonusScore = 25; m_currentScore += bonusScore;
                        if (m_scoreValueText) m_scoreValueText->setString(std::to_string(m_currentScore));
                        std::cout << "BONUS Word: " << m_currentGuess << " | Points: " << bonusScore << " | Total: " << m_currentScore << std::endl;
                        if (m_scoreValueText) { // Animation Logic
                            sf::FloatRect scoreBounds = m_scoreValueText->getGlobalBounds();
                            sf::Vector2f scoreCenterPos = { scoreBounds.position.x + scoreBounds.size.x / 2.f, scoreBounds.position.y + scoreBounds.size.y / 2.f };
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
                        else { std::cerr << "Warning: Cannot animate bonus to score - m_scoreValueText is null." << std::endl; }
                        // ... (End of new bonus word logic) ...
                        actionTaken = true;
                    }
                    goto process_outcome; // Found a bonus match (new or repeat)
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
    // Note: Grid-specific scaling is applied *selectively* below
    const float scaledLetterRadius = S(this, LETTER_R);
    const float scaledWheelOutlineThickness = S(this, 3.f);
    const float scaledLetterCircleOutline = S(this, 2.f);
    const float scaledPathThickness = S(this, 5.0f);
    const float scaledGuessDisplayGap = S(this, GUESS_DISPLAY_GAP);
    const float scaledGuessDisplayPadX = S(this, 15.f);
    const float scaledGuessDisplayPadY = S(this, 5.f);
    const float scaledGuessDisplayRadius = S(this, 8.f);
    const float scaledGuessDisplayOutline = S(this, 1.f);
    const float scaledHudOffsetY = S(this, HUD_TEXT_OFFSET_Y);
    const float scaledHudLineSpacing = S(this, HUD_LINE_SPACING);
    const float scaledHintOffsetX = S(this, 10.f);
    const float scaledHintOffsetY = S(this, 5.f);

    // Scaled Font Sizes (ensure minimum usable size)
    const unsigned int scaledGridLetterFontSize = static_cast<unsigned int>(std::max(8.0f, S(this, 20.f)));
    const unsigned int scaledFlyingLetterFontSize = static_cast<unsigned int>(std::max(8.0f, S(this, 20.f)));
    const unsigned int scaledGuessDisplayFontSize = static_cast<unsigned int>(std::max(8.0f, S(this, 30.f)));
    const unsigned int scaledFoundFontSize = static_cast<unsigned int>(std::max(8.0f, S(this, 20.f)));
    const unsigned int scaledBonusFontSize = static_cast<unsigned int>(std::max(8.0f, S(this, 18.f)));
    const unsigned int scaledHintFontSize = static_cast<unsigned int>(std::max(8.0f, S(this, 20.f)));
    const unsigned int scaledSolvedFontSize = static_cast<unsigned int>(std::max(8.0f, S(this, 26.f)));
    const unsigned int scaledContinueFontSize = static_cast<unsigned int>(std::max(8.0f, S(this, 24.f)));


    //------------------------------------------------------------
    //  Draw Progress Meter (If in session)
    //------------------------------------------------------------
    if (m_isInSession) {
        // ... (Progress Meter drawing - NO CHANGES) ...
        m_progressMeterBg.setFillColor(sf::Color(50, 50, 50, 150)); m_progressMeterBg.setOutlineColor(sf::Color(150, 150, 150));
        m_progressMeterBg.setOutlineThickness(S(this, PROGRESS_METER_OUTLINE)); m_progressMeterFill.setFillColor(sf::Color(0, 180, 0, 200));
        float progressRatio = 0.f; if (m_puzzlesPerSession > 0) { progressRatio = static_cast<float>(m_currentPuzzleIndex + 1) / static_cast<float>(m_puzzlesPerSession); }
        float fillWidth = m_progressMeterBg.getSize().x * progressRatio; m_progressMeterFill.setSize({ fillWidth, m_progressMeterBg.getSize().y });
        m_window.draw(m_progressMeterBg); m_window.draw(m_progressMeterFill);
        if (m_progressMeterText) { const unsigned int scaledProgressFontSize = static_cast<unsigned int>(std::max(8.0f, S(this, 16.f))); m_progressMeterText->setCharacterSize(scaledProgressFontSize); std::string progressStr = std::to_string(m_currentPuzzleIndex + 1) + "/" + std::to_string(m_puzzlesPerSession); m_progressMeterText->setString(progressStr); m_progressMeterText->setFillColor(sf::Color::White); sf::FloatRect textBounds = m_progressMeterText->getLocalBounds(); m_progressMeterText->setOrigin({ textBounds.position.x + textBounds.size.x / 2.f, textBounds.position.y + textBounds.size.y / 2.f }); m_progressMeterText->setPosition(m_progressMeterBg.getPosition()); m_window.draw(*m_progressMeterText); }
    }

    // --- Draw Return to Menu Button ---
    if (m_currentScreen == GameScreen::Playing || m_currentScreen == GameScreen::GameOver) // Show only during gameplay/gameover?
    {
        bool returnHover = m_returnToMenuButtonShape.getGlobalBounds().contains(mousePos);
        sf::Color returnBaseColor = m_currentTheme.menuButtonNormal; // Use theme colors
        sf::Color returnHoverColor = m_currentTheme.menuButtonHover;
        m_returnToMenuButtonShape.setFillColor(returnHover ? returnHoverColor : returnBaseColor);
        m_window.draw(m_returnToMenuButtonShape);
        if (m_returnToMenuButtonText) {
            m_returnToMenuButtonText->setFillColor(m_currentTheme.menuButtonText); // Use theme color
            m_window.draw(*m_returnToMenuButtonText);
        }
    }

    //------------------------------------------------------------
    //  Draw Score Bar
    //------------------------------------------------------------
    // ... (Score bar drawing - NO CHANGES) ...
    m_scoreBar.setFillColor(m_currentTheme.scoreBarBg); m_scoreBar.setOutlineColor(m_currentTheme.wheelOutline); m_scoreBar.setOutlineThickness(1.f);
    m_window.draw(m_scoreBar);
    if (m_scoreLabelText) { m_scoreLabelText->setFillColor(m_currentTheme.scoreTextLabel); m_window.draw(*m_scoreLabelText); }
    if (m_scoreValueText) { if (m_scoreFlourishTimer > 0.f) { float scaleFactor = 1.0f + 0.4f * std::sin((SCORE_FLOURISH_DURATION - m_scoreFlourishTimer) / SCORE_FLOURISH_DURATION * PI); m_scoreValueText->setScale({ scaleFactor, scaleFactor }); sf::FloatRect bounds = m_scoreValueText->getLocalBounds(); m_scoreValueText->setOrigin({ 0.f + bounds.position.x, bounds.position.y + bounds.size.y / 2.f }); } else { m_scoreValueText->setScale({ 1.f, 1.f }); sf::FloatRect bounds = m_scoreValueText->getLocalBounds(); m_scoreValueText->setOrigin({ 0.f + bounds.position.x, bounds.position.y + bounds.size.y / 2.f }); } m_scoreValueText->setFillColor(m_currentTheme.scoreTextValue); m_window.draw(*m_scoreValueText); }

    //------------------------------------------------------------
    //  Draw letter grid
    //------------------------------------------------------------
    if (!m_sorted.empty()) {
        // --- Determine FINAL Tile Size/Radius for Rendering ---
        // (Matches logic in m_updateLayout 4c and m_tilePos)
        float renderTileScaleFactor = (m_currentGridLayoutScale < 1.0f) ? GRID_TILE_RELATIVE_SCALE_WHEN_SHRUNK : 1.0f;
        const float finalRenderTileSize = S(this, TILE_SIZE) * renderTileScaleFactor;
        const float finalRenderTileRadius = finalRenderTileSize * 0.18f;
        const float scaledTileOutline = S(this, 1.f); // Outline thickness can use base scale
        // ---

        // Reusable shapes for drawing tiles and letters
        RoundedRectangleShape tileBackground({ finalRenderTileSize, finalRenderTileSize }, finalRenderTileRadius, 10);
        tileBackground.setOutlineThickness(scaledTileOutline);
        sf::Text letterText(m_font, "", scaledGridLetterFontSize); // Font size uses base scale

        for (std::size_t w = 0; w < m_sorted.size(); ++w) {
            int wordRarity = m_sorted[w].rarity;
            if (w >= m_grid.size()) continue; // Bounds check for m_grid

            for (std::size_t c = 0; c < m_sorted[w].text.length(); ++c) {
                if (c >= m_grid[w].size()) continue; // Bounds check for m_grid[w]

                sf::Vector2f p = m_tilePos(static_cast<int>(w), static_cast<int>(c)); // Gets final position
                bool isFilled = (m_grid[w][c] != '_');

                // Set position for the background tile
                tileBackground.setPosition(p);

                // Configure background appearance based on filled status
                if (isFilled) {
                    tileBackground.setFillColor(m_currentTheme.gridFilledTile);
                    tileBackground.setOutlineColor(m_currentTheme.gridFilledTile);
                }
                else {
                    tileBackground.setFillColor(m_currentTheme.gridEmptyTile);
                    tileBackground.setOutlineColor(m_currentTheme.gridEmptyTile);
                }
                // Draw the background tile ONCE
                m_window.draw(tileBackground);

                // --- Draw Gem OR Letter ---
                if (!isFilled) {
                    // Draw Gem
                    sf::Sprite* gemSprite = nullptr;
                    switch (wordRarity) {
                    case 1: break; // Common words might have no gem
                    case 2: if (m_sapphireSpr) gemSprite = m_sapphireSpr.get(); break; // Uncommon - Sapphire/Emerald
                    case 3: if (m_rubySpr) gemSprite = m_rubySpr.get(); break;     // Rare - Ruby
                    case 4: if (m_diamondSpr) gemSprite = m_diamondSpr.get(); break; // Very Rare - Diamond
                    }

                    if (gemSprite != nullptr) {
                        // Center the gem within the final tile size
                        float tileCenterX = p.x + finalRenderTileSize / 2.f;
                        float tileCenterY = p.y + finalRenderTileSize / 2.f;
                        gemSprite->setPosition({ tileCenterX, tileCenterY });
                        // Note: Gem sprite scale is set once during load based on TILE_SIZE.
                        // If grid shrinks significantly, gems might appear large relative to the tile.
                        // A dynamic rescaling here based on finalRenderTileSize could be added if needed.
                        // Example:
                        // float desiredGemHeight = finalRenderTileSize * 0.60f;
                        // if (gemSprite->getTexture()) { // Check texture exists
                        //    float texHeight = gemSprite->getTexture()->getSize().y;
                        //    if (texHeight > 0) {
                        //       float gemScale = desiredGemHeight / texHeight;
                        //       gemSprite->setScale({gemScale, gemScale});
                        //     }
                        // }
                        m_window.draw(*gemSprite); // Draw the selected gem
                    }
                }
                else {
                    // Draw Letter
                    bool isAnimatingToTile = false;
                    for (const auto& anim : m_anims) { /* ... check if animating ... */ if (anim.target == AnimTarget::Grid && anim.wordIdx == w && anim.charIdx == c && anim.t < 1.0f) { isAnimatingToTile = true; break; } }

                    if (!isAnimatingToTile) {
                        // Apply Grid Flourish effect
                        float currentFlourishScale = 1.0f;
                        bool isFlourishing = false;
                        for (const auto& flourish : m_gridFlourishes) { /* ... check flourish & calculate scale ... */ if (flourish.wordIdx == w && flourish.charIdx == c) { float progress = (GRID_FLOURISH_DURATION - flourish.timer) / GRID_FLOURISH_DURATION; currentFlourishScale = 1.0f + 0.4f * std::sin(progress * PI); isFlourishing = true; break; } }

                        // Prepare letter text
                        letterText.setString(std::string(1, m_grid[w][c]));
                        letterText.setFillColor(m_currentTheme.gridLetter); // Set color
                        sf::FloatRect b = letterText.getLocalBounds();
                        letterText.setOrigin({ b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f }); // Center origin
                        letterText.setPosition(p + sf::Vector2f{ finalRenderTileSize / 2.f, finalRenderTileSize / 2.f }); // Center in tile
                        letterText.setScale({ currentFlourishScale, currentFlourishScale }); // Apply flourish scale

                        m_window.draw(letterText);

                        // Reset scale AFTER drawing if it was modified, for the next letter
                        if (!isFlourishing) { letterText.setScale({ 1.f, 1.f }); }
                    }
                } // End if/else for gem/letter
            } // End char loop
        } // End word loop
    } // End grid drawing


    //------------------------------------------------------------
    //  Draw wheel background & letters (Uses Base UI Scale)
    //------------------------------------------------------------
    // ... (Wheel BG drawing - NO CHANGES) ...
    const float currentWheelRadius = m_currentWheelRadius; const float scaledWheelPadding = S(this, 30.f);
    m_wheelBg.setRadius(currentWheelRadius + scaledWheelPadding); m_wheelBg.setFillColor(m_currentTheme.wheelBg); m_wheelBg.setOutlineColor(m_currentTheme.wheelOutline); m_wheelBg.setOutlineThickness(scaledWheelOutlineThickness); m_wheelBg.setOrigin({ m_wheelBg.getRadius(), m_wheelBg.getRadius() }); m_wheelBg.setPosition({ m_wheelX, m_wheelY });
    m_window.draw(m_wheelBg);

    // --- DRAW FLYING LETTER ANIMATIONS ---
    // ... (Flying letter drawing - NO CHANGES) ...
    sf::Text flyingLetterText(m_font, "", scaledFlyingLetterFontSize); sf::Color flyColorBase = m_currentTheme.gridLetter;
    for (const auto& a : m_anims) { sf::Color currentFlyColor = flyColorBase; if (a.target == AnimTarget::Score) { currentFlyColor = sf::Color::Yellow; } float alpha_ratio = 1.0f; if (a.t > 0.7f) { alpha_ratio = (1.0f - a.t) / 0.3f; alpha_ratio = std::max(0.0f, std::min(1.0f, alpha_ratio)); } currentFlyColor.a = static_cast<std::uint8_t>(255.f * alpha_ratio); flyingLetterText.setFillColor(currentFlyColor); float eased_t = a.t * a.t * (3.f - 2.f * a.t); sf::Vector2f p = a.start + (a.end - a.start) * eased_t; flyingLetterText.setString(std::string(1, a.ch)); sf::FloatRect bounds = flyingLetterText.getLocalBounds(); flyingLetterText.setOrigin({ bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f }); flyingLetterText.setPosition(p); m_window.draw(flyingLetterText); }


    //------------------------------------------------------------
    //  Draw Path lines (BEFORE Wheel Letters) (Uses Base UI Scale)
    //------------------------------------------------------------
    // ... (Path line drawing - NO CHANGES) ...
    if (m_dragging && !m_path.empty()) { const float halfThickness = scaledPathThickness / 2.0f; const sf::PrimitiveType stripType = sf::PrimitiveType::TriangleStrip; const sf::Color pathColor = m_currentTheme.dragLine; if (m_path.size() >= 2) { sf::VertexArray finalPathStrip(stripType, (m_path.size() - 1) * 4); size_t currentVertex = 0; for (size_t i = 0; i < m_path.size() - 1; ++i) { int idx1 = m_path[i]; int idx2 = m_path[i + 1]; if (idx1 < 0 || idx1 >= m_wheelCentres.size() || idx2 < 0 || idx2 >= m_wheelCentres.size()) { continue; } sf::Vector2f p1 = m_wheelCentres[idx1]; sf::Vector2f p2 = m_wheelCentres[idx2]; sf::Vector2f direction = p2 - p1; float length = std::sqrt(direction.x * direction.x + direction.y * direction.y); if (length < 0.1f) continue; sf::Vector2f unitDirection = direction / length; sf::Vector2f unitPerpendicular = { -unitDirection.y, unitDirection.x }; sf::Vector2f offset = unitPerpendicular * halfThickness; if (currentVertex + 3 < finalPathStrip.getVertexCount()) { finalPathStrip[currentVertex].position = p1 - offset; finalPathStrip[currentVertex].color = pathColor; currentVertex++; finalPathStrip[currentVertex].position = p1 + offset; finalPathStrip[currentVertex].color = pathColor; currentVertex++; finalPathStrip[currentVertex].position = p2 - offset; finalPathStrip[currentVertex].color = pathColor; currentVertex++; finalPathStrip[currentVertex].position = p2 + offset; finalPathStrip[currentVertex].color = pathColor; currentVertex++; } else { std::cerr << "VertexArray index out of bounds!" << std::endl; break; } } finalPathStrip.resize(currentVertex); if (finalPathStrip.getVertexCount() > 0) { m_window.draw(finalPathStrip); } } if (!m_path.empty()) { int lastIdx = m_path.back(); if (lastIdx >= 0 && lastIdx < m_wheelCentres.size()) { sf::Vector2f p1 = m_wheelCentres[lastIdx]; sf::Vector2f p2 = mousePos; sf::Vector2f direction = p2 - p1; float length = std::sqrt(direction.x * direction.x + direction.y * direction.y); if (length > 0.1f) { sf::Vector2f unitDirection = direction / length; sf::Vector2f unitPerpendicular = { -unitDirection.y, unitDirection.x }; sf::Vector2f offset = unitPerpendicular * halfThickness; sf::VertexArray rubberBandStrip(stripType, 4); rubberBandStrip[0].position = p1 - offset; rubberBandStrip[1].position = p1 + offset; rubberBandStrip[2].position = p2 - offset; rubberBandStrip[3].position = p2 + offset; rubberBandStrip[0].color = pathColor; rubberBandStrip[1].color = pathColor; rubberBandStrip[2].color = pathColor; rubberBandStrip[3].color = pathColor; m_window.draw(rubberBandStrip); } } } }


    // --- START: Draw Guess Display (Uses Base UI Scale) ---
    // ... (Guess display drawing - NO CHANGES) ...
    if (m_gameState == GState::Playing && !m_currentGuess.empty() && m_guessDisplay_Text && m_guessDisplay_Bg.getPointCount() > 0) { m_guessDisplay_Text->setCharacterSize(scaledGuessDisplayFontSize); m_guessDisplay_Text->setString(m_currentGuess); sf::FloatRect textBounds = m_guessDisplay_Text->getLocalBounds(); sf::Vector2f bgSize = { textBounds.position.x + textBounds.size.x + 2 * scaledGuessDisplayPadX, textBounds.position.y + textBounds.size.y + 2 * scaledGuessDisplayPadY }; float guessY = m_wheelY - (currentWheelRadius + scaledWheelPadding) - (bgSize.y / 2.f) - scaledGuessDisplayGap; m_guessDisplay_Bg.setSize(bgSize); m_guessDisplay_Bg.setRadius(scaledGuessDisplayRadius); m_guessDisplay_Bg.setFillColor(m_currentTheme.gridFilledTile); m_guessDisplay_Bg.setOutlineColor(sf::Color(150, 150, 150, 200)); m_guessDisplay_Bg.setOutlineThickness(scaledGuessDisplayOutline); m_guessDisplay_Bg.setOrigin({ bgSize.x / 2.f, bgSize.y / 2.f }); m_guessDisplay_Bg.setPosition({ m_wheelX, guessY }); m_guessDisplay_Text->setOrigin({ textBounds.position.x + textBounds.size.x / 2.f, textBounds.position.y + textBounds.size.y / 2.f }); m_guessDisplay_Text->setPosition({ m_wheelX, guessY }); m_guessDisplay_Text->setFillColor(m_currentTheme.gridLetter); m_window.draw(m_guessDisplay_Bg); m_window.draw(*m_guessDisplay_Text); }


    // --- Draw Wheel Letters (Uses Base UI Scale) ---
    // ... (Wheel letter drawing - NO CHANGES) ...
    const float baseWheelRadius = S(this, WHEEL_R); float wheelRadiusRatio = 1.0f; if (baseWheelRadius > 1.0f && m_currentWheelRadius > 0.0f) { wheelRadiusRatio = m_currentWheelRadius / baseWheelRadius; } wheelRadiusRatio = std::clamp(wheelRadiusRatio, 0.7f, 1.0f); const unsigned int scaledWheelLetterFontSize = static_cast<unsigned int>(std::max(8.0f, S(this, 25.f) * wheelRadiusRatio)); const float letterCircleRadius = m_currentLetterRenderRadius;
    for (std::size_t i = 0; i < m_base.size(); ++i) { if (i >= m_wheelLetterRenderPos.size()) continue; bool isHilited = std::find(m_path.begin(), m_path.end(), static_cast<int>(i)) != m_path.end(); sf::Vector2f renderPos = m_wheelLetterRenderPos[i]; sf::CircleShape letterCircle(letterCircleRadius); letterCircle.setOrigin({ letterCircleRadius, letterCircleRadius }); letterCircle.setPosition(renderPos); letterCircle.setFillColor(isHilited ? m_currentTheme.wheelOutline : m_currentTheme.letterCircleNormal); letterCircle.setOutlineColor(m_currentTheme.wheelOutline); letterCircle.setOutlineThickness(scaledLetterCircleOutline); m_window.draw(letterCircle); sf::Text chTxt(m_font, std::string(1, static_cast<char>(std::toupper(m_base[i]))), scaledWheelLetterFontSize); chTxt.setFillColor(isHilited ? m_currentTheme.letterTextHighlight : m_currentTheme.letterTextNormal); sf::FloatRect txtBounds = chTxt.getLocalBounds(); chTxt.setOrigin({ txtBounds.position.x + txtBounds.size.x / 2.f, txtBounds.position.y + txtBounds.size.y / 2.f }); chTxt.setPosition(renderPos); m_window.draw(chTxt); }


    //------------------------------------------------------------
    //  Draw UI Buttons / Hover (Uses Base UI Scale)
    //------------------------------------------------------------
    // ... (Scramble/Hint button drawing - NO CHANGES) ...
    if (m_gameState == GState::Playing) { if (m_scrambleSpr) { bool scrambleHover = m_scrambleSpr->getGlobalBounds().contains(mousePos); m_scrambleSpr->setColor(scrambleHover ? sf::Color::White : sf::Color(255, 255, 255, 200)); m_window.draw(*m_scrambleSpr); } if (m_hintSpr) { bool hintHover = m_hintSpr->getGlobalBounds().contains(mousePos); sf::Color hintColor = (m_hintsAvailable > 0) ? (hintHover ? sf::Color::White : sf::Color(255, 255, 255, 200)) : sf::Color(128, 128, 128, 150); m_hintSpr->setColor(hintColor); m_window.draw(*m_hintSpr); } }


    //------------------------------------------------------------
    //  Draw HUD (Uses Base UI Scale, except for Bonus Flourish)
    //------------------------------------------------------------
    float bottomHudStartY = m_wheelY + (m_currentWheelRadius + scaledWheelPadding) + scaledHudOffsetY;
    float currentTopY = bottomHudStartY;

    // Found Text
    std::string foundCountStr = "Found: " + std::to_string(m_found.size()) + "/" + std::to_string(m_solutions.size());
    sf::Text foundTxt(m_font, foundCountStr, scaledFoundFontSize);
    foundTxt.setFillColor(m_currentTheme.hudTextFound);
    sf::FloatRect foundBounds = foundTxt.getLocalBounds();
    foundTxt.setOrigin({ foundBounds.position.x + foundBounds.size.x / 2.f, foundBounds.position.y });
    foundTxt.setPosition({ m_wheelX, currentTopY });
    m_window.draw(foundTxt);
    currentTopY += foundBounds.size.y + scaledHudLineSpacing;

    // Bonus Word Counter (with Flourish)
    if (!m_allPotentialSolutions.empty() || !m_foundBonusWords.empty()) {
        int totalPossibleBonus = 0; for (const auto& potentialWordInfo : m_allPotentialSolutions) { if (!m_found.count(potentialWordInfo.text)) { totalPossibleBonus++; } }
        std::string bonusCountStr = "Bonus Words: " + std::to_string(m_foundBonusWords.size()) + "/" + std::to_string(totalPossibleBonus);
        sf::Text bonusFoundTxt(m_font, bonusCountStr, scaledBonusFontSize);
        bonusFoundTxt.setFillColor(sf::Color::Yellow); // Or theme color
        float bonusFlourishScale = 1.0f;
        if (m_bonusTextFlourishTimer > 0.f) { float progress = (BONUS_TEXT_FLOURISH_DURATION - m_bonusTextFlourishTimer) / BONUS_TEXT_FLOURISH_DURATION; bonusFlourishScale = 1.0f + 0.4f * std::sin(progress * PI); }
        sf::FloatRect bonusBounds = bonusFoundTxt.getLocalBounds();
        bonusFoundTxt.setOrigin({ bonusBounds.position.x + bonusBounds.size.x / 2.f, bonusBounds.position.y });
        bonusFoundTxt.setPosition({ m_wheelX, currentTopY });
        bonusFoundTxt.setScale({ bonusFlourishScale, bonusFlourishScale }); // Apply scale
        m_window.draw(bonusFoundTxt);
        currentTopY += bonusBounds.size.y * bonusFlourishScale + scaledHudLineSpacing; // Adjust Y based on scaled height
    }

    // Hint Count Text
    if (m_hintCountTxt && m_hintSpr) {
        m_hintCountTxt->setCharacterSize(scaledHintFontSize);
        m_hintCountTxt->setString("Hints: " + std::to_string(m_hintsAvailable));
        m_hintCountTxt->setFillColor(m_currentTheme.hudTextFound);
        sf::FloatRect hintTxtBounds = m_hintCountTxt->getLocalBounds();
        m_hintCountTxt->setOrigin({ hintTxtBounds.position.x + hintTxtBounds.size.x / 2.f, hintTxtBounds.position.y });
        sf::FloatRect hintSprBounds = m_hintSpr->getGlobalBounds();
        float hintIconCenterX = m_hintSpr->getPosition().x - hintSprBounds.size.x / 2.f; // Use center based on origin? Check m_hintSpr origin
        float hintIconBottomY = m_hintSpr->getPosition().y + hintSprBounds.size.y / 2.f; // Use bottom based on origin
        // Adjusting slightly based on likely hint sprite origin (bottom-right?)
        hintIconCenterX = m_hintSpr->getPosition().x - hintSprBounds.size.x; // Use left edge if origin is right? Recheck hint sprite origin.
        hintIconBottomY = m_hintSpr->getPosition().y + hintSprBounds.size.y / 2.f; // Use center Y? Recheck hint sprite origin.
        // Let's assume origin is Top-Right for now as set in layout:
        hintIconCenterX = m_hintSpr->getPosition().x - hintSprBounds.size.x / 2.f; // Center X based on top-right origin
        hintIconBottomY = m_hintSpr->getPosition().y + hintSprBounds.size.y; // Bottom Y based on top-right origin
        m_hintCountTxt->setPosition({ hintIconCenterX - scaledHintOffsetX, hintIconBottomY + scaledHintOffsetY }); // Apply offsets
        m_window.draw(*m_hintCountTxt);
    }


    //------------------------------------------------------------
    //  Draw Solved State overlay
    //------------------------------------------------------------
    if (m_currentScreen == GameScreen::GameOver) {
        // ... (Solved overlay drawing - NO CHANGES) ...
        sf::Text winTxt(m_font, "Puzzle Solved!", scaledSolvedFontSize); winTxt.setFillColor(m_currentTheme.hudTextSolved); winTxt.setStyle(sf::Text::Bold);
        sf::FloatRect winTxtBounds = winTxt.getLocalBounds(); sf::Vector2f contBtnSize = m_contBtn.getSize(); const float scaledPadding = S(this, 25.f); const float scaledSpacing = S(this, 20.f);
        float overlayWidth = std::max(winTxtBounds.size.x, contBtnSize.x) + 2.f * scaledPadding; float overlayHeight = winTxtBounds.size.y + contBtnSize.y + scaledSpacing + 2.f * scaledPadding;
        m_solvedOverlay.setSize({ overlayWidth, overlayHeight }); m_solvedOverlay.setRadius(S(this, 15.f)); m_solvedOverlay.setFillColor(m_currentTheme.solvedOverlayBg); m_solvedOverlay.setOrigin({ overlayWidth / 2.f, overlayHeight / 2.f });
        sf::Vector2f windowCenterPix = sf::Vector2f(m_window.getSize()) / 2.f; sf::Vector2f overlayCenter = m_window.mapPixelToCoords(sf::Vector2i(windowCenterPix)); m_solvedOverlay.setPosition(overlayCenter);
        float winTxtCenterY = overlayCenter.y - overlayHeight / 2.f + scaledPadding + (winTxtBounds.position.y + winTxtBounds.size.y / 2.f); float contBtnPosY = winTxtCenterY + (winTxtBounds.size.y / 2.f) + scaledSpacing;
        winTxt.setOrigin({ winTxtBounds.position.x + winTxtBounds.size.x / 2.f, winTxtBounds.position.y + winTxtBounds.size.y / 2.f }); winTxt.setPosition({ overlayCenter.x, winTxtCenterY });
        if (m_contTxt) { m_contTxt->setCharacterSize(scaledContinueFontSize); sf::FloatRect contTxtBounds = m_contTxt->getLocalBounds(); m_contTxt->setOrigin({ contTxtBounds.position.x + contTxtBounds.size.x / 2.f, contTxtBounds.position.y + contTxtBounds.size.y / 2.f }); m_contBtn.setOrigin({ contBtnSize.x / 2.f, 0.f }); m_contBtn.setPosition({ overlayCenter.x, contBtnPosY }); m_contTxt->setPosition(m_contBtn.getPosition() + sf::Vector2f{ 0.f, contBtnSize.y / 2.f }); }
        bool contHover = m_contBtn.getGlobalBounds().contains(mousePos); sf::Color continueHoverColor = adjustColorBrightness(m_currentTheme.continueButton, 1.2f); m_contBtn.setFillColor(contHover ? continueHoverColor : m_currentTheme.continueButton);
        m_window.draw(m_solvedOverlay); m_window.draw(winTxt); m_window.draw(m_contBtn); if (m_contTxt) m_window.draw(*m_contTxt);
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

