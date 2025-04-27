#include "Theme.h"
#include <utility>
#include <cstdint>
#include "ThemeData.h"
#include "GameData.h"
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
#include <random> // For std::mt19937, std::uniform_real_distribution, etc.
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
    m_celebrationEffectTimer(0.f),
    m_currentScreen(GameScreen::MainMenu),
    m_gameState(GState::Playing),
    m_hintsAvailable(INITIAL_HINTS),         // Use constant directly here
    m_wordsSolvedSinceHint(0),
    m_currentScore(0),
    m_scoreFlourishTimer(0.f),
    m_dragging(false),
    m_decor(10),                             // Initialize DecorLayer
    m_selectedDifficulty(DifficultyLevel::None),
    m_puzzlesPerSession(0),
    m_currentPuzzleIndex(0),
    m_isInSession(false),
    // Resource Handles (default construct textures/buffers - loaded later)
    m_scrambleTex(), m_hintTex(), m_sapphireTex(), m_rubyTex(), m_diamondTex(),
    m_selectBuffer(), m_placeBuffer(), m_winBuffer(), m_clickBuffer(), m_hintUsedBuffer(), 
    // Sounds (default construct - buffer set later)
    m_selectSound(), m_placeSound(), m_winSound(), m_clickSound(), m_hintUsedSound(), m_errorWordSound(),
    // Music (default construct - file loaded later)
    m_backgroundMusic(),
    // Sprites (default construct - texture set later)
    m_scrambleSpr(), m_hintSpr(), m_sapphireSpr(), m_rubySpr(), m_diamondSpr(),
    // UI Shapes (can use constructor directly)
    m_contBtn({ 200.f, 50.f }, 10.f, 10),
    m_solvedOverlay(),
    m_scoreBar(),
    m_guessDisplay_Text(),   
    m_guessDisplay_Bg(),  
    m_needsLayoutUpdate(false), 
    m_lastKnownSize(0, 0),
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
    m_returnButtonShape({ 250.f, 50.f }, 10.f, 10),
    m_progressMeterBg(),     
    m_progressMeterFill(),   
    m_progressMeterText()   
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
    m_errorWordSound = std::make_unique<sf::Sound>(m_errorWordBuffer);

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
    m_fullWordList = Words::loadWordListWithRarity("words.csv");
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

}


// --- Process Events (Placeholder) ---
void Game::m_processEvents() {
    // m_needsLayoutUpdate initialized to false in constructor

    while (const std::optional optEvent = m_window.pollEvent()) {
        const sf::Event& event = *optEvent;

        // --- Global Event Handling ---

        if (event.is<sf::Event::Closed>()) {
            m_window.close();
            return;
        }

        if (const auto* rs = event.getIf<sf::Event::Resized>()) {
            unsigned int currentWidth = rs->size.x;
            unsigned int currentHeight = rs->size.y;

            // Determine the target size based on minimums
            unsigned int targetWidth = std::max(currentWidth, static_cast<unsigned int>(MIN_WINDOW_WIDTH));
            unsigned int targetHeight = std::max(currentHeight, static_cast<unsigned int>(MIN_WINDOW_HEIGHT));
            sf::Vector2u targetSize = { targetWidth, targetHeight }; // The size we WANT

            bool wasClamped = (currentWidth < MIN_WINDOW_WIDTH || currentHeight < MIN_WINDOW_HEIGHT);

            // --- Check if Internal State/View/Layout Needs Update ---
            if (m_lastKnownSize != targetSize) {
                std::cout << "DEBUG: Target size [" << targetSize.x << "," << targetSize.y
                    << "] differs from last known [" << m_lastKnownSize.x << "," << m_lastKnownSize.y
                    << "]. Updating internal state." << std::endl;

                // *** Attempt visual resize ONLY if clamping occurred AND target changed ***
                // This is the key change: Only call setSize when the *target* changes while clamping
                if (wasClamped) {
                    std::cout << "DEBUG: Window too small, attempting resize ONCE (for this target) to " << targetSize.x << "x" << targetSize.y << std::endl;
                    m_window.setSize(targetSize);
                    // Still don't rely on visual success immediately
                }
                // **********************************************************************


                // Update the size we will use for view and layout
                m_lastKnownSize = targetSize;
                m_needsLayoutUpdate = true; // Mark for update after event loop

                // Set the view immediately based on the *new* TARGET size
                sf::FloatRect visibleArea({ 0.f, 0.f }, { static_cast<float>(m_lastKnownSize.x), static_cast<float>(m_lastKnownSize.y) });
                m_window.setView(sf::View(visibleArea));
                std::cout << "DEBUG: View updated to " << m_lastKnownSize.x << "x" << m_lastKnownSize.y << std::endl;

            }
            // --- End internal state update check ---

        } // --- End Resized Handling ---


        // --- Screen-Specific Event Handling ---
        if (m_window.isOpen()) {
            if (m_currentScreen == GameScreen::MainMenu) { m_handleMainMenuEvents(event); }
            else if (m_currentScreen == GameScreen::CasualMenu) { m_handleCasualMenuEvents(event); }
            else if (m_currentScreen == GameScreen::SessionComplete) { m_handleSessionCompleteEvents(event); } 
            else if (m_currentScreen == GameScreen::Playing) { m_handlePlayingEvents(event); }
            else if (m_currentScreen == GameScreen::GameOver) { m_handleGameOverEvents(event); }
        }

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
        if (m_scoreFlourishTimer < 0.f) {
            m_scoreFlourishTimer = 0.f;
        }
    }
    // ---------------------------------

    // Update game elements based on screen
    if (m_currentScreen == GameScreen::Playing || m_currentScreen == GameScreen::GameOver)
    {
        m_updateAnims(deltaSeconds);
    }
    else if (m_currentScreen == GameScreen::SessionComplete) // <<< ADD THIS
    {
        m_updateCelebrationEffects(deltaSeconds);
    }
}

// --- Render ---
void Game::m_render() {
    m_window.clear(m_currentTheme.winBg);
    m_decor.draw(m_window); // Draw background decor first

    // Get mouse position once for hover checks within render helpers
    sf::Vector2f mpos = m_window.mapPixelToCoords(sf::Mouse::getPosition(m_window));

    // --- Draw based on current screen ---
    if (m_currentScreen == GameScreen::MainMenu) { m_renderMainMenu(mpos); }
    else if (m_currentScreen == GameScreen::CasualMenu) { m_renderCasualMenu(mpos); }
    else if (m_currentScreen == GameScreen::SessionComplete) { m_renderSessionComplete(mpos); } 
    else { m_renderGameScreen(mpos); }
    

    // TODO: Draw Pop-ups last if needed

    m_window.display(); // Display everything drawn
}

