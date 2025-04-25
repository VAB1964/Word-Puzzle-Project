#include "Theme.h"
#include "ThemeData.h"
#include "Game.h" // Should ideally include everything needed by the Game class declaration
#include "Words.h"
#include "Utils.h"
#include "Constants.h" // Make sure Constants.h is included

#include <SFML/Window/VideoMode.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Window/Mouse.hpp>
#include <iostream>
#include <algorithm>
#include <string>
#include <vector> // Ensure needed standard headers are here
#include <set>
#include <functional>
#include <cmath>
#include <numeric>
#include <random>
#include <ctime>
#include <memory> // For unique_ptr                  

//--------------------------------------------------------------------
//  Game Class Implementation
//--------------------------------------------------------------------

// Game.cpp

Game::Game() : 
    // --- Initialize members in the initializer list ---
    m_window(),                              // Default construct window
    m_font(),                                // Default construct font (will be loaded)
    m_clock(),
    m_currentScreen(GameScreen::MainMenu),
    m_gameState(GState::Playing),
    m_hintsAvailable(INITIAL_HINTS),         // Use constant directly here
    m_wordsSolvedSinceHint(0),
    m_currentScore(0),
    m_dragging(false),
    m_decor(10),                             // Initialize DecorLayer
    // Resource Handles (default construct textures/buffers - loaded later)
    m_scrambleTex(), m_hintTex(), m_sapphireTex(), m_rubyTex(), m_diamondTex(),
    m_selectBuffer(), m_placeBuffer(), m_winBuffer(), m_clickBuffer(), m_hintUsedBuffer(),
    // Sounds (default construct - buffer set later)
    m_selectSound(), m_placeSound(), m_winSound(), m_clickSound(), m_hintUsedSound(),
    // Music (default construct - file loaded later)
    m_backgroundMusic(),
    // Sprites (default construct - texture set later)
    m_scrambleSpr(), m_hintSpr(), m_sapphireSpr(), m_rubySpr(), m_diamondSpr(),
    // UI Shapes (can use constructor directly)
    m_contBtn({ 200.f, 50.f }, 10.f, 10),
    m_solvedOverlay(),
    m_scoreBar(),
    // Add Main Menu Shapes
    m_mainMenuBg(),
    m_casualButtonShape({ 250.f, 50.f }, 10.f, 10),
    m_competitiveButtonShape({ 250.f, 50.f }, 10.f, 10),
    m_quitButtonShape({ 250.f, 50.f }, 10.f, 10),
    // Add Casual Menu Shapes
    m_casualMenuBg(),
    m_easyButtonShape({ 250.f, 50.f }, 10.f, 10),
    m_mediumButtonShape({ 250.f, 50.f }, 10.f, 10),
    m_hardButtonShape({ 250.f, 50.f }, 10.f, 10),
    m_returnButtonShape({ 250.f, 50.f }, 10.f, 10)
{ // --- Constructor Body Starts Here ---

    // Determine Initial Window Size
    const sf::Vector2u desiredInitialSize{ 800u, 800u };
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    unsigned int initialWidth = std::min(desiredInitialSize.x, desktop.size.x);
    unsigned int initialHeight = std::min(desiredInitialSize.y, desktop.size.y);

    // Create the Window
    m_window.create(sf::VideoMode({ initialWidth, initialHeight }), "Word Puzzle", sf::Style::Default);
    m_window.setFramerateLimit(60);
    m_window.setVerticalSyncEnabled(true);

    // Load Resources (Loads font, textures, buffers)
    m_loadResources(); // This remains crucial

    // NOW safe to set properties that require loaded resources
    // Use std::make_unique to create objects managed by unique_ptr
    m_selectSound = std::make_unique<sf::Sound>(m_selectBuffer);
    m_placeSound = std::make_unique<sf::Sound>(m_placeBuffer);
    m_winSound = std::make_unique<sf::Sound>(m_winBuffer);
    m_clickSound = std::make_unique<sf::Sound>(m_clickBuffer);
    m_hintUsedSound = std::make_unique<sf::Sound>(m_hintUsedBuffer);

    m_scrambleSpr = std::make_unique<sf::Sprite>(m_scrambleTex);
    m_hintSpr = std::make_unique<sf::Sprite>(m_hintTex);
    m_sapphireSpr = std::make_unique<sf::Sprite>(m_sapphireTex);
    m_rubySpr = std::make_unique<sf::Sprite>(m_rubyTex);
    m_diamondSpr = std::make_unique<sf::Sprite>(m_diamondTex);

    // --- Set initial properties for newly created objects ---
    m_contTxt->setFillColor(sf::Color::White);
    // Set scales/origins for sprites (use -> because they are pointers now)
    float desiredGemHeight = TILE_SIZE * 0.60f; float gemScale = desiredGemHeight / m_sapphireTex.getSize().y;
    m_sapphireSpr->setScale({ gemScale, gemScale }); m_rubySpr->setScale({ gemScale, gemScale }); m_diamondSpr->setScale({ gemScale, gemScale });
    m_sapphireSpr->setOrigin({ m_sapphireTex.getSize().x / 2.f, m_sapphireTex.getSize().y / 2.f });
    m_rubySpr->setOrigin({ m_rubyTex.getSize().x / 2.f, m_rubyTex.getSize().y / 2.f });
    m_diamondSpr->setOrigin({ m_diamondTex.getSize().x / 2.f, m_diamondTex.getSize().y / 2.f });

    float scrambleScale = SCRAMBLE_BTN_HEIGHT / static_cast<float>(m_scrambleTex.getSize().y); // Use Constant
    m_scrambleSpr->setScale({ scrambleScale, scrambleScale });
    float hintScale = HINT_BTN_HEIGHT / static_cast<float>(m_hintTex.getSize().y); // Use Constant
    m_hintSpr->setScale({ hintScale, hintScale });
    // Initial Game Setup
    m_rebuild(); // Calls m_updateLayout which sets sizes for scoreBar, mainMenuBg etc.
}

// --- Main Game Loop ---
void Game::run() {
    while (m_window.isOpen()) {
        sf::Time dt = m_clock.restart();
        if (dt.asSeconds() > 0.1f) dt = sf::seconds(0.1f); // Clamp dt

        m_processEvents();
        m_update(dt);
        m_render();
    }
}

void Game::m_updateAnims(float dt) // No longer need to pass grid, font, window, sound etc.
{
    m_anims.erase(std::remove_if(m_anims.begin(), m_anims.end(),
        [&](LetterAnim& a) { // Capture 'this' implicitly or explicitly [&, this]
            a.t += dt * 4.f;
            if (a.t >= 1.f) {
                a.t = 1.f;
                if (a.wordIdx >= 0 && a.wordIdx < m_grid.size() && // Use m_grid
                    a.charIdx >= 0 && a.charIdx < m_grid[a.wordIdx].size())
                {
                    m_grid[a.wordIdx][a.charIdx] = a.ch; // Use m_grid
                }
                if (m_placeSound) m_placeSound->play(); // Use m_placeSound->
                return true;
            }
            return false;
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
            m_window.draw(a.particle); // Use m_window

            return false;
        }),
        m_scoreAnims.end());
}