// --- Other Private Methods (Placeholders) ---
void Game::m_rebuild() {
    // Select Random Theme (Keep this)
    if (!m_themes.empty()) {
        m_currentTheme = m_themes[randRange<std::size_t>(0, m_themes.size() - 1)];
	}
	else {
		std::cerr << "CRITICAL Error: No themes available. Exiting.\n";
		exit(1);
	}
   

    // --- Base Word Selection (Revised Logic) ---
    std::string selectedBaseWord = "";
    bool baseWordFound = false;
    std::vector<WordInfo> preCalculatedFilteredSolutions; // To store results if ideal word found

    if (m_roots.empty()) {
        std::cerr << "Error: No root words available for puzzle generation.\n";
        m_base = "ERROR";
    }
    else {
        // Get the specific base word criteria for this puzzle
        PuzzleCriteria baseCriteria = m_getCriteriaForCurrentPuzzle();
        // Get the sub-word filtering criteria for this difficulty
        std::vector<int> allowedSubRarities;
        int minSubLength = MIN_WORD_LENGTH;
        int maxSolutions = 0; // We still need maxSolutions for final truncation
        switch (m_selectedDifficulty) {
        case DifficultyLevel::Easy:   allowedSubRarities = { 1, 2 }; maxSolutions = EASY_MAX_SOLUTIONS; minSubLength = MIN_WORD_LENGTH; break;
        case DifficultyLevel::Medium: allowedSubRarities = { 1, 2, 3 }; maxSolutions = MEDIUM_MAX_SOLUTIONS; minSubLength = MIN_WORD_LENGTH; break;
        case DifficultyLevel::Hard:   allowedSubRarities = { 2, 3, 4 }; maxSolutions = HARD_MAX_SOLUTIONS; minSubLength = HARD_MIN_WORD_LENGTH; break;
        default:                      allowedSubRarities = { 1, 2, 3, 4 }; maxSolutions = 999; minSubLength = MIN_WORD_LENGTH; break;
        }

        std::cout << "Rebuilding Puzzle " << (m_currentPuzzleIndex + 1) << "/" << m_puzzlesPerSession
            << " (Difficulty: " << static_cast<int>(m_selectedDifficulty) << ")" << std::endl;

        // Shuffle roots for variety
        std::shuffle(m_roots.begin(), m_roots.end(), Rng());

        int bestFallbackIndex = -1; // Index of the first word meeting base criteria

        // --- Attempt 1: Find IDEAL match (meets base criteria AND sub-word count) ---
        std::cout << "DEBUG: Searching for IDEAL base word..." << std::endl;
        for (std::size_t i = 0; i < m_roots.size(); ++i) {
            const auto& rootInfo = m_roots[i];

            // Check Base Length
            bool lengthMatch = false;
            for (int len : baseCriteria.allowedLengths) { if (rootInfo.text.length() == len) { lengthMatch = true; break; } }
            if (!lengthMatch) continue;

            // Check Base Rarity
            bool rarityMatch = false;
            for (int rarity : baseCriteria.allowedRarities) { if (rootInfo.rarity == rarity) { rarityMatch = true; break; } }
            if (!rarityMatch) continue;

            // --- Base Criteria Met ---
            std::cout << "  -> Candidate '" << rootInfo.text << "' meets base criteria." << std::endl;

            // Store as potential fallback if none better found yet
            if (bestFallbackIndex == -1) {
                bestFallbackIndex = static_cast<int>(i);
                std::cout << "     (Storing as initial fallback candidate)" << std::endl;
            }

            // --- Check Sub-word Count (only if length is >= 5, or always for stricter control) ---
            if (rootInfo.text.length() >= 5) { // Check sub-words for longer base words
                std::cout << "     Checking sub-word count..." << std::endl;
                std::vector<WordInfo> temp_sub_solutions = Words::subWords(rootInfo.text, m_fullWordList);
                std::vector<WordInfo> filtered_sub_solutions;

                // Filter these sub-words based on difficulty criteria
                for (const auto& subInfo : temp_sub_solutions) {
                    if (subInfo.text.length() < minSubLength) continue;
                    bool subRarityMatch = false;
                    for (int subRarity : allowedSubRarities) { if (subInfo.rarity == subRarity) { subRarityMatch = true; break; } }
                    if (subRarityMatch) {
                        filtered_sub_solutions.push_back(subInfo);
                    }
                }
                std::cout << "     Generated " << filtered_sub_solutions.size() << " valid sub-words." << std::endl;

                // Check if it meets the desired minimum count
                if (filtered_sub_solutions.size() >= MIN_DESIRED_GRID_WORDS) {
                    std::cout << "     >>> IDEAL Candidate Found! Selecting '" << rootInfo.text << "'" << std::endl;
                    selectedBaseWord = rootInfo.text;
                    baseWordFound = true;
                    // Store the already filtered list (we'll still truncate it later)
                    preCalculatedFilteredSolutions = std::move(filtered_sub_solutions);
                    break; // Found ideal word, exit search loop
                }
                else {
                    std::cout << "     (Not enough sub-words, continuing search for ideal)" << std::endl;
                }
            }
            else {
                std::cout << "     (Base word length < 5, not checking sub-word count for ideal criteria)" << std::endl;
            }
        } // --- End Ideal Search Loop ---


        // --- Attempt 2: Use Best Fallback (if no ideal found) ---
        if (!baseWordFound && bestFallbackIndex != -1) {
            std::cout << "DEBUG: No IDEAL word found. Using best fallback candidate: '" << m_roots[bestFallbackIndex].text << "'" << std::endl;
            selectedBaseWord = m_roots[bestFallbackIndex].text;
            baseWordFound = true;
            // Ensure preCalculated list is empty so sub-words are recalculated later
            preCalculatedFilteredSolutions.clear();
        }


        // --- Attempt 3: Broad Fallback (if still no word found) ---
        if (!baseWordFound) {
            std::cerr << "Warning: No base word found matching strict or fallback criteria. Applying BROAD fallback." << std::endl;
            PuzzleCriteria broadFallbackCriteria;
            broadFallbackCriteria.allowedLengths = { 4, 5, 6, 7 }; // Any reasonable length
            broadFallbackCriteria.allowedRarities = { 1, 2, 3, 4 }; // Any rarity
            bestFallbackIndex = -1; // Reset for broad search

            for (std::size_t i = 0; i < m_roots.size(); ++i) { // Iterate again
                const auto& rootInfo = m_roots[i];
                bool lengthMatch = false;
                for (int len : broadFallbackCriteria.allowedLengths) { if (rootInfo.text.length() == len) { lengthMatch = true; break; } }
                if (!lengthMatch) continue;
                // Rarity always matches {1,2,3,4}
                selectedBaseWord = rootInfo.text;
                baseWordFound = true;
                std::cout << "Found base word (Broad Fallback): " << selectedBaseWord << std::endl;
                preCalculatedFilteredSolutions.clear(); // Ensure recalculation
                break;
            }
        }


        // Final Safety Net
        if (!baseWordFound) {
            std::cerr << "CRITICAL FALLBACK: Using first available root word." << std::endl;
            if (!m_roots.empty()) {
                selectedBaseWord = m_roots[0].text;
                baseWordFound = true;
                preCalculatedFilteredSolutions.clear(); // Ensure recalculation
            }
            else {
                selectedBaseWord = "ERROR";
            }
        }
        m_base = selectedBaseWord; // Set the final selected base word
    }
    // --- End Base Word Selection ---


    // Scramble the selected base word letters
    std::shuffle(m_base.begin(), m_base.end(), Rng());


    // --- Sub-word Processing (using selected m_base) ---
    std::vector<WordInfo> final_solutions; // This will become m_solutions

    if (m_base != "ERROR") {
        // Calculate *all* potential solutions for bonus checking later
        m_allPotentialSolutions = Words::subWords(m_base, m_fullWordList);

        // Did we already calculate filtered solutions during ideal word search?
        if (!preCalculatedFilteredSolutions.empty()) {
            std::cout << "DEBUG: Using pre-calculated filtered solutions." << std::endl;
            final_solutions = std::move(preCalculatedFilteredSolutions); // Use the stored list
        }
        else {
            std::cout << "DEBUG: Filtering all potential solutions now." << std::endl;
            // Filter m_allPotentialSolutions based on sub-word criteria
            std::vector<int> allowedSubRarities; // Recalculate criteria needed
            int minSubLength = MIN_WORD_LENGTH;
            switch (m_selectedDifficulty) {
            case DifficultyLevel::Easy:   allowedSubRarities = { 1, 2 }; minSubLength = MIN_WORD_LENGTH; break;
            case DifficultyLevel::Medium: allowedSubRarities = { 1, 2, 3 }; minSubLength = MIN_WORD_LENGTH; break;
            case DifficultyLevel::Hard:   allowedSubRarities = { 2, 3, 4 }; minSubLength = HARD_MIN_WORD_LENGTH; break;
            default:                      allowedSubRarities = { 1, 2, 3, 4 }; minSubLength = MIN_WORD_LENGTH; break;
            }

            for (const auto& subInfo : m_allPotentialSolutions) {
                if (subInfo.text.length() < minSubLength) continue;
                bool subRarityMatch = false;
                for (int subRarity : allowedSubRarities) { if (subInfo.rarity == subRarity) { subRarityMatch = true; break; } }
                if (subRarityMatch) {
                    final_solutions.push_back(subInfo);
                }
            }
        }

        // Truncate the filtered list if necessary (get maxSolutions again)
        int maxSolutions = 0;
        switch (m_selectedDifficulty) {
        case DifficultyLevel::Easy:   maxSolutions = EASY_MAX_SOLUTIONS; break;
        case DifficultyLevel::Medium: maxSolutions = MEDIUM_MAX_SOLUTIONS; break;
        case DifficultyLevel::Hard:   maxSolutions = HARD_MAX_SOLUTIONS; break;
        default:                      maxSolutions = 999; break;
        }

        if (final_solutions.size() > maxSolutions) {
            std::cout << "DEBUG: Truncating final solutions from " << final_solutions.size() << " to " << maxSolutions << std::endl;
            // Sort before truncating
            std::sort(final_solutions.begin(), final_solutions.end(),
                [](const WordInfo& a, const WordInfo& b) {
                    if (a.rarity != b.rarity) return a.rarity < b.rarity;
                    return a.text.length() < b.text.length();
                });
            final_solutions.resize(maxSolutions);
        }

    }
    else {
        // Handle the case where m_base was "ERROR"
        m_allPotentialSolutions.clear();
        final_solutions.clear();
    }

    m_solutions = final_solutions;
    m_sorted = Words::sortForGrid(m_solutions); // Sort this final list for the grid
    // --- END: Sub-word Filtering and Truncation ---

    // Debug print after processing sub-words
    std::cout << "DEBUG: m_rebuild - m_base: " << m_base << ", FINAL m_solutions count: " << m_solutions.size() << ", m_sorted count: " << m_sorted.size() << std::endl;
    if (!m_sorted.empty()) {
        std::cout << "DEBUG: m_rebuild - First sorted word after filter/truncate: '" << m_sorted[0].text << "'" << std::endl;
    }

    // Setup Grid (uses the final m_sorted)
    m_grid.assign(m_sorted.size(), {});
    for (std::size_t i = 0; i < m_sorted.size(); ++i) {
        if (!m_sorted[i].text.empty()) {
            // Assuming m_grid is vector<string> or vector<vector<char>>
            // Resize the inner string/vector and fill with '_'
            m_grid[i].assign(m_sorted[i].text.length(), '_');
        }
        else {
            // Handle potential empty words in the sorted list (optional but safe)
            m_grid[i].clear();
        }
    }


    // Reset Game State Variables *for the puzzle* (Keep this)
    m_found.clear();
    m_foundBonusWords.clear();
    m_anims.clear();
    m_scoreAnims.clear();
    m_clearDragState();
    m_gameState = GState::Playing;

    // DO NOT reset score: m_currentScore persists across puzzles in a session
    // DO NOT reset hints: m_hintsAvailable persists

    // If it's the *first* puzzle of a session, reset hint counters
    if (m_currentPuzzleIndex == 0 && m_isInSession) {
        m_hintsAvailable = INITIAL_HINTS; // Reset hints at start of session
        m_wordsSolvedSinceHint = 0;
    }

    // Update score display (score might be non-zero from previous puzzle)
    if (m_scoreValueText) m_scoreValueText->setString(std::to_string(m_currentScore));


    // IMPORTANT: Calculate layout based on the new data (Keep this)
    m_updateLayout(m_window.getSize());

    // Start Background Music
    m_backgroundMusic.stop();
    if (!m_musicFiles.empty()) {
        std::string musicPath = m_musicFiles[randRange<std::size_t>(0, m_musicFiles.size() - 1)];
        if (m_backgroundMusic.openFromFile(musicPath)) {
            m_backgroundMusic.setLooping(true);
            //m_backgroundMusic.play();
        }
        else {
            std::cerr << "Error loading music file: " << musicPath << std::endl;
        }
    }
} // End m_rebuild
// In Game.cpp