// --- Resource Loading ---
// --- Resource Loading (Modified) ---
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

    // Load hint button texture (Example - adjust path as needed)
    if (!m_hintTex.loadFromFile("assets/hint_icon.png")) {
        std::cerr << "Error loading hint texture!" << std::endl;
        // Consider exiting or using a fallback visual
    }
    else {
        m_hintTex.setSmooth(true);
    }

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

    // --- Create Sounds (Link Buffers) ---
    // --- Create Sounds (Link Buffers) ---
    // Only create sound object IF the corresponding MEMBER buffer loaded successfully
    if (selectLoaded) { m_selectSound = std::make_unique<sf::Sound>(m_selectBuffer); }
    if (placeLoaded) { m_placeSound = std::make_unique<sf::Sound>(m_placeBuffer); }
    if (winLoaded) { m_winSound = std::make_unique<sf::Sound>(m_winBuffer); }
    if (clickLoaded) { m_clickSound = std::make_unique<sf::Sound>(m_clickBuffer); }
    if (hintUsedLoaded) { m_hintUsedSound = std::make_unique<sf::Sound>(m_hintUsedBuffer); }
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
    m_fullWordList = Words::loadWordListWithRarity("words.csv");
    if (m_fullWordList.empty()) { std::cerr << "Failed to load word list or list is empty. Exiting." << std::endl; exit(1); }
    m_roots = Words::withLength(m_fullWordList, WORD_LENGTH);
    if (m_roots.empty()) m_roots = Words::withLength(m_fullWordList, 6);
    if (m_roots.empty()) m_roots = Words::withLength(m_fullWordList, 5);
    if (m_roots.empty()) m_roots = Words::withLength(m_fullWordList, 4);
    if (m_roots.empty()) { std::cerr << "No suitable root words found in list. Exiting." << std::endl; exit(1); }

    // --- Load Color Themes ---
    // Clear any potential default theme added erroneously earlier if necessary
    m_themes.clear();
    m_themes = loadThemes();

    // Check if loading failed or returned empty
    if (m_themes.empty()) {
        std::cerr << "CRITICAL Warning: loadThemes() returned empty vector. Using fallback default theme.\n";
        m_themes.push_back({}); // Add a default-constructed theme as a fallback
    }

}


// --- Process Events (Placeholder) ---
void Game::m_processEvents() {
    // Will copy event loop logic here later
    while (const std::optional optEvent = m_window.pollEvent()) {

        const sf::Event& event = *optEvent;
        // --- Screen-Specific Events ---
        if (m_currentScreen == GameScreen::MainMenu) {
            m_handleMainMenuEvents(event);
        }
        else if (m_currentScreen == GameScreen::CasualMenu) { // *** ADD THIS BRANCH ***
            m_handleCasualMenuEvents(event);
        }
        // else if (m_currentScreen == GameScreen::CompetitiveMenu) { m_handleCompetitiveMenuEvents(event); } // Keep commented
        else if (m_currentScreen == GameScreen::Playing) {
            m_handlePlayingEvents(event);
        }
        else if (m_currentScreen == GameScreen::GameOver) {
            m_handleGameOverEvents(event);
        }
        if (event.is<sf::Event::Closed>()) {
            m_window.close();
        }
    }
}

// --- Update (Placeholder) ---
void Game::m_update(sf::Time dt) {
    float deltaSeconds = dt.asSeconds();
    m_decor.update(deltaSeconds, m_window.getSize(), m_currentTheme);

    if (m_currentScreen == GameScreen::Playing || m_currentScreen == GameScreen::GameOver)
    {
        m_updateAnims(deltaSeconds); // Call member function
        m_updateScoreAnims(deltaSeconds); // Call member function
    }
}

// --- Render ---
void Game::m_render() {
    m_window.clear(m_currentTheme.winBg);
    m_decor.draw(m_window); // Draw background decor first

    // Get mouse position once for hover checks within render helpers
    sf::Vector2f mpos = m_window.mapPixelToCoords(sf::Mouse::getPosition(m_window));

    // --- Draw based on current screen ---
    if (m_currentScreen == GameScreen::MainMenu) {
        m_renderMainMenu(mpos);
    }
    else if (m_currentScreen == GameScreen::CasualMenu) { // *** ADD THIS BRANCH ***
        m_renderCasualMenu(mpos);
    }
    // else if (m_currentScreen == GameScreen::CompetitiveMenu) { m_renderCompetitiveMenu(mpos); } // Keep commented
    else { // Playing or GameOver uses the game screen renderer
        m_renderGameScreen(mpos);
    }

    // TODO: Draw Pop-ups last if needed

    m_window.display(); // Display everything drawn
}

// --- Other Private Methods (Placeholders) ---
void Game::m_rebuild() {
    // Select Random Theme
    if (!m_themes.empty()) {
        m_currentTheme = m_themes[randRange<std::size_t>(0, m_themes.size() - 1)];
    }
    else {
        m_currentTheme = {}; // Default constructor (fallback)
        std::cerr << "Warning: No themes loaded, using default colors.\n";
    }

    // Setup Words
    if (m_roots.empty()) {
        m_base = "error"; // Handle error case
        m_solutions.clear();
        m_sorted.clear();
        std::cerr << "Error: No root words available for puzzle generation.\n";
    }
    else {
        const WordInfo& rootWordInfo = m_roots[randRange<std::size_t>(0, m_roots.size() - 1)];
        m_base = rootWordInfo.text;
        std::shuffle(m_base.begin(), m_base.end(), Rng()); // Use global Rng() from Utils.h
        m_solutions = Words::subWords(m_base, m_fullWordList);
        m_sorted = Words::sortForGrid(m_solutions);
    }

    // Setup Grid
    m_grid.assign(m_sorted.size(), {});
    for (std::size_t i = 0; i < m_sorted.size(); ++i) {
        if (!m_sorted[i].text.empty()) {
            m_grid[i].assign(m_sorted[i].text.length(), '_');
        }
        else {
            m_grid[i].clear();
        }
    }

    // Reset Game State Variables
    m_found.clear();
    m_anims.clear();
    m_scoreAnims.clear(); // Clear score particles too
    m_clearDragState(); // Use helper function
    m_gameState = GState::Playing; // Set internal game state
    // m_currentScreen should be set by the menu logic before calling rebuild

    // Reset Score & Hints (Score reset removed for persistence)
     // currentScore = 0; // Keep score persistent
    m_hintsAvailable = INITIAL_HINTS;
    m_wordsSolvedSinceHint = 0;
    if (m_scoreValueText) m_scoreValueText->setString(std::to_string(m_currentScore)); // Update score display


    // IMPORTANT: Calculate layout based on the new data
    m_updateLayout();

    // Start Background Music
    m_backgroundMusic.stop();
    if (!m_musicFiles.empty()) {
        std::string musicPath = m_musicFiles[randRange<std::size_t>(0, m_musicFiles.size() - 1)];
        if (m_backgroundMusic.openFromFile(musicPath)) {
            m_backgroundMusic.setLooping(true);
            m_backgroundMusic.play();
        }
        else {
            std::cerr << "Error loading music file: " << musicPath << std::endl;
        }
    }
}
// In Game.cpp

void Game::m_updateLayout() {
    // Get current window size from the member variable
    sf::Vector2u winSize = m_window.getSize();
    sf::Vector2f windowCenter = sf::Vector2f(winSize) / 2.f;

    // --- Score Bar ---
    float scoreBarWidth = 250.f;
    m_scoreBar.setSize({ scoreBarWidth, SCORE_BAR_HEIGHT });
    m_scoreBar.setRadius(10.f);
    m_scoreBar.setOrigin({ scoreBarWidth / 2.f, SCORE_BAR_HEIGHT / 2.f });
    m_scoreBar.setPosition({ windowCenter.x, SCORE_BAR_TOP_MARGIN + SCORE_BAR_HEIGHT / 2.f });

    // --- Score Text Positioning ---
    sf::Vector2f barCenter = m_scoreBar.getPosition();
    if (m_scoreLabelText) { // Check if pointer is valid
        sf::FloatRect labelBounds = m_scoreLabelText->getLocalBounds();
        m_scoreLabelText->setOrigin({ labelBounds.size.x, labelBounds.position.y + labelBounds.size.y / 2.f });
        m_scoreLabelText->setPosition({ barCenter.x - 5.f, barCenter.y });
    }
    if (m_scoreValueText) { // Check if pointer is valid
        sf::FloatRect valueBounds = m_scoreValueText->getLocalBounds();
        m_scoreValueText->setOrigin({ 0.f, valueBounds.position.y + valueBounds.size.y / 2.f });
        m_scoreValueText->setPosition({ barCenter.x + 5.f, barCenter.y });
    }

    // --- Adjust Grid Start Position ---
    // NOTE: m_gridStartY is now a member variable
    m_gridStartY = SCORE_BAR_TOP_MARGIN + SCORE_BAR_HEIGHT + GRID_TOP_MARGIN;

    // --- Wheel & Game Elements ---
    m_wheelX = windowCenter.x;
    m_wheelY = (float)winSize.y - WHEEL_BOTTOM_MARGIN;
    m_wheelCentres.resize(m_base.size()); // Use m_base
    if (!m_base.empty()) {
        float angleStep = (2.f * PI) / static_cast<float>(m_base.size());
        for (std::size_t i = 0; i < m_base.size(); ++i) {
            float ang = static_cast<float>(i) * angleStep - PI / 2.f;
            m_wheelCentres[i] = { m_wheelX + WHEEL_R * std::cos(ang), m_wheelY + WHEEL_R * std::sin(ang) };
        }
    }

    // --- Grid Layout Calculation ---
    m_totalGridW = 0.f; // Reset grid width calculation
    if (m_sorted.empty()) { // Use m_sorted
        m_gridStartX = m_wheelX;
        m_wordCol.clear(); m_wordRow.clear(); m_colMaxLen.clear(); m_colXOffset.clear();
    }
    else {
        float gridMaxY = m_wheelY - WHEEL_R - LETTER_R - GRID_WHEEL_GAP;
        float availableGridHeight = std::max(0.f, gridMaxY - m_gridStartY);
        float tileAndPadHeight = TILE_SIZE + TILE_PAD;
        if (tileAndPadHeight <= 0) tileAndPadHeight = 1.f;
        int maxRowsPerCol = std::max(1, static_cast<int>(availableGridHeight / tileAndPadHeight));
        int numCols = std::max(1, static_cast<int>(std::ceil(static_cast<float>(m_sorted.size()) / static_cast<float>(maxRowsPerCol))));

        m_wordCol.resize(m_sorted.size());
        m_wordRow.resize(m_sorted.size());
        m_colMaxLen.assign(numCols, 0);
        for (std::size_t w = 0; w < m_sorted.size(); ++w) {
            int c = static_cast<int>(w) / maxRowsPerCol;
            int r = static_cast<int>(w) % maxRowsPerCol;
            if (c >= numCols) c = numCols - 1;
            m_wordCol[w] = c; m_wordRow[w] = r;
            if (c >= 0 && c < m_colMaxLen.size()) {
                m_colMaxLen[c] = std::max<int>(m_colMaxLen[c], static_cast<int>(m_sorted[w].text.length()));
            }
        }

        m_colXOffset.resize(numCols);
        float currentX = 0;
        float tileAndPadWidth = TILE_SIZE + TILE_PAD;
        for (int c = 0; c < numCols; ++c) {
            m_colXOffset[c] = currentX;
            float colWidth = static_cast<float>(m_colMaxLen[c]) * tileAndPadWidth - (m_colMaxLen[c] > 0 ? TILE_PAD : 0.f);
            if (colWidth < 0) colWidth = 0;
            currentX += colWidth + COL_PAD;
        }
        m_totalGridW = currentX - (numCols > 0 ? COL_PAD : 0.f);
        if (m_totalGridW < 0) m_totalGridW = 0;
        m_gridStartX = m_wheelX - m_totalGridW / 2.f;
        for (int c = 0; c < numCols; ++c) {
            m_colXOffset[c] += m_gridStartX; // Make absolute
        }
    }

    // --- Other UI Element Positions (relative to wheel/window) ---
    if (m_scrambleSpr && !m_scrambleTex.getSize().y == 0) { // Check pointer and texture loaded
        float scrambleScale = SCRAMBLE_BTN_HEIGHT / static_cast<float>(m_scrambleTex.getSize().y);
        m_scrambleSpr->setScale({ scrambleScale, scrambleScale });
        m_scrambleSpr->setOrigin({ 0.f, static_cast<float>(m_scrambleTex.getSize().y) / 2.f });
        m_scrambleSpr->setPosition({ m_wheelX + WHEEL_R + SCRAMBLE_BTN_OFFSET_X, m_wheelY + SCRAMBLE_BTN_OFFSET_Y });
    }
    if (m_hintSpr && !m_hintTex.getSize().y == 0) { // Check pointer and texture loaded
        float hintScale = HINT_BTN_HEIGHT / static_cast<float>(m_hintTex.getSize().y);
        m_hintSpr->setScale({ hintScale, hintScale });
        m_hintSpr->setOrigin({ static_cast<float>(m_hintTex.getSize().x), static_cast<float>(m_hintTex.getSize().y) / 2.f });
        m_hintSpr->setPosition({ m_wheelX - WHEEL_R - SCRAMBLE_BTN_OFFSET_X, m_wheelY + SCRAMBLE_BTN_OFFSET_Y });
    }
    if (m_contTxt) { // Check pointer
        sf::FloatRect contBtnBounds = m_contBtn.getLocalBounds();
        m_contBtn.setOrigin({ contBtnBounds.size.x / 2.f, 0.f });
        m_contBtn.setPosition({ m_wheelX, m_wheelY + WHEEL_R + CONTINUE_BTN_OFFSET_Y });
        sf::FloatRect contTxtBounds = m_contTxt->getLocalBounds();
        m_contTxt->setOrigin({ contTxtBounds.position.x + contTxtBounds.size.x / 2.f, contTxtBounds.position.y + contTxtBounds.size.y / 2.f });
        m_contTxt->setPosition(m_contBtn.getPosition() + sf::Vector2f{ 0.f, contBtnBounds.size.y / 2.f });
    }


    // --- Menu Layout Calculations ---
    // Ensure text objects are valid before getting bounds
    if (m_mainMenuTitle && m_casualButtonShape.getSize().x > 0) { // Check pointer and valid button size
        const float menuPadding = 40.f;
        const float buttonSpacing = 20.f;
        sf::Vector2f buttonSize = m_casualButtonShape.getSize();

        sf::FloatRect titleBounds = m_mainMenuTitle->getLocalBounds();
        float titleWidth = titleBounds.position.x + titleBounds.size.x; 

        float mainMenuHeight = m_mainMenuTitle->getCharacterSize() + buttonSpacing + 3 * buttonSize.y + 2 * buttonSpacing + 2 * menuPadding;
        float mainMenuWidth = std::max(buttonSize.x, titleWidth) + 2 * menuPadding;

        m_mainMenuBg.setSize({ mainMenuWidth, mainMenuHeight });
        m_mainMenuBg.setRadius(15.f);
        m_mainMenuBg.setOrigin({ mainMenuWidth / 2.f, mainMenuHeight / 2.f });
        m_mainMenuBg.setPosition(windowCenter);

        sf::Vector2f menuTopLeft = { windowCenter.x - mainMenuWidth / 2.f, windowCenter.y - mainMenuHeight / 2.f };

        m_mainMenuTitle->setOrigin({ titleBounds.position.x + titleBounds.size.x / 2.f, titleBounds.position.y });
        m_mainMenuTitle->setPosition({ windowCenter.x, menuTopLeft.y + menuPadding });
        float currentY = menuTopLeft.y + menuPadding + m_mainMenuTitle->getCharacterSize() + buttonSpacing * 2;

        // Position Buttons and Text (Check pointers before accessing methods)
        if (m_casualButtonText) {
            m_casualButtonShape.setOrigin({ buttonSize.x / 2.f, 0.f });
            m_casualButtonShape.setPosition({ windowCenter.x, currentY });
            sf::FloatRect casualTextBounds = m_casualButtonText->getLocalBounds();
            m_casualButtonText->setOrigin({ casualTextBounds.position.x + casualTextBounds.size.x / 2.f, casualTextBounds.position.y + casualTextBounds.size.y / 2.f });
            m_casualButtonText->setPosition(m_casualButtonShape.getPosition() + sf::Vector2f{ 0.f, buttonSize.y / 2.f });
            currentY += buttonSize.y + buttonSpacing;
        }
        if (m_competitiveButtonText) {
            m_competitiveButtonShape.setOrigin({ buttonSize.x / 2.f, 0.f });
            m_competitiveButtonShape.setPosition({ windowCenter.x, currentY });
            sf::FloatRect compTextBounds = m_competitiveButtonText->getLocalBounds();
            m_competitiveButtonText->setOrigin({ compTextBounds.position.x + compTextBounds.size.x / 2.f, compTextBounds.position.y + compTextBounds.size.y / 2.f });
            m_competitiveButtonText->setPosition(m_competitiveButtonShape.getPosition() + sf::Vector2f{ 0.f, buttonSize.y / 2.f });
            currentY += buttonSize.y + buttonSpacing;
        }
        if (m_quitButtonText) {
            m_quitButtonShape.setOrigin({ buttonSize.x / 2.f, 0.f });
            m_quitButtonShape.setPosition({ windowCenter.x, currentY });
            sf::FloatRect quitTextBounds = m_quitButtonText->getLocalBounds();
            m_quitButtonText->setOrigin({ quitTextBounds.position.x + quitTextBounds.size.x / 2.f, quitTextBounds.position.y + quitTextBounds.size.y / 2.f });
            m_quitButtonText->setPosition(m_quitButtonShape.getPosition() + sf::Vector2f{ 0.f, buttonSize.y / 2.f });
        }
    }

    // --- Casual Menu Layout ---
// Similar logic to Main Menu, adjust title/button count
    const float casualButtonSpacing = 20.f; // Can use different spacing if needed
    sf::Vector2f casualButtonSize = m_easyButtonShape.getSize(); // Assume same size

    if (m_casualMenuTitle) { // Check pointer
        const float menuPadding = 40.f;
        sf::FloatRect casualTitleBounds = m_casualMenuTitle->getLocalBounds();
        float casualTitleWidth = casualTitleBounds.position.x + casualTitleBounds.size.x;

        float casualMenuHeight = m_casualMenuTitle->getCharacterSize() + casualButtonSpacing + 4 * casualButtonSize.y + 3 * casualButtonSpacing + 2 * menuPadding;
        float casualMenuWidth = std::max(casualButtonSize.x, casualTitleWidth) + 2 * menuPadding;

        m_casualMenuBg.setSize({ casualMenuWidth, casualMenuHeight });
        m_casualMenuBg.setRadius(15.f);
        m_casualMenuBg.setOrigin({ casualMenuWidth / 2.f, casualMenuHeight / 2.f });
        m_casualMenuBg.setPosition(windowCenter);

        sf::Vector2f casualMenuTopLeft = { windowCenter.x - casualMenuWidth / 2.f, windowCenter.y - casualMenuHeight / 2.f };

        m_casualMenuTitle->setOrigin({ casualTitleBounds.position.x + casualTitleBounds.size.x / 2.f, casualTitleBounds.position.y });
        m_casualMenuTitle->setPosition({ windowCenter.x, casualMenuTopLeft.y + menuPadding });
        float currentCasualY = casualMenuTopLeft.y + menuPadding + m_casualMenuTitle->getCharacterSize() + casualButtonSpacing * 2;

        // Position Buttons and Text
        if (m_easyButtonText) {
            m_easyButtonShape.setOrigin({ casualButtonSize.x / 2.f, 0.f });
            m_easyButtonShape.setPosition({ windowCenter.x, currentCasualY });
            sf::FloatRect easyTextBounds = m_easyButtonText->getLocalBounds();
            m_easyButtonText->setOrigin({ easyTextBounds.position.x + easyTextBounds.size.x / 2.f, easyTextBounds.position.y + easyTextBounds.size.y / 2.f });
            m_easyButtonText->setPosition(m_easyButtonShape.getPosition() + sf::Vector2f{ 0.f, casualButtonSize.y / 2.f });
            currentCasualY += casualButtonSize.y + casualButtonSpacing;
        }
        if (m_mediumButtonText) {
            m_mediumButtonShape.setOrigin({ casualButtonSize.x / 2.f, 0.f });
            m_mediumButtonShape.setPosition({ windowCenter.x, currentCasualY });
            sf::FloatRect medTextBounds = m_mediumButtonText->getLocalBounds();
            m_mediumButtonText->setOrigin({ medTextBounds.position.x + medTextBounds.size.x / 2.f, medTextBounds.position.y + medTextBounds.size.y / 2.f });
            m_mediumButtonText->setPosition(m_mediumButtonShape.getPosition() + sf::Vector2f{ 0.f, casualButtonSize.y / 2.f });
            currentCasualY += casualButtonSize.y + casualButtonSpacing;
        }
        if (m_hardButtonText) {
            m_hardButtonShape.setOrigin({ casualButtonSize.x / 2.f, 0.f });
            m_hardButtonShape.setPosition({ windowCenter.x, currentCasualY });
            sf::FloatRect hardTextBounds = m_hardButtonText->getLocalBounds();
            m_hardButtonText->setOrigin({ hardTextBounds.position.x + hardTextBounds.size.x / 2.f, hardTextBounds.position.y + hardTextBounds.size.y / 2.f });
            m_hardButtonText->setPosition(m_hardButtonShape.getPosition() + sf::Vector2f{ 0.f, casualButtonSize.y / 2.f });
            currentCasualY += casualButtonSize.y + casualButtonSpacing;
        }
        if (m_returnButtonText) {
            m_returnButtonShape.setOrigin({ casualButtonSize.x / 2.f, 0.f });
            m_returnButtonShape.setPosition({ windowCenter.x, currentCasualY });
            sf::FloatRect retTextBounds = m_returnButtonText->getLocalBounds();
            m_returnButtonText->setOrigin({ retTextBounds.position.x + retTextBounds.size.x / 2.f, retTextBounds.position.y + retTextBounds.size.y / 2.f });
            m_returnButtonText->setPosition(m_returnButtonShape.getPosition() + sf::Vector2f{ 0.f, casualButtonSize.y / 2.f });
        }
    }
} // End of m_updateLayout implementation