void Game::m_updateLayout(sf::Vector2u windowSize) {
    // Get current window size from the member variable
    sf::Vector2u winSize = windowSize;
    sf::Vector2f windowCenter = sf::Vector2f(winSize) / 2.f;

    // --- Progress Meter Positioning --- (Add this section near the top)
    float meterX = windowCenter.x; // Center horizontally
    float meterY = PROGRESS_METER_TOP_MARGIN + PROGRESS_METER_HEIGHT / 2.f; // Center vertically based on top margin

    // Background
    m_progressMeterBg.setSize({ PROGRESS_METER_WIDTH, PROGRESS_METER_HEIGHT });
    m_progressMeterBg.setOrigin({ PROGRESS_METER_WIDTH / 2.f, PROGRESS_METER_HEIGHT / 2.f });
    m_progressMeterBg.setPosition({ meterX, meterY });
    // Outline/Fill colors will be set in render based on theme

    // Fill (Position set relative to Bg in render, size calculated in render)
    m_progressMeterFill.setOrigin({ 0.f, PROGRESS_METER_HEIGHT / 2.f }); // Origin at left-center
    m_progressMeterFill.setPosition({ meterX - PROGRESS_METER_WIDTH / 2.f, meterY }); // Initial position at left edge of Bg

    // Text (Optional - Position set relative to Bg in render)
    if (m_progressMeterText) {
        // Set origin for centering later in render
        // Origin set dynamically in render based on text bounds
    }
    // --- End Progress Meter Positioning ---


    // --- Score Bar ---
    float scoreBarWidth = 250.f;
    float scoreBarY = PROGRESS_METER_TOP_MARGIN + PROGRESS_METER_HEIGHT + SCORE_BAR_TOP_MARGIN + SCORE_BAR_HEIGHT / 2.f;
    m_scoreBar.setSize({ scoreBarWidth, SCORE_BAR_HEIGHT });
    m_scoreBar.setRadius(10.f);
    m_scoreBar.setOrigin({ scoreBarWidth / 2.f, SCORE_BAR_HEIGHT / 2.f });
    m_scoreBar.setPosition({ windowCenter.x, scoreBarY });

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
    m_gridStartY = scoreBarY + SCORE_BAR_HEIGHT / 2.f + GRID_TOP_MARGIN;

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
        float gridMaxY = m_wheelY - WHEEL_R - LETTER_R - GRID_WHEEL_GAP; // Uses m_wheelY
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

    // --- Guess Display Positioning (Above Wheel) ---
    if (m_guessDisplay_Text) {
        // We'll set the exact position and background size in render based on text content
        // For now, just note the general area
        // float guessY = m_wheelY - WHEEL_R - LETTER_R - GUESS_DISPLAY_GAP; // Define GUESS_DISPLAY_GAP in Constants.h
        // m_guessDisplay_Text->setPosition({ m_wheelX, guessY }); // Center X, Y above wheel
        // m_guessDisplay_Bg.setPosition({ m_wheelX, guessY }); // Center X, Y above wheel
        // Origins will be set in render
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
            bool gridMatchFound = false;    // Did we find a NEW word for the grid this turn?
            bool bonusMatchFound = false;   // Did we find a NEW bonus word this turn?
            bool textMatchedExisting = false; // Did the text match a word already found (grid or bonus)?
            int foundWordIdx = -1;        // Index *in m_sorted* if it's a new grid word

            std::cout << "DEBUG: Mouse Released. Guess: '" << m_currentGuess << "'" << std::endl; // Start Debug

            // --- Check 1: Does the guess match text of a GRID word? ---
            for (std::size_t w = 0; w < m_sorted.size(); ++w) {
                const std::string& solutionOriginalCase = m_sorted[w].text;
                std::string solutionUpper = solutionOriginalCase;
                std::transform(solutionUpper.begin(), solutionUpper.end(), solutionUpper.begin(),
                    [](unsigned char c) { return std::toupper(c); });

                if (solutionUpper == m_currentGuess) { // Text matches a grid word!
                    bool alreadyFoundOnGrid = (m_found.find(solutionOriginalCase) != m_found.end());

                    if (!alreadyFoundOnGrid) {
                        // --- Handle NEW Grid Word Found ---
                        std::cout << "DEBUG: Found NEW match on GRID: '" << solutionOriginalCase << "'" << std::endl;
                        gridMatchFound = true; // Mark that we found a new grid word this turn
                        foundWordIdx = static_cast<int>(w);
                        m_found.insert(solutionOriginalCase);         // Add to grid set

                        // --- Handle scoring, hints, animations for GRID WORD --- Start Full Block ---
                        int baseScore = static_cast<int>(m_currentGuess.length()) * 10;
                        int rarityBonus = (m_sorted[w].rarity > 1) ? (m_sorted[w].rarity * 25) : 0;
                        m_currentScore += baseScore + rarityBonus;
                        if (m_scoreValueText) m_scoreValueText->setString(std::to_string(m_currentScore));

                        // Check for earning a hint
                        m_wordsSolvedSinceHint++;
                        if (m_wordsSolvedSinceHint >= WORDS_PER_HINT) {
                            m_hintsAvailable++;
                            m_wordsSolvedSinceHint = 0; // Reset counter
                            if (m_hintCountTxt) m_hintCountTxt->setString("Hints: " + std::to_string(m_hintsAvailable));
                            // TODO: Maybe add a small visual/audio cue for earning a hint?
                        }

                        // --- Create letter animations TO GRID ---
                        for (std::size_t c = 0; c < m_currentGuess.length(); ++c) {
                            if (c < m_path.size()) { // Ensure path index is valid
                                int pathNodeIdx = m_path[c];
                                if (pathNodeIdx >= 0 && pathNodeIdx < m_wheelCentres.size()) {
                                    sf::Vector2f startPos = m_wheelCentres[pathNodeIdx];
                                    sf::Vector2f endPos = m_tilePos(foundWordIdx, static_cast<int>(c));
                                    endPos.x += TILE_SIZE / 2.f; // Center of tile
                                    endPos.y += TILE_SIZE / 2.f;
                                    // Use aggregate initialization for LetterAnim
                                    m_anims.push_back({
                                        m_currentGuess[c],    // ch
                                        startPos,             // start
                                        endPos,               // end
                                        0.f - (c * 0.03f),    // t (Slight delay based on index)
                                        foundWordIdx,         // wordIdx
                                        static_cast<int>(c),  // charIdx
                                        AnimTarget::Grid      // target
                                        });
                                }
                            }
                        } // --- End letter animations ---

                        std::cout << "GRID Word: " << m_currentGuess << " | Rarity: " << m_sorted[foundWordIdx].rarity << " | Len: " << m_currentGuess.length() << " | Rarity Bonus: " << rarityBonus << " | BasePts: " << baseScore << " | Total: " << m_currentScore << std::endl;

                        // --- Check for Puzzle Solved ---
                        // Compare count of found words (original case) with total solutions ON GRID
                        if (m_found.size() == m_solutions.size()) {
                            std::cout << "DEBUG: All grid words found! Puzzle solved." << std::endl;
                            if (m_winSound) m_winSound->play();
                            m_gameState = GState::Solved; // Internal state
                            m_currentScreen = GameScreen::GameOver; // Change screen state
                        }
                        // --- Handle scoring, hints, animations for GRID WORD --- End Full Block ---

                    }
                    else {
                        // Text matched a grid word, but it was already found
                        std::cout << "DEBUG: Matched GRID word '" << solutionOriginalCase << "', but already found." << std::endl;
                        textMatchedExisting = true; // Mark that the text is valid but word already used
                    }
                    // Whether new or already found, if text matched a grid word, no need to check further
                    goto end_word_checks; // Use goto to jump past bonus check cleanly
                }
            } // --- End Grid Check Loop ---


// --- Check 2: Does the guess match text of a BONUS word? ---
            if (!gridMatchFound && !textMatchedExisting) { // Only check if not a grid word (new or existing)
                std::cout << "DEBUG: Checking for BONUS word..." << std::endl;
                for (const auto& potentialWordInfo : m_allPotentialSolutions) {
                    const std::string& bonusWordOriginalCase = potentialWordInfo.text;
                    std::string bonusWordUpper = bonusWordOriginalCase;
                    std::transform(bonusWordUpper.begin(), bonusWordUpper.end(), bonusWordUpper.begin(),
                        [](unsigned char c) { return std::toupper(c); });

                    if (bonusWordUpper == m_currentGuess) { // Text matches a potential bonus word!

                        // *** NEW CHECK: Is this word already found ON THE GRID? ***
                        bool alreadyFoundOnGrid = (m_found.find(bonusWordOriginalCase) != m_found.end());
                        if (alreadyFoundOnGrid) {
                            std::cout << "DEBUG: Matched bonus text '" << bonusWordOriginalCase << "', but it's already on the grid." << std::endl;
                            textMatchedExisting = true; // Mark that the text is valid but word used on grid
                            goto end_word_checks; // Don't treat as bonus or invalid
                        }
                        // **********************************************************


                        // Is this specific word already found *as a bonus word*?
                        bool alreadyFoundBonus = (m_foundBonusWords.find(bonusWordOriginalCase) != m_foundBonusWords.end());

                        if (!alreadyFoundBonus) { // Note: alreadyFoundOnGrid check above ensures it's not a grid word either
                            // --- Handle NEW Bonus Word Found --- START FULL BLOCK ---
                            std::cout << "DEBUG: Found NEW match for BONUS: '" << bonusWordOriginalCase << "'" << std::endl;
                            bonusMatchFound = true; // Mark that we found a new bonus word this turn
                            m_foundBonusWords.insert(bonusWordOriginalCase); // Add to bonus found set ONLY

                            // --- Handle scoring ---
                            int bonusScore = 25; // Or base on length/rarity if desired
                            m_currentScore += bonusScore;
                            if (m_scoreValueText) m_scoreValueText->setString(std::to_string(m_currentScore));
                            std::cout << "BONUS Word: " << m_currentGuess << " | Points: " << bonusScore << " | Total: " << m_currentScore << std::endl;
                            // Note: Bonus words usually don't affect hint earning

                            // --- Trigger Bonus Letter Animations TO SCORE ---
                            std::cout << "DEBUG: Preparing bonus animations..." << std::endl;
                            if (m_scoreValueText) {
                                sf::FloatRect scoreBounds = m_scoreValueText->getGlobalBounds();
                                sf::Vector2f scoreCenterPos = {
                                    scoreBounds.position.x + scoreBounds.size.x / 2.f,
                                    scoreBounds.position.y + scoreBounds.size.y / 2.f
                                };
                                std::cout << "DEBUG: Animating bonus letters to score center: (" << scoreCenterPos.x << "," << scoreCenterPos.y << ")" << std::endl;

                                for (std::size_t c = 0; c < m_currentGuess.length(); ++c) {
                                    if (c < m_path.size()) {
                                        int pathNodeIdx = m_path[c];
                                        if (pathNodeIdx >= 0 && pathNodeIdx < m_wheelCentres.size()) {
                                            sf::Vector2f startPos = m_wheelCentres[pathNodeIdx];
                                            sf::Vector2f endPos = scoreCenterPos;
                                            // Use aggregate initialization for LetterAnim
                                            m_anims.push_back({
                                                m_currentGuess[c],    // ch
                                                startPos,             // start
                                                endPos,               // end
                                                0.f - (c * 0.05f),    // t (Slight delay)
                                                -1,                   // wordIdx (Not used)
                                                -1,                   // charIdx (Not used)
                                                AnimTarget::Score     // target
                                                });
                                        }
                                    }
                                }
                                std::cout << "DEBUG: Bonus animations CREATED." << std::endl;
                            }
                            else {
                                std::cerr << "Warning: Cannot animate bonus to score - m_scoreValueText is null." << std::endl;
                            }
                            // --- End Bonus Animations ---

                            // Optional: Play bonus sound
                            // if(m_bonusSound) m_bonusSound->play();

                            // --- Handle NEW Bonus Word Found --- END FULL BLOCK ---

                        }
                        else {
                            // Text matched a potential bonus word, but it was already found *as bonus*
                            std::cout << "DEBUG: Matched BONUS word '" << bonusWordOriginalCase << "', but already found AS BONUS." << std::endl;
                            textMatchedExisting = true; // Mark that the text is valid but word already used
                        }
                        // Whether new or already found bonus, if text matched, stop checking
                        goto end_word_checks;
                    }
                } // --- End Bonus Check Loop ---
            } // --- End if (!gridMatchFound && !textMatchedExisting) ---

        end_word_checks:; // Label for goto jump target

            // --- Check 3: Incorrect/Already Found Word Handling ---
            if (!gridMatchFound && !bonusMatchFound) { // If no NEW word was found this turn
                if (textMatchedExisting) {
                    // Text matched a word, but it was already found (grid or bonus)
                    std::cout << "Word '" << m_currentGuess << "' already found." << std::endl;
                    // Play "already found" sound?
                    // if (m_alreadyFoundSound) m_alreadyFoundSound->play();
                }
                else {
                    // Text did not match any grid word or any potential bonus word
                    std::cout << "Word '" << m_currentGuess << "' is not valid for this puzzle." << std::endl;
                    // Play incorrect sound?
                    if (m_errorWordSound) m_errorWordSound->play();
                }
            }

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

            // Check Continue Button Click
            if (m_contBtn.getGlobalBounds().contains(mp)) {
                if (m_clickSound) m_clickSound->play();

                // --- Session Handling ---
                if (m_isInSession) {
                    m_currentPuzzleIndex++; // Move to the next puzzle index

                    if (m_currentPuzzleIndex < m_puzzlesPerSession) {
                        // --- Go to Next Puzzle ---
                        std::cout << "DEBUG: Continuing Session - Calling m_rebuild for puzzle " << m_currentPuzzleIndex + 1 << std::endl;
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
void Game::m_renderGameScreen(const sf::Vector2f& mousePos) {

    //------------------------------------------------------------
    //  Draw Progress Meter (If in session)
    //------------------------------------------------------------
    if (m_isInSession) {
        // Apply theme colors
        m_progressMeterBg.setFillColor(sf::Color(50, 50, 50, 150)); // Dark semi-transparent bg (or use theme)
        m_progressMeterBg.setOutlineColor(sf::Color(150, 150, 150)); // Light outline (or use theme)
        m_progressMeterBg.setOutlineThickness(PROGRESS_METER_OUTLINE);
        m_progressMeterFill.setFillColor(sf::Color(0, 180, 0, 200)); // Green fill (or use theme)

        // Calculate fill width
        float progressRatio = 0.f;
        if (m_puzzlesPerSession > 0) { // Avoid division by zero
            // Add 1 to index because it's 0-based
            progressRatio = static_cast<float>(m_currentPuzzleIndex + 1) / static_cast<float>(m_puzzlesPerSession);
        }
        float fillWidth = PROGRESS_METER_WIDTH * progressRatio;
        m_progressMeterFill.setSize({ fillWidth, PROGRESS_METER_HEIGHT });

        // Draw elements
        m_window.draw(m_progressMeterBg);
        m_window.draw(m_progressMeterFill);

        // Draw optional text overlay
        if (m_progressMeterText) {
            std::string progressStr = std::to_string(m_currentPuzzleIndex + 1) + "/" + std::to_string(m_puzzlesPerSession);
            m_progressMeterText->setString(progressStr);
            m_progressMeterText->setFillColor(sf::Color::White); // Or theme color

            // Center text on the meter background
            sf::FloatRect textBounds = m_progressMeterText->getLocalBounds();
            m_progressMeterText->setOrigin({ textBounds.position.x + textBounds.size.x / 2.f,
                                            textBounds.position.y + textBounds.size.y / 2.f });
            m_progressMeterText->setPosition(m_progressMeterBg.getPosition()); // Position at Bg center
            m_window.draw(*m_progressMeterText);
        }
    }

    //------------------------------------------------------------
    //  Draw Score Bar
    //------------------------------------------------------------
    m_scoreBar.setFillColor(m_currentTheme.scoreBarBg);
    m_scoreLabelText->setFillColor(m_currentTheme.scoreTextLabel);
    m_scoreValueText->setFillColor(m_currentTheme.scoreTextValue);
    m_scoreValueText->setString(std::to_string(m_currentScore));

    // --- Apply Score Flourish ---
    float currentScale = 1.0f;
    if (m_scoreFlourishTimer > 0.f) {
        // Optional: Make scale pulse based on timer (e.g., sine wave)
        float pulse = 1.0f + (SCORE_FLOURISH_SCALE - 1.0f) * (std::sin( (SCORE_FLOURISH_DURATION - m_scoreFlourishTimer) * PI * 2.f / SCORE_FLOURISH_DURATION ) * 0.5f + 0.5f) ;
        currentScale = pulse;

        // Simpler: Just scale up while timer is active
        //currentScale = SCORE_FLOURISH_SCALE;
        // Maybe change color too?
        // m_scoreValueText->setFillColor(sf::Color::Yellow);
    }
    else {
        // Reset color if changed during flourish
        // m_scoreValueText->setFillColor(m_currentTheme.scoreTextValue);
    }
    m_scoreValueText->setScale({ currentScale, currentScale }); // Apply scale
    // --- End Flourish ---

    // Re-center score value text
    sf::FloatRect valueBounds = m_scoreValueText->getLocalBounds();
    m_scoreValueText->setOrigin({ 0.f, valueBounds.position.y + valueBounds.size.y / 2.f });
    m_scoreValueText->setPosition({ m_scoreBar.getPosition().x + 5.f, m_scoreBar.getPosition().y });

    m_window.draw(m_scoreBar);
    m_window.draw(*m_scoreLabelText);
    m_window.draw(*m_scoreValueText);
    m_scoreValueText->setScale({ 1.f, 1.f });

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
    sf::Text flyingLetterText(m_font, "", 20);
    flyingLetterText.setFillColor(m_currentTheme.gridLetter); // Default color

    for (const auto& a : m_anims) {
        if (a.t >= 1.f || a.t < 0.f) continue; // Skip finished or delayed animations

        // Change color if targeting score?
        if (a.target == AnimTarget::Score) {
            flyingLetterText.setFillColor(sf::Color::Yellow); // Make bonus letters yellow
        }
        else {
            flyingLetterText.setFillColor(m_currentTheme.gridLetter); // Reset to default for grid
        }

        float eased_t = a.t * a.t * (3.f - 2.f * a.t);
        sf::Vector2f p = a.start + (a.end - a.start) * eased_t;

        flyingLetterText.setString(std::string(1, a.ch));
        sf::FloatRect bounds = flyingLetterText.getLocalBounds();
        flyingLetterText.setOrigin({ bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f });
        flyingLetterText.setPosition(p);
        m_window.draw(flyingLetterText);
    } // --- END OF DRAWING FLYING LETTERS ---


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

    // --- START: Draw Guess Display (Above Wheel) ---
    if (m_gameState == GState::Playing && !m_currentGuess.empty() && m_guessDisplay_Text) {
        m_guessDisplay_Text->setString(m_currentGuess); // Set current guess text

        // Calculate bounds and position
        sf::FloatRect textBounds = m_guessDisplay_Text->getLocalBounds();
        // Add padding for the background
        const float bgPaddingX = 15.f;
        const float bgPaddingY = 5.f;
        sf::Vector2f bgSize = { textBounds.position.x + textBounds.size.x + 2 * bgPaddingX,
                                textBounds.position.y + textBounds.size.y + 2 * bgPaddingY };

        // Position above the wheel (adjust GAP as needed)
        const float GUESS_DISPLAY_GAP = 20.f;
        float guessY = m_wheelY - (WHEEL_R + 30.f) - (bgSize.y / 2.f) - GUESS_DISPLAY_GAP; // Center Y of bg above wheel radius + padding

        // Setup background
        m_guessDisplay_Bg.setSize(bgSize);
        m_guessDisplay_Bg.setRadius(8.f);
        m_guessDisplay_Bg.setFillColor(sf::Color(50, 50, 50, 200));
        m_guessDisplay_Bg.setOutlineColor(sf::Color(150, 150, 150, 200));
        m_guessDisplay_Bg.setOutlineThickness(1.f);
        m_guessDisplay_Bg.setOrigin({ bgSize.x / 2.f, bgSize.y / 2.f });
        m_guessDisplay_Bg.setPosition({ m_wheelX, guessY }); // Pass {x, y}

        // Setup text origin and position (centered within background)
        // Center text origin using local bounds
        m_guessDisplay_Text->setOrigin({ textBounds.position.x + textBounds.size.x / 2.f,
                                        textBounds.position.y + textBounds.size.y / 2.f }); // Pass {x, y}
        m_guessDisplay_Text->setPosition({ m_wheelX, guessY }); // Pass {x, y}
        m_guessDisplay_Text->setFillColor(m_currentTheme.hudTextGuess); // Use theme color

        // Draw
        m_window.draw(m_guessDisplay_Bg);
        m_window.draw(*m_guessDisplay_Text);
    }  // --- END: Draw Guess Display (Above Wheel) ---

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
    /*if (m_gameState == GState::Playing && !m_currentGuess.empty()) {
        sf::Text guessTxt(m_font, "Guess: " + m_currentGuess, 20); // Create locally
        guessTxt.setFillColor(m_currentTheme.hudTextGuess);
        sf::FloatRect guessBounds = guessTxt.getLocalBounds();
        guessTxt.setOrigin({ guessBounds.position.x + guessBounds.size.x / 2.f, guessBounds.position.y });
        guessTxt.setPosition({ m_wheelX, hudY });
        m_window.draw(guessTxt);
        hudY += HUD_LINE_SPACING;
    }*/

    // Found Text
    float bottomHudStartY = m_wheelY + (WHEEL_R + 5.f) + HUD_TEXT_OFFSET_Y; // Start below wheel radius + padding
    float currentBottomHudY = bottomHudStartY; // Use a runner variable
    std::string foundCountStr = "Found: " + std::to_string(m_found.size()) + "/" + std::to_string(m_solutions.size());
    sf::Text foundTxt(m_font, foundCountStr, 20);
    foundTxt.setFillColor(m_currentTheme.hudTextFound);
    sf::FloatRect foundBounds = foundTxt.getLocalBounds();
    foundTxt.setOrigin({ foundBounds.position.x + foundBounds.size.x / 2.f,
                        foundBounds.position.y + foundBounds.size.y / 2.f });
    foundTxt.setPosition({ m_wheelX, currentBottomHudY }); // Use runner Y
    m_window.draw(foundTxt);
    currentBottomHudY += foundTxt.getCharacterSize() + HUD_LINE_SPACING * 0.5f; // Move Y down (adjust spacing)

    // Bonus Word Counter (Position using currentBottomHudY)
    if (!m_allPotentialSolutions.empty() || !m_foundBonusWords.empty()) {
        int totalPossibleBonus = 0;
        for (const auto& pWord : m_allPotentialSolutions) {
            bool isOnGrid = false;
            for (const auto& gWord : m_solutions) { if (pWord.text == gWord.text) { isOnGrid = true; break; } }
            if (!isOnGrid) totalPossibleBonus++;
        }
        // ---
        std::string bonusCountStr = "Bonus Words: " + std::to_string(m_foundBonusWords.size()) + "/" + std::to_string(totalPossibleBonus);
        sf::Text bonusFoundTxt(m_font, bonusCountStr, 18);
        bonusFoundTxt.setFillColor(sf::Color::Yellow);
        sf::FloatRect bonusBounds = bonusFoundTxt.getLocalBounds();
        bonusFoundTxt.setOrigin({ bonusBounds.position.x + bonusBounds.size.x / 2.f,
                                 bonusBounds.position.y + bonusBounds.size.y / 2.f });
        bonusFoundTxt.setPosition({ m_wheelX, currentBottomHudY }); // Use runner Y
        m_window.draw(bonusFoundTxt);
        currentBottomHudY += bonusFoundTxt.getCharacterSize() + HUD_LINE_SPACING * 0.5f; // Move Y down
    }

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

        // --- Draw Score/Bonus Particles ---
        for (auto& scoreAnim : m_scoreAnims) { // Iterate by non-const ref if modifying color
            if (scoreAnim.t < 1.0f) { // Only draw active particles
                // Optional: Apply alpha fade-out based on t?
                sf::Color col = scoreAnim.particle.getFillColor();
                col.a = static_cast<uint8_t>(255.f * (1.f - scoreAnim.t)); // Fade out alpha
                scoreAnim.particle.setFillColor(col);
                // The particle position is set in m_updateScoreAnims
                m_window.draw(scoreAnim.particle);
            }
        }


        // Draw the overlay elements
        m_window.draw(m_solvedOverlay);
        m_window.draw(winTxt);
        m_window.draw(m_contBtn);
        m_window.draw(*m_contTxt);
    }
}

// --- Celebration Effects ---
void Game::m_startCelebrationEffects() {
    m_confetti.clear();
    m_balloons.clear();
    m_celebrationEffectTimer = 0.f;

    sf::Vector2u windowSize = m_window.getSize();
    float w = static_cast<float>(windowSize.x);
    float h = static_cast<float>(windowSize.y);

    // --- Spawn Initial Confetti Burst ---
    int confettiCount = 200;
    for (int i = 0; i < confettiCount; ++i) {
        ConfettiParticle p;
        // ... (Confetti initialization logic - keep as is) ...
         // Start near bottom corners
        float startXConfetti = (randRange(0, 1) == 0) ? randRange(-20.f, 60.f) : randRange(w - 60.f, w + 20.f);
        float startYConfetti = randRange(h - 40.f, h + 20.f);
        p.shape.setPosition({ startXConfetti, startYConfetti });
        p.shape.setSize({ randRange(4.f, 8.f), randRange(6.f, 12.f) });
        p.shape.setFillColor(sf::Color(randRange(100, 255), randRange(100, 255), randRange(100, 255)));
        p.shape.setOrigin(p.shape.getSize() / 2.f);
        float angle = 0;
        if (startXConfetti < w / 2.f) { angle = randRange(270.f + 10.f, 270.f + 80.f); }
        else { angle = randRange(180.f + 10.f, 180.f + 80.f); }
        float speed = randRange(150.f, 450.f);
        float angleRad = angle * PI / 180.f;
        p.velocity = { std::cos(angleRad) * speed, std::sin(angleRad) * speed };
        p.angularVelocity = randRange(-360.f, 360.f);
        p.lifetime = randRange(2.0f, 5.0f);
        p.initialLifetime = p.lifetime;
        m_confetti.push_back(std::move(p));
    }

    // --- Spawn Initial Balloons ---
    int balloonCount = 7;
    // *** Define balloonRadius OUTSIDE the loop if it's constant ***
    const float balloonRadius = 30.f;
    // *************************************************************

    for (int i = 0; i < balloonCount; ++i) {
        Balloon b; // Declare 'b' INSIDE the loop scope

        // *** Now use balloonRadius ***
        float startX = randRange(balloonRadius * 2.f, w - balloonRadius * 2.f);
        b.position = { startX, h + balloonRadius + randRange(10.f, 100.f) }; // Start below screen
        b.initialX = startX; // Store the starting X
        // ****************************

        b.shape.setRadius(balloonRadius); // Use constant radius
        b.shape.setFillColor(sf::Color(randRange(100, 255), randRange(100, 255), randRange(100, 255), 230));
        b.shape.setOutlineColor(sf::Color::White);
        b.shape.setOutlineThickness(1.f);
        b.shape.setOrigin({ balloonRadius, balloonRadius }); // Use constant radius

        b.stringShape.setSize({ 2.f, randRange(40.f, 70.f) });
        b.stringShape.setFillColor(sf::Color(200, 200, 200));
        b.stringShape.setOrigin({ 1.f, 0 });

        // Set movement parameters using members of 'b'
        b.riseSpeed = randRange(-100.f, -50.f); // Use the adjusted range
        b.swaySpeed = randRange(0.8f, 1.8f);  // Use potentially adjusted range
        b.swayAmount = randRange(30.f, 60.f); // Use potentially adjusted range
        b.swayTimer = randRange(0.f, 2.f * PI);
        b.timeToDisappear = randRange(6.0f, 15.0f);

        m_balloons.push_back(std::move(b)); // Move the fully initialized 'b'
    }
}

void Game::m_updateCelebrationEffects(float dt) {
    sf::Vector2u windowSize = m_window.getSize();
    float h = static_cast<float>(windowSize.y);
    float w = static_cast<float>(windowSize.x);
    const float GRAVITY = 98.0f; // Pixels per second squared (adjust!)

    // --- Update Confetti ---
    // Use index-based loop for safe removal
    for (size_t i = 0; i < m_confetti.size(); /* no increment here */) {
        ConfettiParticle& p = m_confetti[i];

        // Apply gravity
        p.velocity.y += GRAVITY * dt;

        // Update position
        p.shape.move(p.velocity * dt);

        // Update rotation
        // *** Construct sf::Angle from degrees ***
        float rotationDegrees = p.angularVelocity * dt;
        p.shape.rotate(sf::degrees(rotationDegrees)); // Use sf::degrees() helper

        // Decrease lifetime
        p.lifetime -= dt;

        // Fade out based on lifetime
        float alphaRatio = std::max(0.f, p.lifetime / p.initialLifetime);
        sf::Color color = p.shape.getFillColor();
        color.a = static_cast<uint8_t>(255.f * alphaRatio);
        p.shape.setFillColor(color);

        // Check for removal
        if (p.lifetime <= 0.f || p.shape.getPosition().y > h + 50.f) { // Remove if dead or way off bottom
            // Efficiently remove by swapping with the last element and popping back
            std::swap(m_confetti[i], m_confetti.back());
            m_confetti.pop_back();
            // Do NOT increment 'i' here, as the swapped element needs checking next
        }
        else {
            ++i; // Only increment if no removal occurred
        }
    }

    // --- Update Balloons ---
    for (size_t i = 0; i < m_balloons.size(); /* no increment */) {
        Balloon& b = m_balloons[i];

        // Update vertical position
        b.position.y += b.riseSpeed * dt;

        // Update horizontal sway using sine wave
        b.swayTimer += dt;
        float currentSwayOffset = std::sin(b.swayTimer * b.swaySpeed) * b.swayAmount;
        // Apply sway relative to initial spawn X (or a slowly drifting center?)
        // Let's recalculate center based on current position for simplicity here
        float centerX = b.position.x; // Or could use a fixed X if needed
        b.shape.setPosition({ b.initialX + currentSwayOffset, b.position.y });

        // Position string below the balloon shape
        b.stringShape.setPosition(b.shape.getPosition() + sf::Vector2f(0, b.shape.getRadius()));

        // Decrease lifetime
        b.timeToDisappear -= dt;

        // Check for removal
        float topEdge = b.position.y - b.shape.getRadius();
        if (b.timeToDisappear <= 0.f || topEdge < -100.f) { // Remove if dead or way off top
            std::swap(m_balloons[i], m_balloons.back());
            m_balloons.pop_back();
        }
        else {
            ++i;
        }
    }

    // --- Optional: Spawn more effects over time ---
    m_celebrationEffectTimer += dt;
    if (m_celebrationEffectTimer > 0.15f) { // Spawn every 0.15 seconds
        m_celebrationEffectTimer = 0.f; // Reset timer

        // Spawn a few more confetti from corners
        for (int j = 0; j < 5; ++j) { /* Add logic similar to m_startCelebrationEffects to spawn 5 more */ }

        // Spawn one more balloon randomly?
        if (randRange(0, 10) < 2) { // 20% chance each spawn interval
            /* Add logic similar to m_startCelebrationEffects to spawn 1 more */
        }
    }
}

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
    if (m_scoreValueText) { // Reuse score text object maybe?
        std::string finalScoreStr = "Final Score: " + std::to_string(m_currentScore);
        m_scoreValueText->setString(finalScoreStr);
        m_scoreValueText->setCharacterSize(48); // Make it bigger
        m_scoreValueText->setFillColor(sf::Color::Yellow); // Highlight color
        // Center it on screen
        sf::FloatRect bounds = m_scoreValueText->getLocalBounds();
        m_scoreValueText->setOrigin({ bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f });
        m_scoreValueText->setPosition({ m_window.getSize().x / 2.f, m_window.getSize().y * 0.3f }); // Position near top-center
        m_scoreValueText->setScale({ 1.f, 1.f }); // Ensure no flourish scale applied
        m_window.draw(*m_scoreValueText);
        // Reset size/color after drawing if needed elsewhere
        m_scoreValueText->setCharacterSize(24);
        m_scoreValueText->setFillColor(m_currentTheme.scoreTextValue);
    }


    // --- Draw Celebration Effects --- (On top of score/background)
    m_renderCelebrationEffects(m_window);


    // --- Draw Continue Button ---
    // Reposition? Maybe center bottom?
    sf::Vector2f buttonPos = { m_window.getSize().x / 2.f, m_window.getSize().y * 0.8f };
    sf::Vector2f contBtnSize = m_contBtn.getSize();
    m_contBtn.setOrigin({ contBtnSize.x / 2.f, contBtnSize.y / 2.f }); // Center origin
    m_contBtn.setPosition(buttonPos);

    if (m_contTxt) { // Position text on button
        sf::FloatRect contTxtBounds = m_contTxt->getLocalBounds();
        m_contTxt->setOrigin({ contTxtBounds.position.x + contTxtBounds.size.x / 2.f, contTxtBounds.position.y + contTxtBounds.size.y / 2.f });
        m_contTxt->setPosition(m_contBtn.getPosition());
    }

    // Handle hover
    bool contHover = m_contBtn.getGlobalBounds().contains(mousePos);
    m_contBtn.setFillColor(contHover ? adjustColorBrightness(m_currentTheme.continueButton, 1.3f) : m_currentTheme.continueButton);

    m_window.draw(m_contBtn);
    if (m_contTxt) m_window.draw(*m_contTxt);

}

void Game::m_handleSessionCompleteEvents(const sf::Event& event) {
    // Optional: Handle Close, Resize if not done globally

    if (const auto* mb = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mb->button == sf::Mouse::Button::Left) {
            sf::Vector2f mp = m_window.mapPixelToCoords(mb->position);

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