sf::Vector2f Game::m_tilePos(int wordIdx, int charIdx) {
    // Use member variables m_sorted, m_wordCol, m_wordRow, m_colXOffset, m_gridStartY
    sf::Vector2f result = { -100.f, -100.f };

    if (m_sorted.empty() || wordIdx < 0 || wordIdx >= m_wordCol.size() || wordIdx >= m_wordRow.size() || charIdx < 0) {
        // Out of bounds or no data
    }
    else {
        int c = m_wordCol[wordIdx];
        int r = m_wordRow[wordIdx];

        if (c < 0 || c >= m_colXOffset.size()) {
            // Invalid column index
        }
        else {
            float x = m_colXOffset[c] + static_cast<float>(charIdx) * (TILE_SIZE + TILE_PAD);
            float y = m_gridStartY + static_cast<float>(r) * (TILE_SIZE + TILE_PAD);
            result = { x, y };
        }
    }
    return result;
}

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

        // TODO: Store selected difficulty later

        if (m_easyButtonShape.getGlobalBounds().contains(mp)) {
            m_clickSound->play();
            m_rebuild(); // Rebuild game with selected settings (implicitly easy for now)
            m_currentScreen = GameScreen::Playing;
        }
        else if (m_mediumButtonShape.getGlobalBounds().contains(mp)) {
            m_clickSound->play();
            m_rebuild(); // Rebuild game with selected settings (implicitly medium for now)
            m_currentScreen = GameScreen::Playing;
        }
        else if (m_hardButtonShape.getGlobalBounds().contains(mp)) {
            m_clickSound->play();
            m_rebuild(); // Rebuild game with selected settings (implicitly hard for now)
            m_currentScreen = GameScreen::Playing;
        }
        else if (m_returnButtonShape.getGlobalBounds().contains(mp)) {
            m_clickSound->play();
            m_currentScreen = GameScreen::MainMenu; // Go back
        }
    }
    // TODO: Handle hover for pop-ups in MouseMoved later
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

    // --- Handle Window Resize ---
    // (Redundant if handled globally in m_processEvents, but safe to keep)
    if (const auto* rs = event.getIf<sf::Event::Resized>()) {
        sf::FloatRect visibleArea(
            { 0.f, 0.f }, // Top-left position as sf::Vector2f
            { static_cast<float>(rs->size.x), static_cast<float>(rs->size.y) } // Size as sf::Vector2f
        );
        m_window.setView(sf::View(visibleArea));
        m_updateLayout(); // Recalculate positions based on new size
        return;
    }


    // Only process game input if not solved (already checked by screen state, but double-check internal state)
    if (m_gameState != GState::Playing) {
        return; // Don't process game input if already solved internally
    }

    // --- Mouse Button Pressed ---
    if (const auto* mb = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mb->button == sf::Mouse::Button::Left) {
            sf::Vector2f mp = m_window.mapPixelToCoords(mb->position);

            // Check Scramble Button
            if (m_scrambleSpr && m_scrambleSpr->getGlobalBounds().contains(mp)) {
                if (m_clickSound) m_clickSound->play();
                std::shuffle(m_base.begin(), m_base.end(), Rng());
                m_updateLayout(); // Update wheel letter positions graphically
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
                if (distSq(mp, m_wheelCentres[i]) < LETTER_R * LETTER_R) {
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
                if (distSq(mp, m_wheelCentres[i]) < LETTER_R * LETTER_R) {

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
            bool foundMatch = false;
            int foundWordIdx = -1;

            // m_currentGuess is already built in UPPERCASE by the drag logic

            for (std::size_t w = 0; w < m_sorted.size(); ++w) {

                // Get the solution word in its original case (likely lowercase)
                const std::string& solutionOriginalCase = m_sorted[w].text;

                // Check if this original case word is already in the found set
                bool alreadyFound = (m_found.find(solutionOriginalCase) != m_found.end());

                // --- Start of Changes ---

                // Create an uppercase version of the solution word for comparison
                std::string solutionUpper = solutionOriginalCase;
                std::transform(solutionUpper.begin(), solutionUpper.end(), solutionUpper.begin(),
                    [](unsigned char c) { return std::toupper(c); });

                // Compare the uppercase solution with the uppercase guess
                bool textMatch = (solutionUpper == m_currentGuess);

                // --- End of Changes ---


                // Check if word not already found AND the uppercase versions match
                if (!alreadyFound && textMatch) { // Use the new comparison result
                    foundMatch = true;
                    foundWordIdx = static_cast<int>(w);

                    // *** IMPORTANT: Add the ORIGINAL case word to the found set ***
                    m_found.insert(solutionOriginalCase);

                    // Add score based on length and rarity (Example scoring)
                    // Use m_currentGuess.length() as it reflects the length of the matched word
                    int baseScore = static_cast<int>(m_currentGuess.length()) * 10;
                    int rarityBonus = (m_sorted[w].rarity > 1) ? (m_sorted[w].rarity * 25) : 0;
                    m_currentScore += baseScore + rarityBonus;

                    // Update score display immediately (optional, render also does it)
                    if (m_scoreValueText) m_scoreValueText->setString(std::to_string(m_currentScore));

                    // Check for earning a hint
                    m_wordsSolvedSinceHint++;
                    if (m_wordsSolvedSinceHint >= WORDS_PER_HINT) {
                        m_hintsAvailable++;
                        m_wordsSolvedSinceHint = 0; // Reset counter
                        if (m_hintCountTxt) m_hintCountTxt->setString("Hints: " + std::to_string(m_hintsAvailable)); // Update display
                        // TODO: Maybe add a small visual/audio cue for earning a hint?
                    }


                    // --- Create letter animations ---
                    // Use m_currentGuess for the characters as they are uppercase and match the user input
                    for (std::size_t c = 0; c < m_currentGuess.length(); ++c) {
                        if (c < m_path.size()) { // Ensure path index is valid
                            int pathNodeIdx = m_path[c];
                            if (pathNodeIdx >= 0 && pathNodeIdx < m_wheelCentres.size()) {
                                sf::Vector2f startPos = m_wheelCentres[pathNodeIdx];
                                sf::Vector2f endPos = m_tilePos(foundWordIdx, static_cast<int>(c));
                                endPos.x += TILE_SIZE / 2.f; // Center of tile
                                endPos.y += TILE_SIZE / 2.f;

                                // Animate the uppercase letter from the guess
                                m_anims.push_back({
                                    m_currentGuess[c],
                                    startPos,
                                    endPos,
                                    0.f,
                                    foundWordIdx,
                                    static_cast<int>(c)
                                    });
                            }
                        }
                    }
                    // --- End letter animations ---
                    std::cout << "Word: " << m_currentGuess << " | Rarity: " << m_sorted[foundWordIdx].rarity << " | Len: " << m_currentGuess.length() << " | Rarity Bonus: " << rarityBonus << " | BasePts: " << baseScore << " | Total: " << m_currentScore << std::endl;

                    // --- Check for Puzzle Solved ---
                    // Compare count of found words (original case) with total solutions
                    if (m_found.size() == m_solutions.size()) {
                        if (m_winSound) m_winSound->play();
                        m_gameState = GState::Solved; // Internal state
                        m_currentScreen = GameScreen::GameOver; // Change screen state
                    }
                    break; // Found a match, stop checking other words
                }
            } // End for loop iterating through potential solution words

            // If no match was found, maybe play a "buzz" or incorrect sound?
            if (!foundMatch) {
                // std::cout << "Word '" << m_currentGuess << "' not found or already solved.\n"; // Optional Debug
                // Play incorrect sound if you have one
            }

            m_clearDragState(); // Clear path and guess regardless of match
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
        m_updateLayout(); // Recalculate layout for the game over screen too
        return;
    }

    // --- Mouse Button Pressed (for Continue button) ---
    if (const auto* mb = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mb->button == sf::Mouse::Button::Left) {
            sf::Vector2f mp = m_window.mapPixelToCoords(mb->position);

            // Check Continue Button Click
            if (m_contBtn.getGlobalBounds().contains(mp)) {
                if (m_clickSound) m_clickSound->play();
                // Option 1: Go back to the main menu
                m_currentScreen = GameScreen::MainMenu;

                // Option 2: Start a new game immediately (if desired)
                // m_rebuild(); // Generate new puzzle
                // m_currentScreen = GameScreen::Playing; // Switch back to playing

                // Reset internal state if necessary (rebuild does some of this)
                m_gameState = GState::Playing; // Reset internal state for next game
            }
        }
    }

    // --- Key Pressed (Optional: Enter to Continue) ---
    // if (event.is<sf::Event::KeyPressed>()) {
    //     if (event.getIf<sf::Event::KeyPressed>()->scancode == sf::Keyboard::Scan::Enter ||
    //         event.getIf<sf::Event::KeyPressed>()->scancode == sf::Keyboard::Scan::NumpadEnter ||
    //         event.getIf<sf::Event::KeyPressed>()->scancode == sf::Keyboard::Scan::Space ) {
    //
    //          if (m_clickSound) m_clickSound->play();
    //          m_currentScreen = GameScreen::MainMenu; // Or m_rebuild() etc.
    //          m_gameState = GState::Playing;
    //     }
    // }

} // End m_handleGameOverEvents

// --- Render Game Screen ---
void Game::m_renderGameScreen(const sf::Vector2f& mousePos) {
    //------------------------------------------------------------
    //  Draw Score Bar
    //------------------------------------------------------------
    m_scoreBar.setFillColor(m_currentTheme.scoreBarBg);
    m_scoreLabelText->setFillColor(m_currentTheme.scoreTextLabel);
    m_scoreValueText->setFillColor(m_currentTheme.scoreTextValue);
    m_scoreValueText->setString(std::to_string(m_currentScore));
    // Re-center score value text
    sf::FloatRect valueBounds = m_scoreValueText->getLocalBounds();
    m_scoreValueText->setOrigin({ 0.f, valueBounds.position.y + valueBounds.size.y / 2.f });
    m_scoreValueText->setPosition({ m_scoreBar.getPosition().x + 5.f, m_scoreBar.getPosition().y });

    m_window.draw(m_scoreBar);
    m_window.draw(*m_scoreLabelText);
    m_window.draw(*m_scoreValueText);

    //------------------------------------------------------------
    //  Draw letter grid (Gems visible before solving)
    //------------------------------------------------------------
    if (!m_sorted.empty()) {
        RoundedRectangleShape tileBackground({ TILE_SIZE, TILE_SIZE }, TILE_SIZE * 0.18f, 10);
        tileBackground.setOutlineThickness(1.f);

        sf::Text letterText(m_font, "", 20); // Recreate locally for simplicity here
        letterText.setFillColor(m_currentTheme.gridLetter);

        for (std::size_t w = 0; w < m_sorted.size(); ++w) {
            if (w >= m_grid.size() || w >= m_sorted.size()) continue;

            int wordRarity = m_sorted[w].rarity;
            for (std::size_t c = 0; c < m_sorted[w].text.length(); ++c) {
                if (c >= m_grid[w].size()) continue;

                sf::Vector2f p = m_tilePos(static_cast<int>(w), static_cast<int>(c));
                bool isFilled = (m_grid[w][c] != '_');

                // --- Draw Base Tile (Empty Look) ---
                tileBackground.setPosition(p);
                tileBackground.setFillColor(m_currentTheme.gridEmptyTile);
                tileBackground.setOutlineColor(sf::Color(200, 200, 200));
                m_window.draw(tileBackground);

                // --- Draw Gem ONLY if the slot is EMPTY ---
                if (!isFilled) {
                    sf::Sprite* gemSprite = nullptr;
                    if (wordRarity == 2) { gemSprite = m_sapphireSpr.get(); }
                    else if (wordRarity == 3) { gemSprite = m_rubySpr.get(); }
                    else if (wordRarity == 4) { gemSprite = m_diamondSpr.get(); }

                    if (gemSprite != nullptr) {
                        float tileCenterX = p.x + TILE_SIZE / 2.f;
                        float tileCenterY = p.y + TILE_SIZE / 2.f;
                        gemSprite->setPosition({ tileCenterX, tileCenterY });
                        m_window.draw(*gemSprite);
                    }
                }
                // --- Draw Filled Tile Appearance and Letter if Filled ---
                else {
                    bool isAnimatingToTile = false;
                    for (const auto& anim : m_anims) if (anim.wordIdx == static_cast<int>(w) && anim.charIdx == static_cast<int>(c)) { isAnimatingToTile = true; break; }

                    if (!isAnimatingToTile) {
                        tileBackground.setPosition(p);
                        tileBackground.setFillColor(m_currentTheme.gridFilledTile);
                        tileBackground.setOutlineColor(adjustColorBrightness(m_currentTheme.gridFilledTile, 0.7f));
                        m_window.draw(tileBackground);

                        letterText.setString(std::string(1, m_grid[w][c]));
                        sf::FloatRect b = letterText.getLocalBounds();
                        letterText.setOrigin({ b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f });
                        letterText.setPosition(p + sf::Vector2f{ TILE_SIZE / 2.f, TILE_SIZE / 2.f });
                        m_window.draw(letterText);
                    }
                }
            }
        }
    } // End grid drawing

    //------------------------------------------------------------
    //  Draw wheel background & letters
    //------------------------------------------------------------
    m_wheelBg.setRadius(WHEEL_R + 30.f);
    m_wheelBg.setFillColor(m_currentTheme.wheelBg);
    m_wheelBg.setOutlineColor(m_currentTheme.wheelOutline);
    m_wheelBg.setOutlineThickness(3.f);
    m_wheelBg.setOrigin({ m_wheelBg.getRadius(), m_wheelBg.getRadius() });
    m_wheelBg.setPosition({ m_wheelX, m_wheelY });
    m_window.draw(m_wheelBg);

     // --- DRAW FLYING LETTER ANIMATIONS --- <<<<<<<<< ADD THIS SECTION
    // (Draw these AFTER the static grid so they appear on top)
    sf::Text flyingLetterText(m_font, "", 20); // Create a reusable Text object
    flyingLetterText.setFillColor(m_currentTheme.gridLetter); // Use theme color

    for (const auto& a : m_anims) {
        // Only draw animations that are currently in progress (t < 1.0)
        if (a.t >= 1.f) continue;

        // Calculate interpolated position
        // Use easing function for smoother animation (optional but nice)
        float eased_t = a.t * a.t * (3.f - 2.f * a.t); // Smoothstep easing
        // float eased_t = a.t; // Linear (original)
        sf::Vector2f p = a.start + (a.end - a.start) * eased_t;

        flyingLetterText.setString(std::string(1, a.ch));
        sf::FloatRect bounds = flyingLetterText.getLocalBounds();
        flyingLetterText.setOrigin({ bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f });
        flyingLetterText.setPosition(p);
        m_window.draw(flyingLetterText);
    }

    //------------------------------------------------------------
    //  Draw Path lines (BEFORE Wheel Letters)
    //------------------------------------------------------------
    if (m_dragging && !m_path.empty()) {
        float thickness = 5.0f; float halfThickness = thickness / 2.0f;
        sf::PrimitiveType stripType = static_cast<sf::PrimitiveType>(4);

        if (m_path.size() >= 2) {
            sf::VertexArray thickLineStrip(stripType, m_path.size() * 2);
            for (std::size_t i = 0; i < m_path.size(); ++i) {
                std::size_t prevIdx = (i > 0) ? m_path[i - 1] : m_path[i];
                std::size_t currIdx = m_path[i];
                std::size_t nextIdx = (i < m_path.size() - 1) ? m_path[i + 1] : m_path[i];
                sf::Vector2f prevPos, currPos, nextPos;
                bool currentValid = currIdx < m_wheelCentres.size();
                currPos = currentValid ? m_wheelCentres[currIdx] : sf::Vector2f{ m_wheelX, m_wheelY };
                prevPos = (i > 0 && prevIdx < m_wheelCentres.size() && prevIdx != currIdx) ? m_wheelCentres[prevIdx] : currPos;
                nextPos = (i < m_path.size() - 1 && nextIdx < m_wheelCentres.size() && nextIdx != currIdx) ? m_wheelCentres[nextIdx] : currPos;
                sf::Vector2f dir1 = currPos - prevPos; sf::Vector2f dir2 = nextPos - currPos;
                float len1 = std::hypot(dir1.x, dir1.y); if (len1 > 1e-6f) dir1 /= len1;
                float len2 = std::hypot(dir2.x, dir2.y); if (len2 > 1e-6f) dir2 /= len2;
                sf::Vector2f normal1 = { -dir1.y, dir1.x }; sf::Vector2f normal2 = { -dir2.y, dir2.x };
                sf::Vector2f miterNormal;
                if (len1 > 1e-6f && len2 > 1e-6f && std::abs(dir1.x * dir2.x + dir1.y * dir2.y) < 0.999f) {
                    miterNormal = (normal1 + normal2) / 2.f; float miterLen = std::hypot(miterNormal.x, miterNormal.y);
                    if (miterLen > 1e-6f) { float dot = (dir1.x * dir2.x + dir1.y * dir2.y); float angleRad = std::acos(std::clamp(dot, -1.0f, 1.0f)); float scale = 1.0f / std::max(0.2f, std::cos(angleRad / 2.0f)); miterNormal = (miterNormal / miterLen) * std::min(scale, 3.0f); }
                    else { miterNormal = normal1; }
                }
                else if (len2 > 1e-6f) { miterNormal = normal2; }
                else if (len1 > 1e-6f) { miterNormal = normal1; }
                else { miterNormal = { 0.f, -1.f }; }
                sf::Vector2f vertexA = currPos + miterNormal * halfThickness; sf::Vector2f vertexB = currPos - miterNormal * halfThickness;
                thickLineStrip[i * 2 + 0].position = vertexA; thickLineStrip[i * 2 + 1].position = vertexB;
                sf::Color vertColor = currentValid ? m_currentTheme.dragLine : sf::Color::Red;
                thickLineStrip[i * 2 + 0].color = vertColor; thickLineStrip[i * 2 + 1].color = vertColor;
            }
            m_window.draw(thickLineStrip);
        }

        // Draw rubber band segment
        sf::Vector2f mouseEndPos = m_window.mapPixelToCoords(sf::Mouse::getPosition(m_window)); // Get current mouse pos
        int lastLetterIdx = m_path.back();
        sf::Vector2f lastLetterPos = { m_wheelX, m_wheelY }; bool lastLetterValid = false;
        if (lastLetterIdx >= 0 && lastLetterIdx < m_wheelCentres.size()) { lastLetterPos = m_wheelCentres[lastLetterIdx]; lastLetterValid = true; }
        sf::Vector2f segmentDir = mouseEndPos - lastLetterPos; float segmentLen = std::hypot(segmentDir.x, segmentDir.y);
        sf::Vector2f segmentNormal = { 0, 0 };
        if (segmentLen > 1e-6f) { segmentDir /= segmentLen; segmentNormal = { -segmentDir.y, segmentDir.x }; }
        sf::VertexArray rubberBandStrip(stripType, 4);
        rubberBandStrip[0].position = lastLetterPos + segmentNormal * halfThickness; rubberBandStrip[1].position = lastLetterPos - segmentNormal * halfThickness;
        rubberBandStrip[2].position = mouseEndPos + segmentNormal * halfThickness; rubberBandStrip[3].position = mouseEndPos - segmentNormal * halfThickness;
        sf::Color finalSegmentColor = lastLetterValid ? m_currentTheme.dragLine : sf::Color::Red;
        rubberBandStrip[0].color = finalSegmentColor; rubberBandStrip[1].color = finalSegmentColor; rubberBandStrip[2].color = finalSegmentColor; rubberBandStrip[3].color = finalSegmentColor;
        m_window.draw(rubberBandStrip);
    }



    for (std::size_t i = 0; i < m_base.size(); ++i) {
        if (i >= m_wheelCentres.size()) continue;
        bool isHilited = std::find(m_path.begin(), m_path.end(), static_cast<int>(i)) != m_path.end();
        sf::CircleShape letterCircle(LETTER_R);
        letterCircle.setOrigin({ LETTER_R, LETTER_R });
        letterCircle.setPosition(m_wheelCentres[i]);
        letterCircle.setFillColor(isHilited ? m_currentTheme.letterCircleHighlight : m_currentTheme.letterCircleNormal);
        letterCircle.setOutlineThickness(2.f);
        letterCircle.setOutlineColor(isHilited ? sf::Color::White : sf::Color(50, 50, 50, 200));
        m_window.draw(letterCircle);

        sf::Text chTxt(m_font, std::string(1, static_cast<char>(std::toupper(m_base[i]))), 25);
        chTxt.setFillColor(isHilited ? m_currentTheme.letterTextHighlight : m_currentTheme.letterTextNormal);
        sf::FloatRect txtBounds = chTxt.getLocalBounds();
        chTxt.setOrigin({ txtBounds.position.x + txtBounds.size.x / 2.f, txtBounds.position.y + txtBounds.size.y / 2.f });
        chTxt.setPosition(m_wheelCentres[i]);
        m_window.draw(chTxt);
    }

    //------------------------------------------------------------
    //  Draw UI Buttons / Hover
    //------------------------------------------------------------
    if (m_gameState == GState::Playing) {
        bool scrambleHover = m_scrambleSpr->getGlobalBounds().contains(mousePos);
        m_scrambleSpr->setColor(scrambleHover ? sf::Color::White : sf::Color(255, 255, 255, 200));
        m_window.draw(*m_scrambleSpr);

        bool hintHover = m_hintSpr->getGlobalBounds().contains(mousePos);
        sf::Color hintColor = (m_hintsAvailable > 0) ? (hintHover ? sf::Color::White : sf::Color(255, 255, 255, 200)) : sf::Color(128, 128, 128, 150);
        m_hintSpr->setColor(hintColor);
        m_window.draw(*m_hintSpr);
    }

    //------------------------------------------------------------
    //  Draw HUD
    //------------------------------------------------------------
    float hudY = m_wheelY + WHEEL_R + HUD_TEXT_OFFSET_Y;
    // Guess Text
    if (m_gameState == GState::Playing && !m_currentGuess.empty()) {
        sf::Text guessTxt(m_font, "Guess: " + m_currentGuess, 20); // Create locally
        guessTxt.setFillColor(m_currentTheme.hudTextGuess);
        sf::FloatRect guessBounds = guessTxt.getLocalBounds();
        guessTxt.setOrigin({ guessBounds.position.x + guessBounds.size.x / 2.f, guessBounds.position.y });
        guessTxt.setPosition({ m_wheelX, hudY });
        m_window.draw(guessTxt);
        hudY += HUD_LINE_SPACING;
    }
    // Found Text
    std::string foundCountStr = "Found: " + std::to_string(m_found.size()) + "/" + std::to_string(m_solutions.size());
    sf::Text foundTxt(m_font, foundCountStr, 20); // Create locally
    foundTxt.setFillColor(m_currentTheme.hudTextFound);
    sf::FloatRect foundBounds = foundTxt.getLocalBounds();
    foundTxt.setOrigin({ foundBounds.position.x + foundBounds.size.x / 2.f, foundBounds.position.y });
    foundTxt.setPosition({ m_wheelX, hudY });
    m_window.draw(foundTxt);

    // Hint Count Text
    m_hintCountTxt->setString("Hints: " + std::to_string(m_hintsAvailable)); // Use member text object
    m_hintCountTxt->setFillColor(m_currentTheme.hudTextFound);
    sf::FloatRect hintTxtBounds = m_hintCountTxt->getLocalBounds();
    m_hintCountTxt->setOrigin({ hintTxtBounds.position.x + hintTxtBounds.size.x / 2.f, hintTxtBounds.position.y });
    sf::FloatRect hintSprBounds = m_hintSpr->getGlobalBounds();
    float hintIconCenterX = m_hintSpr->getPosition().x - hintSprBounds.size.x / 2.f;
    float hintIconBottomY = m_hintSpr->getPosition().y + hintSprBounds.size.y / 2.f;
    m_hintCountTxt->setPosition({ hintIconCenterX - 10.f, hintIconBottomY + 5.f });
    m_window.draw(*m_hintCountTxt);

    //------------------------------------------------------------
    //  Draw Solved State overlay
    //------------------------------------------------------------
    if (m_currentScreen == GameScreen::GameOver) { // Check screen state
        // Prepare Elements
        sf::Text winTxt(m_font, "Puzzle Solved!", 26); // Create locally
        winTxt.setFillColor(m_currentTheme.hudTextSolved);
        winTxt.setStyle(sf::Text::Bold);

        // Calculate bounds and required size
        sf::FloatRect winTxtBounds = winTxt.getLocalBounds();
        sf::Vector2f contBtnSize = m_contBtn.getSize();
        const float padding = 25.f; const float spacing = 20.f;
        float overlayWidth = std::max(winTxtBounds.size.x, contBtnSize.x) + 2.f * padding;
        float overlayHeight = winTxtBounds.size.y + contBtnSize.y + spacing + 2.f * padding;

        // Set up the overlay background
        m_solvedOverlay.setSize({ overlayWidth, overlayHeight });
        m_solvedOverlay.setRadius(15.f);
        m_solvedOverlay.setFillColor(m_currentTheme.solvedOverlayBg);
        m_solvedOverlay.setOrigin({ overlayWidth / 2.f, overlayHeight / 2.f });
        sf::Vector2f windowCenter = sf::Vector2f(m_window.getSize()) / 2.f;
        m_solvedOverlay.setPosition(windowCenter);

        // Position elements centered within the overlay
        float winTxtCenterY = windowCenter.y - overlayHeight / 2.f + padding + (winTxtBounds.position.y + winTxtBounds.size.y / 2.f);
        float contBtnPosY = winTxtCenterY + (winTxtBounds.size.y / 2.f) + spacing;
        winTxt.setOrigin({ winTxtBounds.position.x + winTxtBounds.size.x / 2.f, winTxtBounds.position.y + winTxtBounds.size.y / 2.f });
        winTxt.setPosition({ windowCenter.x, winTxtCenterY });
        m_contBtn.setOrigin({ contBtnSize.x / 2.f, 0.f });
        m_contBtn.setPosition({ windowCenter.x, contBtnPosY });
        sf::FloatRect contTxtBounds = m_contTxt->getLocalBounds();
        m_contTxt->setOrigin({ contTxtBounds.position.x + contTxtBounds.size.x / 2.f, contTxtBounds.position.y + contTxtBounds.size.y / 2.f });
        m_contTxt->setPosition(m_contBtn.getPosition() + sf::Vector2f{ 0.f, contBtnSize.y / 2.f });

        // Handle Continue Button Hover
        bool contHover = m_contBtn.getGlobalBounds().contains(mousePos);
        sf::Color continueHoverColor = adjustColorBrightness(m_currentTheme.continueButton, 1.2f);
        m_contBtn.setFillColor(contHover ? continueHoverColor : m_currentTheme.continueButton);

        // Draw the overlay elements
        m_window.draw(m_solvedOverlay);
        m_window.draw(winTxt);
        m_window.draw(m_contBtn);
        m_window.draw(*m_contTxt);
    }
}