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

inline float S(const Game* g, float du) { return du * g->m_uiScale; }

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
    m_dragging(false),
    m_decor(10),                             // Initialize DecorLayer
    m_selectedDifficulty(DifficultyLevel::None),
    m_puzzlesPerSession(0),
    m_currentPuzzleIndex(0),
    m_isInSession(false),
    m_uiScale(1.f),

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
    m_debugDrawCircleMode(false),
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

    //----------------------------------------------------------------
    //  create window at something close to the reference size but
    //  never bigger than the desktop.
    //----------------------------------------------------------------
    const sf::Vector2u desiredInitialSize{ 1000u, 800u };
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    unsigned int initialWidth = std::min(desiredInitialSize.x, desktop.size.x);
    unsigned int initialHeight = std::min(desiredInitialSize.y, desktop.size.y);

    m_window.create(sf::VideoMode({ initialWidth, initialHeight }),
        "Word Puzzle", sf::Style::Default);
    m_window.setFramerateLimit(60);
    m_window.setVerticalSyncEnabled(true);

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
    //m_updateView(m_window.getSize());
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
    else if (m_currentScreen == GameScreen::SessionComplete) 
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

// --- Other Private Methods (Placeholders) ---
void Game::m_rebuild() {
    // Select Random Theme
    if (!m_themes.empty()) {
        m_currentTheme = m_themes[0]; //forcing to test colors
        //m_currentTheme = m_themes[randRange<std::size_t>(0, m_themes.size() - 1)];
    }
    else {
        m_currentTheme = {};
        std::cerr << "Warning: No themes loaded, using default colors.\n";
    }

    // --- Base Word Selection (Revised Logic with Used Word Check) ---
    std::string selectedBaseWord = "";
    bool baseWordFound = false;
    std::vector<WordInfo> preCalculatedFilteredSolutions; // To store results if ideal word found

    if (m_roots.empty()) {
        std::cerr << "Error: No root words available for puzzle generation.\n";
        m_base = "ERROR"; // Set error state
    }
    else {
        // Get the specific base word criteria for this puzzle
        PuzzleCriteria baseCriteria = m_getCriteriaForCurrentPuzzle();
        // Get the sub-word filtering criteria for this difficulty
        std::vector<int> allowedSubRarities;
        int minSubLength = MIN_WORD_LENGTH;
        int maxSolutions = 0;
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

        int bestFallbackIndex = -1; // Index of the first word meeting base criteria AND is unused

        // --- Attempt 1: Find IDEAL match (meets base criteria, sub-word count, AND is unused) ---
        std::cout << "DEBUG: Searching for IDEAL base word..." << std::endl;
        for (std::size_t i = 0; i < m_roots.size(); ++i) {
            const auto& rootInfo = m_roots[i];
            const std::string& candidateWord = rootInfo.text; // Get candidate text

            // *** Check if already used this session ***
            if (m_usedBaseWordsThisSession.count(candidateWord)) {
                // std::cout << "  -> Candidate '" << candidateWord << "' skipped (already used)." << std::endl; // Optional verbose debug
                continue; // Skip if already used
            }
            // ****************************************

            // Check Base Length
            bool lengthMatch = false;
            for (int len : baseCriteria.allowedLengths) { if (rootInfo.text.length() == len) { lengthMatch = true; break; } }
            if (!lengthMatch) continue;

            // Check Base Rarity
            bool rarityMatch = false;
            for (int rarity : baseCriteria.allowedRarities) { if (rootInfo.rarity == rarity) { rarityMatch = true; break; } }
            if (!rarityMatch) continue;

            // --- Base Criteria Met & Not Used Yet ---
            std::cout << "  -> Candidate '" << candidateWord << "' meets base criteria and is unused." << std::endl;

            // Store as potential fallback if none better found yet
            if (bestFallbackIndex == -1) {
                bestFallbackIndex = static_cast<int>(i);
                std::cout << "     (Storing as initial fallback candidate)" << std::endl;
            }

            // --- Check Sub-word Count (only if length >= 5 or always?) ---
            if (rootInfo.text.length() >= 5) {
                std::cout << "     Checking sub-word count..." << std::endl;
                std::vector<WordInfo> temp_sub_solutions = Words::subWords(candidateWord, m_fullWordList);
                std::vector<WordInfo> filtered_sub_solutions;

                for (const auto& subInfo : temp_sub_solutions) {
                    if (subInfo.text.length() < minSubLength) continue;
                    bool subRarityMatch = false;
                    for (int subRarity : allowedSubRarities) { if (subInfo.rarity == subRarity) { subRarityMatch = true; break; } }
                    if (subRarityMatch) { filtered_sub_solutions.push_back(subInfo); }
                }
                std::cout << "     Generated " << filtered_sub_solutions.size() << " valid sub-words." << std::endl;

                if (filtered_sub_solutions.size() >= MIN_DESIRED_GRID_WORDS) {
                    std::cout << "     >>> IDEAL Candidate Found! Selecting '" << candidateWord << "'" << std::endl;
                    selectedBaseWord = candidateWord;
                    baseWordFound = true;
                    preCalculatedFilteredSolutions = std::move(filtered_sub_solutions);
                    m_usedBaseWordsThisSession.insert(selectedBaseWord); // Mark as used
                    break; // Found ideal word
                }
                else { std::cout << "     (Not enough sub-words, continuing search)" << std::endl; }
            }
            else { std::cout << "     (Base word length < 5, skipping sub-word count check for ideal)" << std::endl; }
        } // --- End Ideal Search Loop ---


        // --- Attempt 2: Use Best Fallback (if no ideal found but base match exists) ---
        if (!baseWordFound && bestFallbackIndex != -1) {
            // bestFallbackIndex points to a word that met base criteria AND was unused
            selectedBaseWord = m_roots[bestFallbackIndex].text;
            std::cout << "DEBUG: No IDEAL word found. Using best fallback candidate: '" << selectedBaseWord << "'" << std::endl;
            baseWordFound = true;
            preCalculatedFilteredSolutions.clear(); // Ensure sub-words are recalculated
            m_usedBaseWordsThisSession.insert(selectedBaseWord); // Mark as used
        }


        // --- Attempt 3: Broad Fallback (if still no word found) ---
        if (!baseWordFound) {
            std::cerr << "Warning: No base word found matching strict/fallback criteria. Applying BROAD fallback." << std::endl;
            PuzzleCriteria broadFallbackCriteria;
            broadFallbackCriteria.allowedLengths = { 4, 5, 6, 7 };
            broadFallbackCriteria.allowedRarities = { 1, 2, 3, 4 };

            for (std::size_t i = 0; i < m_roots.size(); ++i) { // Iterate again
                const auto& rootInfo = m_roots[i];
                const std::string& candidateWord = rootInfo.text;

                // *** Check if already used this session ***
                if (m_usedBaseWordsThisSession.count(candidateWord)) {
                    continue; // Skip if already used
                }
                // ****************************************

                // Check broad criteria
                bool lengthMatch = false;
                for (int len : broadFallbackCriteria.allowedLengths) { if (rootInfo.text.length() == len) { lengthMatch = true; break; } }
                if (!lengthMatch) continue;
                // Rarity always matches {1,2,3,4}

                selectedBaseWord = candidateWord;
                baseWordFound = true;
                preCalculatedFilteredSolutions.clear(); // Ensure recalculation
                m_usedBaseWordsThisSession.insert(selectedBaseWord); // Mark as used
                std::cout << "Found base word (Broad Fallback): " << selectedBaseWord << std::endl;
                break;
            }
        }


        // Final Safety Net
        if (!baseWordFound) {
            std::cerr << "CRITICAL FALLBACK: Cannot find unused root word. Using first available (may repeat)." << std::endl;
            if (!m_roots.empty()) {
                // Find the first word in the shuffled list, regardless of used status
                selectedBaseWord = m_roots[0].text;
                baseWordFound = true; // Mark as found to proceed
                preCalculatedFilteredSolutions.clear(); // Ensure recalculation
                // Do NOT insert into used set here, as we are allowing a repeat
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
            // Filter m_allPotentialSolutions based on sub-word criteria
            std::cout << "DEBUG: Filtering all potential solutions now." << std::endl;
            std::vector<int> allowedSubRarities; // Recalculate criteria needed
            int minSubLength = MIN_WORD_LENGTH;
            switch (m_selectedDifficulty) { /* Set allowedSubRarities/minSubLength */
            case DifficultyLevel::Easy:   allowedSubRarities = { 1, 2 }; minSubLength = MIN_WORD_LENGTH; break;
            case DifficultyLevel::Medium: allowedSubRarities = { 1, 2, 3 }; minSubLength = MIN_WORD_LENGTH; break;
            case DifficultyLevel::Hard:   allowedSubRarities = { 2, 3, 4 }; minSubLength = HARD_MIN_WORD_LENGTH; break;
            default:                      allowedSubRarities = { 1, 2, 3, 4 }; minSubLength = MIN_WORD_LENGTH; break;
            }

            for (const auto& subInfo : m_allPotentialSolutions) {
                if (subInfo.text.length() < minSubLength) continue;
                bool subRarityMatch = false;
                for (int subRarity : allowedSubRarities) { if (subInfo.rarity == subRarity) { subRarityMatch = true; break; } }
                if (subRarityMatch) { final_solutions.push_back(subInfo); }
            }
        }

        // Truncate the filtered list if necessary (get maxSolutions again)
        int maxSolutions = 0;
        switch (m_selectedDifficulty) { /* Set maxSolutions */
        case DifficultyLevel::Easy:   maxSolutions = EASY_MAX_SOLUTIONS; break;
        case DifficultyLevel::Medium: maxSolutions = MEDIUM_MAX_SOLUTIONS; break;
        case DifficultyLevel::Hard:   maxSolutions = HARD_MAX_SOLUTIONS; break;
        default:                      maxSolutions = 999; break;
        }

        if (final_solutions.size() > maxSolutions) {
            std::cout << "DEBUG: Truncating final solutions from " << final_solutions.size() << " to " << maxSolutions << std::endl;
            std::sort(final_solutions.begin(), final_solutions.end(),
                [](const WordInfo& a, const WordInfo& b) { /* Sorting lambda */
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

    // Assign the final list FOR THE GRID to member variables
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
            m_grid[i].assign(m_sorted[i].text.length(), '_');
        }
        else {
            m_grid[i].clear();
        }
    }


    // Reset Game State Variables *for the puzzle*
    m_found.clear();
    m_foundBonusWords.clear(); // Cleared here for each puzzle
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


    // IMPORTANT: Calculate layout based on the new data
    m_updateLayout(m_window.getSize());

    // Start Background Music
    m_backgroundMusic.stop();
    if (!m_musicFiles.empty()) {
        std::string musicPath = m_musicFiles[randRange<std::size_t>(0, m_musicFiles.size() - 1)];
        if (m_backgroundMusic.openFromFile(musicPath)) {
            m_backgroundMusic.setLooping(true);
            //m_backgroundMusic.play(); // Keep commented if music is annoying during debug
        }
        else {
            std::cerr << "Error loading music file: " << musicPath << std::endl;
        }
    }
} // End m_rebuild


// --- START OF FULL m_updateLayout FUNCTION (Simplified 5-row limit) ---

void Game::m_updateLayout(sf::Vector2u windowSize) {

    // 1. Calculate Global UI Scale ----------------------------------
    m_uiScale = std::min(windowSize.x / static_cast<float>(REF_W),
        windowSize.y / static_cast<float>(REF_H));
    m_uiScale = std::clamp(m_uiScale, 0.65f, 1.6f);

    // 2. Define Design Space References & Sections ------------------
    const float designW = static_cast<float>(REF_W);
    const float designH = static_cast<float>(REF_H);
    const sf::Vector2f designCenter = { designW / 2.f, designH / 2.f };
    const float designTopEdge = 0.f;
    const float designBottomEdge = designH;

    // --- Define Vertical Section Boundaries (DESIGN UNITS) ---
    const float topSectionHeight = designH * 0.15f;
    const float wheelSectionHeight = designH * 0.35f; // This is more of a desired minimum space
    const float gridSectionHeight = designH - topSectionHeight - wheelSectionHeight; // Available space for grid

    const float topSectionBottomY = designTopEdge + topSectionHeight;
    const float gridSectionTopY = topSectionBottomY;
    const float gridSectionBottomY = gridSectionTopY + gridSectionHeight;
    const float wheelSectionTopY = gridSectionBottomY; // Where wheel section *starts*
    const float wheelSectionBottomY = designBottomEdge; // Where wheel section *ends*

    // --- Start Logging ---
    std::cout << "--- Layout Update (" << windowSize.x << "x" << windowSize.y << ") ---" << std::endl;
    std::cout << "UI Scale (m_uiScale): " << m_uiScale << std::endl;
    std::cout << "Design Space: " << designW << "x" << designH << " (Center: " << designCenter.x << "," << designCenter.y << ")" << std::endl;


    // 3. Position Top Elements (Progress, Score) within Top Section ---------------
    const float scaledScoreBarWidth = S(this, SCORE_BAR_WIDTH); const float scaledScoreBarHeight = S(this, SCORE_BAR_HEIGHT); const float scaledScoreBarBottomMargin = S(this, SCORE_BAR_BOTTOM_MARGIN);
    const float scoreBarX_design = designCenter.x; const float scoreBarY_design = topSectionBottomY - scaledScoreBarBottomMargin - scaledScoreBarHeight / 2.f;
    m_scoreBar.setSize({ scaledScoreBarWidth, scaledScoreBarHeight }); m_scoreBar.setRadius(S(this, 10.f)); m_scoreBar.setOrigin({ scaledScoreBarWidth / 2.f, scaledScoreBarHeight / 2.f }); m_scoreBar.setPosition({ scoreBarX_design, scoreBarY_design }); m_scoreBar.setOutlineThickness(S(this, 1.f));
    const float scaledMeterHeight = S(this, PROGRESS_METER_HEIGHT); const float scaledMeterWidth = S(this, PROGRESS_METER_WIDTH); const float scaledMeterScoreGap = S(this, METER_SCORE_GAP);
    const float meterX_design = designCenter.x; const float meterY_design = scoreBarY_design - scaledScoreBarHeight / 2.f - scaledMeterScoreGap - scaledMeterHeight / 2.f;
    m_progressMeterBg.setSize({ scaledMeterWidth, scaledMeterHeight }); m_progressMeterBg.setOrigin({ scaledMeterWidth / 2.f, scaledMeterHeight / 2.f }); m_progressMeterBg.setPosition({ meterX_design, meterY_design }); /*m_progressMeterBg.setRadius(S(this, 5.f));*/ m_progressMeterBg.setOutlineThickness(S(this, PROGRESS_METER_OUTLINE));
    m_progressMeterFill.setOrigin({ 0.f, scaledMeterHeight / 2.f }); m_progressMeterFill.setPosition({ meterX_design - scaledMeterWidth / 2.f, meterY_design });
    if (m_progressMeterText) { const unsigned int bf = 16; unsigned int sf = (unsigned int)std::max(8.0f, S(this, (float)bf)); m_progressMeterText->setCharacterSize(sf); }
    const float scaledScoreTextOffset = S(this, 5.f);
    if (m_scoreValueText) { const unsigned int bf = 24; unsigned int sf = (unsigned int)std::max(10.0f, S(this, (float)bf)); m_scoreValueText->setCharacterSize(sf); sf::FloatRect vb = m_scoreValueText->getLocalBounds(); sf::Vector2f o = { 0.f,vb.position.y + vb.size.y / 2.f }; sf::Vector2f p = { scoreBarX_design + scaledScoreTextOffset, scoreBarY_design }; m_scoreValueText->setOrigin(o); m_scoreValueText->setPosition(p); }
    if (m_scoreLabelText) { const unsigned int bf = 24; unsigned int sf = (unsigned int)std::max(10.0f, S(this, (float)bf)); m_scoreLabelText->setCharacterSize(sf); sf::FloatRect lb = m_scoreLabelText->getLocalBounds(); sf::Vector2f o = { lb.position.x + lb.size.x, lb.position.y + lb.size.y / 2.f }; sf::Vector2f p = { scoreBarX_design - scaledScoreTextOffset, scoreBarY_design }; m_scoreLabelText->setOrigin(o); m_scoreLabelText->setPosition(p); }


    // 4. Calculate Grid Layout within Grid Section ---------------
    m_gridStartY = gridSectionTopY; // Keep this line
    const float availableGridHeight = gridSectionHeight; // Keep this line

    // --- Reset grid calculation variables ---
    int numCols = 1;
    int maxRowsPerCol = m_sorted.empty() ? 1 : (int)m_sorted.size();
    m_totalGridW = 0;
    float actualGridHeight = 0;
    m_wordCol.clear();
    m_wordRow.clear();
    m_colMaxLen.clear();
    m_colXOffset.clear();

    if (!m_sorted.empty()) {
        // --- Grid calculation heuristic ---
        const float st = S(this, TILE_SIZE);
        const float sp = S(this, TILE_PAD);
        const float sc = S(this, COL_PAD);
        const float stph = st + sp; // Scaled Tile + Pad Height
        const float stpw = st + sp; // Scaled Tile + Pad Width

        int bestFitCols = 1;
        int bestFitRows = (int)m_sorted.size();
        float minWidthVertFit = std::numeric_limits<float>::max();
        bool foundVerticalFit = false;

        int narrowestOverallCols = 1;
        int narrowestOverallRows = (int)m_sorted.size();
        float minWidthOverall = std::numeric_limits<float>::max();

        // Limit maximum columns slightly to prevent excessively wide layouts
        int maxPossibleCols = std::min(8, std::max(1, (int)m_sorted.size()));

        // --- Loop to find best layout ---
        for (int tryCols = 1; tryCols <= maxPossibleCols; ++tryCols) {
            int rowsNeeded = ((int)m_sorted.size() + tryCols - 1) / tryCols;
            if (rowsNeeded <= 0) rowsNeeded = 1;

            std::vector<int> currentTryColMaxLen(tryCols, 0);
            float currentTryWidth = 0;
            // Calculate max word length per column for this 'tryCols' layout
            for (size_t w = 0; w < m_sorted.size(); ++w) {
                int c = (int)w / rowsNeeded; // Determine column index for word w
                if (c >= 0 && c < tryCols) { // Check bounds
                    currentTryColMaxLen[c] = std::max<int>(currentTryColMaxLen[c], (int)m_sorted[w].text.length());
                }
            }
            // Calculate total width for this layout
            for (int len : currentTryColMaxLen) {
                currentTryWidth += (float)len * stpw - (len > 0 ? sp : 0.f); // Subtract trailing pad if len > 0
            }
            currentTryWidth += (float)std::max(0, tryCols - 1) * sc; // Add column padding
            if (currentTryWidth < 0) currentTryWidth = 0;

            // Calculate total height for this layout
            float currentTryHeight = (float)rowsNeeded * stph - (rowsNeeded > 0 ? sp : 0.f); // Subtract trailing pad
            if (currentTryHeight < 0) currentTryHeight = 0;


            // Track narrowest overall layout
            if (currentTryWidth < minWidthOverall) {
                minWidthOverall = currentTryWidth;
                narrowestOverallCols = tryCols;
                narrowestOverallRows = rowsNeeded;
            }

            // Track best layout that fits vertically
            if (currentTryHeight <= availableGridHeight) {
                if (!foundVerticalFit || currentTryWidth < minWidthVertFit) {
                    minWidthVertFit = currentTryWidth;
                    bestFitCols = tryCols;
                    bestFitRows = rowsNeeded;
                    foundVerticalFit = true;
                }
            }
        } // --- End loop to find best layout ---


        // --- Determine initial chosen layout ---
        int chosenCols = narrowestOverallCols;
        int chosenRows = narrowestOverallRows;
        if (foundVerticalFit) {
            chosenCols = bestFitCols;
            chosenRows = bestFitRows;
            std::cout << "  GRID INIT: Found Vertical Fit: Cols=" << chosenCols << ", RowsPerCol=" << chosenRows << std::endl;
        }
        else {
            std::cout << "  GRID INIT: Using Narrowest Overall: Cols=" << chosenCols << ", RowsPerCol=" << chosenRows << std::endl;
        }


        // --- ***** APPLY 5-ROW LIMIT (SIMPLIFIED) ***** ---
        const int MAX_ROWS_LIMIT = 5; // The row limit

        if (chosenRows > MAX_ROWS_LIMIT) { // Apply limit if initial calculation needs more than 5 rows
            std::cout << "  GRID OVERRIDE: Initial calculation needs " << chosenRows << " rows." << std::endl;
            std::cout << "                 Forcing Max Rows to " << MAX_ROWS_LIMIT << "." << std::endl;

            chosenRows = MAX_ROWS_LIMIT; // Apply the limit
            // Recalculate the number of columns needed for this fixed row count
            chosenCols = ((int)m_sorted.size() + chosenRows - 1) / chosenRows;
            if (chosenCols <= 0) chosenCols = 1; // Ensure at least one column

            std::cout << "                 Recalculated Columns Needed: " << chosenCols << std::endl;
        }
        // --- ***** END OF SIMPLIFIED ROW LIMIT LOGIC ***** ---


        // --- Finalize grid dimensions based on CHOSEN (potentially overridden) rows/cols ---
        numCols = chosenCols;
        maxRowsPerCol = chosenRows;

        // Calculate actual grid height based on final maxRowsPerCol
        actualGridHeight = (float)maxRowsPerCol * stph - (maxRowsPerCol > 0 ? sp : 0.f); // Subtract trailing pad
        if (actualGridHeight < 0) actualGridHeight = 0;

        // Log final grid dimensions
        std::cout << "  GRID FINAL: Using Layout: Cols=" << numCols << ", RowsPerCol=" << maxRowsPerCol << std::endl;
        std::cout << "  GRID H/W: Final Grid Height = " << actualGridHeight << std::endl;
        // Add warning if grid bottom still exceeds section (can happen if MAX_ROWS is still too many for the available height)
        if (m_gridStartY + actualGridHeight > gridSectionBottomY + 1.0f) { // Added tolerance
            std::cout << "  GRID WARNING: Grid bottom (Y=" << (m_gridStartY + actualGridHeight)
                << ") still exceeds section bottom (Y=" << gridSectionBottomY << ")!" << std::endl;
        }


        // --- Calculate grid word positions and column offsets using FINAL numCols/maxRowsPerCol ---
        m_wordCol.resize(m_sorted.size());
        m_wordRow.resize(m_sorted.size());
        m_colMaxLen.assign(numCols, 0); // Use final numCols

        // Assign words to columns/rows and find max length per column *based on the final layout*
        for (size_t w = 0; w < m_sorted.size(); ++w) {
            int c = (int)w / maxRowsPerCol; // Use final maxRowsPerCol
            int r = (int)w % maxRowsPerCol;
            if (c >= numCols) c = numCols - 1; // Safety clamp to final numCols
            m_wordCol[w] = c;
            m_wordRow[w] = r;
            if (c >= 0 && c < m_colMaxLen.size()) { // Check bounds using final numCols size
                m_colMaxLen[c] = std::max<int>(m_colMaxLen[c], (int)m_sorted[w].text.length());
            }
        }

        // Calculate column X offsets and total grid width based on m_colMaxLen (now using final layout)
        m_colXOffset.resize(numCols); // Use final numCols
        float currentX = 0;
        for (int c = 0; c < numCols; ++c) { // Use final numCols
            m_colXOffset[c] = currentX;
            float colWidth = 0.f;
            if (c >= 0 && c < m_colMaxLen.size()) { // Check bounds
                int len = m_colMaxLen[c];
                colWidth = (float)len * stpw - (len > 0 ? sp : 0.f); // Subtract trailing pad
            }
            if (colWidth < 0) colWidth = 0;
            currentX += colWidth + sc; // Add width and column padding
        }
        // Calculate final total width
        m_totalGridW = currentX - (numCols > 0 ? sc : 0.f); // Subtract trailing column pad
        if (m_totalGridW < 0) m_totalGridW = 0;

        // Calculate final starting X position for centering
        m_gridStartX = designCenter.x - m_totalGridW / 2.f;

        // Adjust column offsets by the starting X position
        for (int c = 0; c < numCols; ++c) {
            if (c < m_colXOffset.size()) { // Check bounds
                m_colXOffset[c] += m_gridStartX;
            }
        }

    }
    else {
        // Grid is empty, set defaults
        m_gridStartX = designCenter.x;
        m_totalGridW = 0;
        actualGridHeight = 0;
        std::cout << "  GRID LAYOUT: Grid empty." << std::endl;
    }
    // --- End Grid Calculation Section ---


    // 5. Determine Final Wheel Size & Position ---------------
    const float scaledGridWheelGap = S(this, GRID_WHEEL_GAP);
    const float scaledWheelBottomMargin = S(this, WHEEL_BOTTOM_MARGIN);
    const float scaledLetterRadius = S(this, LETTER_R); // Design letter radius
    const float scaledHudMinHeight = S(this, HUD_AREA_MIN_HEIGHT);
    float gridActualBottomY = m_gridStartY + actualGridHeight; // Uses the potentially adjusted grid height
    float gridAreaLimitY = gridActualBottomY + scaledGridWheelGap; // Y below which wheel path *should* start
    float wheelCenterBottomLimit = designBottomEdge - scaledWheelBottomMargin - scaledHudMinHeight; // Y above which wheel center must be to leave HUD space
    std::cout << "--- WHEEL LAYOUT: Start Calculation ---" << std::endl;
    std::cout << "  Grid Actual Bottom Y: " << gridActualBottomY << ", Scaled Gap: " << scaledGridWheelGap << ", Grid Area Limit Y: " << gridAreaLimitY << std::endl;
    std::cout << "  Wheel Center Bottom Limit: " << wheelCenterBottomLimit << std::endl;
    float defaultScaledWheelRadius = S(this, WHEEL_R);
    float availableWheelHeight = std::max(0.f, wheelCenterBottomLimit - gridAreaLimitY); // Space for wheel CENTER between limits
    std::cout << "  WHEEL VSPACE: Available Height (GridLimit to CenterBottomLimit) = " << availableWheelHeight << std::endl;

    // --- REVISED Wheel Size/Pos Logic v5 ---
    float finalScaledWheelRadius = defaultScaledWheelRadius;
    float finalWheelY = 0;

    float absoluteMinRadius = S(this, WHEEL_R * 0.4f); // Physical min radius based on default
    absoluteMinRadius = std::max(absoluteMinRadius, scaledLetterRadius * 1.2f); // Ensure letters fit
    std::cout << "  WHEEL MIN/MAX: Absolute Min Radius = " << absoluteMinRadius << ", Default Radius = " << defaultScaledWheelRadius << std::endl;

    // Determine the max radius that *could* fit if centered in the available space
    float maxRadiusPossible = availableWheelHeight / 2.0f;

    // Determine the radius to actually use
    if (maxRadiusPossible >= absoluteMinRadius) {
        // Enough space for at least the minimum physical wheel.
        // Use the largest radius possible up to the default size.
        finalScaledWheelRadius = std::min(maxRadiusPossible, defaultScaledWheelRadius);
        // Make sure we don't go below the absolute minimum either (can happen if availableHeight is small but > 2*absMin)
        finalScaledWheelRadius = std::max(finalScaledWheelRadius, absoluteMinRadius);
        std::cout << "  WHEEL LOGIC: Space available. MaxPossible=" << maxRadiusPossible << ", FinalRadius=" << finalScaledWheelRadius << std::endl;
    }
    else {
        // Not enough space even for the absolute minimum. Use the absolute minimum.
        finalScaledWheelRadius = absoluteMinRadius;
        std::cout << "  WHEEL LOGIC: VERY TIGHT SPACE. Using absolute min radius: " << finalScaledWheelRadius << std::endl;
    }

    // Calculate position based on the final chosen radius
    // Start by centering in the available space
    finalWheelY = gridAreaLimitY + availableWheelHeight / 2.f;

    // Now, CLAMP the Y position to ensure the FINAL radius fits within the bounds
    float minYPos = gridAreaLimitY + finalScaledWheelRadius; // Lowest center Y to avoid grid overlap
    float maxYPos = wheelCenterBottomLimit - finalScaledWheelRadius; // Highest center Y to allow HUD space

    std::cout << "  WHEEL Y CLAMP: Initial Center Y = " << finalWheelY << ", Min Allowed Y = " << minYPos << ", Max Allowed Y = " << maxYPos << std::endl;

    if (minYPos > maxYPos) {
        // Impossible to fit without *some* overlap. Prioritize avoiding grid overlap.
        std::cout << "  WHEEL Y CLAMP WARNING: Min Y > Max Y. Prioritizing grid gap." << std::endl;
        finalWheelY = minYPos; // Position wheel just below grid limit
    }
    else {
        // Possible to fit, clamp the centered position within the valid range.
        finalWheelY = std::clamp(finalWheelY, minYPos, maxYPos);
    }
    // --- End REVISED Wheel Size/Pos Logic v5 ---

    m_wheelX = designCenter.x; m_wheelY = finalWheelY; m_currentWheelRadius = finalScaledWheelRadius;
    std::cout << "  WHEEL FINAL: Assigned m_wheelX = " << m_wheelX << ", m_wheelY = " << m_wheelY << ", m_currentWheelRadius = " << m_currentWheelRadius << std::endl;
    std::cout << "--- WHEEL LAYOUT: End Calculation ---" << std::endl;


    // 6. Calculate Final Wheel Letter Positions (Original m_wheelCentres loop)-----
    // --> NOTE: m_wheelCentres are the LOGICAL centers for path detection
    m_wheelCentres.resize(m_base.size());
    if (!m_base.empty()) {
        float angleStep = (2.f * PI) / (float)m_base.size();
        for (size_t i = 0; i < m_base.size(); ++i) {
            float ang = (float)i * angleStep - PI / 2.f;
            m_wheelCentres[i] = { m_wheelX + m_currentWheelRadius * std::cos(ang),
                                  m_wheelY + m_currentWheelRadius * std::sin(ang) };
        }
    }

    // 6b. Calculate Final Wheel Letter RENDER Positions and Radius
    // --> NOTE: m_wheelLetterRenderPos are for DRAWING and HIT DETECTION
    m_wheelLetterRenderPos.resize(m_base.size());
    const float baseWheelRadius = S(this, WHEEL_R); // Scaled default radius
    const float scaledWheelPadding = S(this, 30.f); // Dynamic scaling
    const float visualBgRadius = m_currentWheelRadius + scaledWheelPadding;

    // Calculate scaling factor for letter circles/fonts based on final wheel size vs default
    float wheelRadiusRatio = 1.0f;
    if (baseWheelRadius > 1.0f && m_currentWheelRadius > 0.0f) {
        wheelRadiusRatio = m_currentWheelRadius / baseWheelRadius;
    }
    wheelRadiusRatio = std::clamp(wheelRadiusRatio, 0.7f, 1.0f); // Clamp scaling factor

    // Calculate and store the final radius used for RENDERING and HIT DETECTION
    m_currentLetterRenderRadius = S(this, LETTER_R) * wheelRadiusRatio;

    // Calculate the radius for LETTER POSITIONING on the wheel
    // This determines *where* the center of the letter circles are placed.
    const float letterPositionGap = S(this, 5.f); // Small gap from visual edge (scaled)
    // Calculate the radius where letters should be placed - slightly inside the visual BG edge
    float letterPositionRadius = visualBgRadius - m_currentLetterRenderRadius - letterPositionGap;
    // Ensure the letter position radius isn't too small (e.g., less than half the logical radius or less than the render radius itself)
    letterPositionRadius = std::max(letterPositionRadius, m_currentWheelRadius * 0.5f);
    letterPositionRadius = std::max(letterPositionRadius, m_currentLetterRenderRadius);

    std::cout << "  LAYOUT INFO: LetterRenderRadius=" << m_currentLetterRenderRadius
        << ", LetterPositionRadius=" << letterPositionRadius << std::endl;

    // Calculate and store the final render position for each letter using letterPositionRadius
    if (!m_base.empty()) {
        float angleStep = (2.f * PI) / (float)m_base.size();
        for (size_t i = 0; i < m_base.size(); ++i) {
            float ang = (float)i * angleStep - PI / 2.f;
            m_wheelLetterRenderPos[i] = {
                m_wheelX + letterPositionRadius * std::cos(ang),
                m_wheelY + letterPositionRadius * std::sin(ang)
            };
        }
    }
    // --- END OF WHEEL LETTER RENDER POSITION CALCULATION ---


    // 7. Other UI Element Positions ---------------
    // Buttons relative to wheel's visual edge (visualBgRadius)
    if (m_scrambleSpr && m_scrambleTex.getSize().y > 0) { float h = S(this, SCRAMBLE_BTN_HEIGHT); float s = h / m_scrambleTex.getSize().y; m_scrambleSpr->setScale({ s,s }); m_scrambleSpr->setOrigin({ 0.f,m_scrambleTex.getSize().y / 2.f }); m_scrambleSpr->setPosition({ m_wheelX + visualBgRadius + S(this,SCRAMBLE_BTN_OFFSET_X), m_wheelY + S(this,SCRAMBLE_BTN_OFFSET_Y) }); }
    if (m_hintSpr && m_hintTex.getSize().y > 0) { float h = S(this, HINT_BTN_HEIGHT); float s = h / m_hintTex.getSize().y; m_hintSpr->setScale({ s,s }); m_hintSpr->setOrigin({ (float)m_hintTex.getSize().x, m_hintTex.getSize().y / 2.f }); m_hintSpr->setPosition({ m_wheelX - visualBgRadius - S(this,HINT_BTN_OFFSET_X), m_wheelY + S(this,HINT_BTN_OFFSET_Y) }); }
    if (m_hintCountTxt) { const unsigned int bf = 20; unsigned int sf = (unsigned int)std::max(8.0f, S(this, (float)bf)); m_hintCountTxt->setCharacterSize(sf); }

    // Continue Button positioning (relative to wheel's visual edge)
    if (m_contTxt && m_contBtn.getPointCount() > 0) {
        sf::Vector2f s = { S(this,200.f),S(this,50.f) }; // Scaled button size
        m_contBtn.setSize(s);
        m_contBtn.setRadius(S(this, 10.f)); // Scaled radius
        m_contBtn.setOrigin({ s.x / 2.f, 0.f }); // Origin: Top-Center
        // Position below the visual bottom edge of the wheel background
        m_contBtn.setPosition({ m_wheelX, m_wheelY + visualBgRadius + S(this,CONTINUE_BTN_OFFSET_Y) }); // Use visualBgRadius

        // Position Continue Text centered on the button
        const unsigned int bf = 24;
        unsigned int sf = (unsigned int)std::max(10.0f, S(this, (float)bf));
        m_contTxt->setCharacterSize(sf);
        sf::FloatRect tb = m_contTxt->getLocalBounds();
        m_contTxt->setOrigin({ tb.position.x + tb.size.x / 2.f, tb.position.y + tb.size.y / 2.f }); // Center origin
        m_contTxt->setPosition(m_contBtn.getPosition() + sf::Vector2f{ 0.f, s.y / 2.f }); // Position relative to button's origin
    }

    // Guess Display positioning (done dynamically in render, but set properties here)
    if (m_guessDisplay_Text) { const unsigned int bf = 30; unsigned int sf = (unsigned int)std::max(10.0f, S(this, (float)bf)); m_guessDisplay_Text->setCharacterSize(sf); }
    if (m_guessDisplay_Bg.getPointCount() > 0) { m_guessDisplay_Bg.setRadius(S(this, 8.f)); m_guessDisplay_Bg.setOutlineThickness(S(this, 1.f)); }

    // --- Log Calculated HUD Start Position & Visual Wheel Top ---
    const float scaledHudOffsetY = S(this, HUD_TEXT_OFFSET_Y);
    // Calculate where HUD starts based on visual wheel bottom edge
    float calculatedHudStartY = m_wheelY + visualBgRadius + scaledHudOffsetY;
    float visualWheelTopEdgeY = m_wheelY - visualBgRadius; // Top of wheel BG
    std::cout << "  WHEEL/HUD INFO: Visual Wheel BG Top Edge Y = " << visualWheelTopEdgeY << std::endl;
    std::cout << "  WHEEL/HUD INFO: Calculated HUD Start Y = " << calculatedHudStartY << std::endl;
    if (visualWheelTopEdgeY < gridActualBottomY - 0.1f) { std::cout << "  WHEEL/HUD WARNING: Visual Wheel BG (Y=" << visualWheelTopEdgeY << ") overlaps Grid Bottom (Y=" << gridActualBottomY << ")!" << std::endl; }
    if (calculatedHudStartY > designBottomEdge + 0.1f) { std::cout << "  WHEEL/HUD WARNING: Calculated HUD Start Y (" << calculatedHudStartY << ") is below Design Bottom Edge (" << designBottomEdge << ")" << std::endl; }


    // 8. Menu Layouts --------
    sf::Vector2f windowCenterPix = sf::Vector2f(windowSize) / 2.f; sf::Vector2f mappedWindowCenter = m_window.mapPixelToCoords(sf::Vector2i(windowCenterPix));
    const float scaledMenuPadding = S(this, 40.f); const float scaledButtonSpacing = S(this, 20.f); const unsigned int scaledTitleSize = (unsigned int)std::max(12.0f, S(this, 36.f)); const unsigned int scaledButtonFontSize = (unsigned int)std::max(10.0f, S(this, 24.f)); const sf::Vector2f scaledButtonSize = { S(this,250.f),S(this,50.f) }; const float scaledButtonRadius = S(this, 10.f); const float scaledMenuRadius = S(this, 15.f);
    auto centerTextOnButton = [&](const std::unique_ptr<sf::Text>& textPtr, const RoundedRectangleShape& button) { if (!textPtr) return; sf::Text* text = textPtr.get(); sf::FloatRect tb = text->getLocalBounds(); text->setOrigin({ tb.position.x + tb.size.x / 2.f,tb.position.y + tb.size.y / 2.f }); text->setPosition(button.getPosition() + sf::Vector2f{ 0.f,button.getSize().y / 2.f }); };
    if (m_mainMenuTitle && m_casualButtonShape.getPointCount() > 0) { m_mainMenuTitle->setCharacterSize(scaledTitleSize); m_casualButtonText->setCharacterSize(scaledButtonFontSize); m_competitiveButtonText->setCharacterSize(scaledButtonFontSize); m_quitButtonText->setCharacterSize(scaledButtonFontSize); m_casualButtonShape.setSize(scaledButtonSize); m_competitiveButtonShape.setSize(scaledButtonSize); m_quitButtonShape.setSize(scaledButtonSize); m_casualButtonShape.setRadius(scaledButtonRadius); m_competitiveButtonShape.setRadius(scaledButtonRadius); m_quitButtonShape.setRadius(scaledButtonRadius); sf::FloatRect titleBounds = m_mainMenuTitle->getLocalBounds(); float sths = titleBounds.size.y + titleBounds.position.y + scaledButtonSpacing; float tbh = 3 * scaledButtonSize.y + 2 * scaledButtonSpacing; float smmh = scaledMenuPadding + sths + tbh + scaledMenuPadding; float smmw = std::max(scaledButtonSize.x, titleBounds.size.x + titleBounds.position.x) + 2 * scaledMenuPadding; m_mainMenuBg.setSize({ smmw,smmh }); m_mainMenuBg.setRadius(scaledMenuRadius); m_mainMenuBg.setOrigin({ smmw / 2.f,smmh / 2.f }); m_mainMenuBg.setPosition(mappedWindowCenter); sf::Vector2f mbp = m_mainMenuBg.getPosition(); float mty = mbp.y - smmh / 2.f; m_mainMenuTitle->setOrigin({ titleBounds.position.x + titleBounds.size.x / 2.f,titleBounds.position.y }); m_mainMenuTitle->setPosition({ mbp.x,mty + scaledMenuPadding }); float currentY = mty + scaledMenuPadding + sths; m_casualButtonShape.setOrigin({ scaledButtonSize.x / 2.f,0.f }); m_casualButtonShape.setPosition({ mbp.x,currentY }); centerTextOnButton(m_casualButtonText, m_casualButtonShape); currentY += scaledButtonSize.y + scaledButtonSpacing; m_competitiveButtonShape.setOrigin({ scaledButtonSize.x / 2.f,0.f }); m_competitiveButtonShape.setPosition({ mbp.x,currentY }); centerTextOnButton(m_competitiveButtonText, m_competitiveButtonShape); currentY += scaledButtonSize.y + scaledButtonSpacing; m_quitButtonShape.setOrigin({ scaledButtonSize.x / 2.f,0.f }); m_quitButtonShape.setPosition({ mbp.x,currentY }); centerTextOnButton(m_quitButtonText, m_quitButtonShape); }
    if (m_casualMenuTitle && m_easyButtonShape.getPointCount() > 0) { m_casualMenuTitle->setCharacterSize(scaledTitleSize); m_easyButtonText->setCharacterSize(scaledButtonFontSize); m_mediumButtonText->setCharacterSize(scaledButtonFontSize); m_hardButtonText->setCharacterSize(scaledButtonFontSize); m_returnButtonText->setCharacterSize(scaledButtonFontSize); m_easyButtonShape.setSize(scaledButtonSize); m_mediumButtonShape.setSize(scaledButtonSize); m_hardButtonShape.setSize(scaledButtonSize); m_returnButtonShape.setSize(scaledButtonSize); m_easyButtonShape.setRadius(scaledButtonRadius); m_mediumButtonShape.setRadius(scaledButtonRadius); m_hardButtonShape.setRadius(scaledButtonRadius); m_returnButtonShape.setRadius(scaledButtonRadius); sf::FloatRect ctb = m_casualMenuTitle->getLocalBounds(); float sths = ctb.size.y + ctb.position.y + scaledButtonSpacing; float tbh = 4 * scaledButtonSize.y + 3 * scaledButtonSpacing; float scmh = scaledMenuPadding + sths + tbh + scaledMenuPadding; float scmw = std::max(scaledButtonSize.x, ctb.size.x + ctb.position.x) + 2 * scaledMenuPadding; m_casualMenuBg.setSize({ scmw,scmh }); m_casualMenuBg.setRadius(scaledMenuRadius); m_casualMenuBg.setOrigin({ scmw / 2.f,scmh / 2.f }); m_casualMenuBg.setPosition(mappedWindowCenter); sf::Vector2f cmbp = m_casualMenuBg.getPosition(); float cmty = cmbp.y - scmh / 2.f; m_casualMenuTitle->setOrigin({ ctb.position.x + ctb.size.x / 2.f,ctb.position.y }); m_casualMenuTitle->setPosition({ cmbp.x,cmty + scaledMenuPadding }); float ccy = cmty + scaledMenuPadding + sths; m_easyButtonShape.setOrigin({ scaledButtonSize.x / 2.f,0.f }); m_easyButtonShape.setPosition({ cmbp.x,ccy }); centerTextOnButton(m_easyButtonText, m_easyButtonShape); ccy += scaledButtonSize.y + scaledButtonSpacing; m_mediumButtonShape.setOrigin({ scaledButtonSize.x / 2.f,0.f }); m_mediumButtonShape.setPosition({ cmbp.x,ccy }); centerTextOnButton(m_mediumButtonText, m_mediumButtonShape); ccy += scaledButtonSize.y + scaledButtonSpacing; m_hardButtonShape.setOrigin({ scaledButtonSize.x / 2.f,0.f }); m_hardButtonShape.setPosition({ cmbp.x,ccy }); centerTextOnButton(m_hardButtonText, m_hardButtonShape); ccy += scaledButtonSize.y + scaledButtonSpacing; m_returnButtonShape.setOrigin({ scaledButtonSize.x / 2.f,0.f }); m_returnButtonShape.setPosition({ cmbp.x,ccy }); centerTextOnButton(m_returnButtonText, m_returnButtonShape); }


    // --- Final Summary Log ---
    std::cout << "--- Overall Layout Summary (Design Coords) ---" << std::endl;
    std::cout << "Grid: StartY=" << m_gridStartY << " ActualH=" << actualGridHeight << " BottomY=" << gridActualBottomY << std::endl;
    std::cout << "Wheel: CenterY=" << m_wheelY << " Radius=" << m_currentWheelRadius << " VisualTopY=" << visualWheelTopEdgeY << std::endl;
    std::cout << "HUD: StartY=" << calculatedHudStartY << std::endl;
    std::cout << "---------------------------------------------" << std::endl;

} // End of m_updateLayout implementation

// --- END OF FULL m_updateLayout FUNCTION ---

sf::Vector2f Game::m_tilePos(int wordIdx, int charIdx) {
    sf::Vector2f result = { -1000.f, -1000.f }; // Default off-screen

    // --- Bounds checks (keep as is) ---
    if (m_sorted.empty() || wordIdx < 0 || wordIdx >= m_wordCol.size() || wordIdx >= m_wordRow.size() || charIdx < 0) {
        // Out of bounds or no data
        return result; // Return default off-screen position
    }

    int c = m_wordCol[wordIdx];
    int r = m_wordRow[wordIdx];

    if (c < 0 || c >= m_colXOffset.size()) {
        // Invalid column index
        return result; // Return default off-screen position
    }

    // --- Use SCALED tile dimensions for offset calculation ---
    const float scaledTileSize = S(this, TILE_SIZE);     // Get scaled size using S()
    const float scaledTilePad = S(this, TILE_PAD);       // Get scaled pad using S()
    const float scaledStepWidth = scaledTileSize + scaledTilePad; // Calculate scaled step

    // m_colXOffset[c] and m_gridStartY were calculated using scaled values in m_updateLayout
    float x = m_colXOffset[c] + static_cast<float>(charIdx) * scaledStepWidth; // Use scaled step
    float y = m_gridStartY + static_cast<float>(r) * scaledStepWidth;       // Use scaled step (assuming square tiles/padding)

    result = { x, y };

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
void Game::m_renderGameScreen(const sf::Vector2f& mousePos) { // mousePos is already mapped to coords

	
    //------------------------------------------------------------
    //  Calculate common scaled values ONCE
    //------------------------------------------------------------
    const float scaledTileSize = S(this, TILE_SIZE);
    const float scaledTilePad = S(this, TILE_PAD);
    const float scaledLetterRadius = S(this, LETTER_R); // For wheel letters
    //const float scaledWheelRadius = S(this, WHEEL_R); // Base wheel radius
    //const float scaledWheelPadding = S(this, 30.f); // Padding around wheel
    const float scaledWheelOutlineThickness = S(this, 3.f);
    const float scaledLetterCircleOutline = S(this, 2.f);
    const float scaledPathThickness = S(this, 5.0f); // For drag path
    const float scaledGuessDisplayGap = S(this, GUESS_DISPLAY_GAP);
    const float scaledGuessDisplayPadX = S(this, 15.f);
    const float scaledGuessDisplayPadY = S(this, 5.f);
    const float scaledGuessDisplayRadius = S(this, 8.f);
    const float scaledGuessDisplayOutline = S(this, 1.f);
    const float scaledHudOffsetY = S(this, HUD_TEXT_OFFSET_Y);
    const float scaledHudLineSpacing = S(this, HUD_LINE_SPACING);
    const float scaledHintOffsetX = S(this, 10.f); // Offset for hint text from icon center
    const float scaledHintOffsetY = S(this, 5.f);  // Offset for hint text below icon


    // Scaled Font Sizes (ensure minimum usable size)
    const unsigned int scaledGridLetterFontSize = static_cast<unsigned int>(std::max(8.0f, S(this, 20.f)));
    const unsigned int scaledFlyingLetterFontSize = static_cast<unsigned int>(std::max(8.0f, S(this, 20.f)));
    //const unsigned int scaledWheelLetterFontSize = static_cast<unsigned int>(std::max(8.0f, S(this, 25.f)));
    const unsigned int scaledGuessDisplayFontSize = static_cast<unsigned int>(std::max(8.0f, S(this, 30.f)));
    const unsigned int scaledFoundFontSize = static_cast<unsigned int>(std::max(8.0f, S(this, 20.f)));
    const unsigned int scaledBonusFontSize = static_cast<unsigned int>(std::max(8.0f, S(this, 18.f)));
    const unsigned int scaledHintFontSize = static_cast<unsigned int>(std::max(8.0f, S(this, 20.f)));
    const unsigned int scaledSolvedFontSize = static_cast<unsigned int>(std::max(8.0f, S(this, 26.f)));
    const unsigned int scaledContinueFontSize = static_cast<unsigned int>(std::max(8.0f, S(this, 24.f))); // Assuming contTxt base size is 24


    //------------------------------------------------------------
    //  Draw Progress Meter (If in session) - Assuming layout set size/pos correctly
    //------------------------------------------------------------
    if (m_isInSession) {
        m_progressMeterBg.setFillColor(sf::Color(50, 50, 50, 150)); // Or theme
        m_progressMeterBg.setOutlineColor(sf::Color(150, 150, 150)); // Or theme
        // Use scaled thickness - Requires PROGRESS_METER_OUTLINE constant
        m_progressMeterBg.setOutlineThickness(S(this, PROGRESS_METER_OUTLINE));

        m_progressMeterFill.setFillColor(sf::Color(0, 180, 0, 200)); // Or theme

        float progressRatio = 0.f;
        if (m_puzzlesPerSession > 0) {
            progressRatio = static_cast<float>(m_currentPuzzleIndex + 1) / static_cast<float>(m_puzzlesPerSession);
        }
        // Use scaled width from layout - Assuming m_progressMeterBg.getSize().x was set correctly in layout
        float fillWidth = m_progressMeterBg.getSize().x * progressRatio;
        m_progressMeterFill.setSize({ fillWidth, m_progressMeterBg.getSize().y }); // Use height from layout's bg size

        m_window.draw(m_progressMeterBg);
        m_window.draw(m_progressMeterFill);

        if (m_progressMeterText) {
            const unsigned int scaledProgressFontSize = static_cast<unsigned int>(std::max(8.0f, S(this, 16.f))); // Base size 16?
            m_progressMeterText->setCharacterSize(scaledProgressFontSize); // Set scaled size
            std::string progressStr = std::to_string(m_currentPuzzleIndex + 1) + "/" + std::to_string(m_puzzlesPerSession);
            m_progressMeterText->setString(progressStr);
            m_progressMeterText->setFillColor(sf::Color::White); // Or theme color

            sf::FloatRect textBounds = m_progressMeterText->getLocalBounds();
            m_progressMeterText->setOrigin({ textBounds.position.x + textBounds.size.x / 2.f,
                                            textBounds.position.y + textBounds.size.y / 2.f });
            m_progressMeterText->setPosition(m_progressMeterBg.getPosition()); // Position at Bg center (already scaled)
            m_window.draw(*m_progressMeterText);
        }
    }

    //------------------------------------------------------------
    //  Draw Score Bar - Apply theme colors and draw
    //------------------------------------------------------------
    // Apply theme colors to the score bar shape
    m_scoreBar.setFillColor(m_currentTheme.scoreBarBg);
    m_scoreBar.setOutlineColor(m_currentTheme.wheelOutline);
    // Optional: Scale outline thickness if desired, or use a fixed value
    // m_scoreBar.setOutlineThickness(S(this, 1.f)); // Example scaled outline
    m_scoreBar.setOutlineThickness(1.f); // Example fixed outline

    // Draw the score bar shape (position/size/radius set in layout)
    m_window.draw(m_scoreBar);

    // Apply theme colors to score text elements
    if (m_scoreLabelText) {
        m_scoreLabelText->setFillColor(m_currentTheme.scoreTextLabel);
        // Character size & position/origin are set in layout
        m_window.draw(*m_scoreLabelText);
    }
    if (m_scoreValueText) {
        // Apply visual flourish scaling if timer is active
        if (m_scoreFlourishTimer > 0.f) {
            float scaleFactor = 1.0f + 0.4f * std::sin((SCORE_FLOURISH_DURATION - m_scoreFlourishTimer) / SCORE_FLOURISH_DURATION * PI); // Simple pulse
            m_scoreValueText->setScale({ scaleFactor, scaleFactor });
            // Optionally adjust origin slightly to keep centered during scale
            sf::FloatRect bounds = m_scoreValueText->getLocalBounds();
            // Assuming origin was left-center - adjust if different
            m_scoreValueText->setOrigin({ 0.f + bounds.position.x, bounds.position.y + bounds.size.y / 2.f });
        }
        else {
            m_scoreValueText->setScale({ 1.f, 1.f }); // Reset scale
            // Reset origin if it was changed
            sf::FloatRect bounds = m_scoreValueText->getLocalBounds();
            m_scoreValueText->setOrigin({ 0.f + bounds.position.x, bounds.position.y + bounds.size.y / 2.f });
        }

        m_scoreValueText->setFillColor(m_currentTheme.scoreTextValue);
        // Character size & position are set in layout
        m_window.draw(*m_scoreValueText);

        // Reset scale immediately after drawing if it was changed by flourish
        // This prevents the scale affecting subsequent bounds calculations if the object is reused.
        // m_scoreValueText->setScale({1.f, 1.f}); // Redundant if done in the else block
    }

    //------------------------------------------------------------
    //  Draw letter grid (Gems visible before solving)
    //------------------------------------------------------------
    if (!m_sorted.empty()) {
        // Use scaled constants calculated at the top
        const float scaledTileRadius = scaledTileSize * 0.18f; // Relative scaling is fine here
        const float scaledTileOutline = S(this, 1.f);          // Or use a constant

        RoundedRectangleShape tileBackground({ scaledTileSize, scaledTileSize }, scaledTileRadius, 10);
        tileBackground.setOutlineThickness(scaledTileOutline);

        sf::Text letterText(m_font, "", scaledGridLetterFontSize); // Use pre-calculated scaled size
        letterText.setFillColor(m_currentTheme.gridLetter);

        for (std::size_t w = 0; w < m_sorted.size(); ++w) {
            // ... bounds checks ...
            int wordRarity = m_sorted[w].rarity;
            for (std::size_t c = 0; c < m_sorted[w].text.length(); ++c) {
                // ... bounds checks ...
                sf::Vector2f p = m_tilePos(static_cast<int>(w), static_cast<int>(c)); // Uses fixed, scaled m_tilePos
                bool isFilled = (m_grid[w][c] != '_');

                tileBackground.setPosition(p); // Use scaled pos
                // ... set colors ...
                m_window.draw(tileBackground); // Draws with scaled size/radius/thickness

                if (!isFilled) { // Draw Gem instead of letter
                    // Determine which gem sprite to use based on word rarity
                    sf::Sprite* gemSprite = nullptr; // Pointer to the sprite to draw
                    switch (wordRarity) {
                    case 1: // Common - none
                        break;
					case 2: // Uncommon - Emerald(Sapphire)
                        if (m_sapphireSpr) gemSprite = m_sapphireSpr.get();
                        break;
                    case 3: // Rare - Ruby
                        if (m_rubySpr) gemSprite = m_rubySpr.get();
                        break;
                    case 4: // Very Rare - Also Diamond or another distinct gem if you add one
                        if (m_diamondSpr) gemSprite = m_diamondSpr.get();
                        break;
                    }

                    // Set tile background color (e.g., empty color)
                    //tileBackground.setFillColor(m_currentTheme.gridEmptyTile);
                    //tileBackground.setOutlineColor(m_currentTheme.gridEmptyTile);
                    m_window.draw(tileBackground); // Draw the empty tile background first

                    // If a valid gem sprite was selected, position and draw it
                    if (gemSprite != nullptr) {
                        // Calculate center of the tile
                        float tileCenterX = p.x + scaledTileSize / 2.f;
                        float tileCenterY = p.y + scaledTileSize / 2.f;
                        // Set position (Origin is already center from constructor/load)
                        gemSprite->setPosition({ tileCenterX, tileCenterY });
                        // Ensure scale was set correctly in constructor/load
                        m_window.draw(*gemSprite); // Draw the gem sprite
                    }
                }
                else { // Draw Letter
                    bool isAnimatingToTile = false; /* ... check anims ... */
                    if (!isAnimatingToTile) {
                        tileBackground.setPosition(p); // Draw filled bg
                        // ... set colors ...
                        m_window.draw(tileBackground);

                        letterText.setString(std::string(1, m_grid[w][c]));
                        sf::FloatRect b = letterText.getLocalBounds(); // Depends on scaled font size
                        letterText.setOrigin({ b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f });
                        // Position using scaled pos 'p' and scaledTileSize
                        letterText.setPosition(p + sf::Vector2f{ scaledTileSize / 2.f, scaledTileSize / 2.f });
                        m_window.draw(letterText);
                    }
                }
            }
        }
    } // End grid drawing


//------------------------------------------------------------
//  Draw wheel background & letters
//------------------------------------------------------------
// Use the radius calculated and stored during layout
    const float currentWheelRadius = m_currentWheelRadius; // Get from member variable
    const float scaledWheelPadding = S(this, 30.f); // Scale padding on the fly
    

    m_wheelBg.setRadius(currentWheelRadius + scaledWheelPadding); // USE STORED RADIUS + scaled padding
    m_wheelBg.setFillColor(m_currentTheme.wheelBg);
    m_wheelBg.setOutlineColor(m_currentTheme.wheelOutline);
    m_wheelBg.setOutlineThickness(scaledWheelOutlineThickness);
    m_wheelBg.setOrigin({ m_wheelBg.getRadius(), m_wheelBg.getRadius() });
    m_wheelBg.setPosition({ m_wheelX, m_wheelY }); // Use X, Y from layout
    m_window.draw(m_wheelBg);

    // --- DRAW FLYING LETTER ANIMATIONS ---
    sf::Text flyingLetterText(m_font, "", scaledFlyingLetterFontSize); // USE SCALED SIZE
    // Set a base color - maybe the grid letter color or a specific "flying" color
    sf::Color flyColorBase = m_currentTheme.gridLetter; // Or m_currentTheme.flyingLetter if you add it

    for (const auto& a : m_anims) {
        // Determine color based on target (e.g., grid vs score)
        sf::Color currentFlyColor = flyColorBase;
        if (a.target == AnimTarget::Score) {
            // Make bonus letters visually distinct, e.g., yellow
            currentFlyColor = sf::Color::Yellow;
        }

        // Optional: Add fading based on animation progress (t)
        // Example: Fade out towards the end
        float alpha_ratio = 1.0f;
        if (a.t > 0.7f) { // Start fading in the last 30%
            alpha_ratio = (1.0f - a.t) / 0.3f;
            alpha_ratio = std::max(0.0f, std::min(1.0f, alpha_ratio)); // Clamp 0-1
        }
        currentFlyColor.a = static_cast<std::uint8_t>(255.f * alpha_ratio);


        // Apply the final color
        flyingLetterText.setFillColor(currentFlyColor);

        // Calculate interpolated position using easing
        // Using Smoothstep easing: 3t^2 - 2t^3
        float eased_t = a.t * a.t * (3.f - 2.f * a.t);
        sf::Vector2f p = a.start + (a.end - a.start) * eased_t; // Calculate position

        // Set character, origin, and position for drawing
        flyingLetterText.setString(std::string(1, a.ch));
        sf::FloatRect bounds = flyingLetterText.getLocalBounds(); // Get bounds AFTER setting string
        flyingLetterText.setOrigin({ bounds.position.x + bounds.size.x / 2.f,
                                     bounds.position.y + bounds.size.y / 2.f }); // Center origin
        flyingLetterText.setPosition(p); // Set final position

        // Draw the animated letter
        m_window.draw(flyingLetterText);
    }
    // --- END FLYING LETTER ANIMATIONS ---


    //------------------------------------------------------------
    //  Draw Path lines (BEFORE Wheel Letters)
    //------------------------------------------------------------
    if (m_dragging && !m_path.empty()) {
        const float halfThickness = scaledPathThickness / 2.0f; // USE SCALED value
        const sf::PrimitiveType stripType = sf::PrimitiveType::TriangleStrip;
        const sf::Color pathColor = m_currentTheme.dragLine; // Get theme color

        // --- Draw path between selected letters ---
        if (m_path.size() >= 2) {
            // Create a single vertex array sized for the whole path
            // Each segment adds 4 vertices (to form a quad with TriangleStrip)
            sf::VertexArray finalPathStrip(stripType, (m_path.size() - 1) * 4);
            size_t currentVertex = 0; // Index for assigning vertices

            for (size_t i = 0; i < m_path.size() - 1; ++i) {
                int idx1 = m_path[i];
                int idx2 = m_path[i + 1];

                // Ensure indices are valid
                if (idx1 < 0 || idx1 >= m_wheelCentres.size() || idx2 < 0 || idx2 >= m_wheelCentres.size()) {
                    continue; // Skip invalid segment
                }

                sf::Vector2f p1 = m_wheelCentres[idx1];
                sf::Vector2f p2 = m_wheelCentres[idx2];

                sf::Vector2f direction = p2 - p1;
                float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
                if (length < 0.1f) continue; // Avoid division by zero for very short segments

                sf::Vector2f unitDirection = direction / length;
                sf::Vector2f unitPerpendicular = { -unitDirection.y, unitDirection.x }; // Rotate 90 degrees
                sf::Vector2f offset = unitPerpendicular * halfThickness;

                // Assign vertices for the segment strip part using direct indexing
                if (currentVertex + 3 < finalPathStrip.getVertexCount()) { // Bounds check before assigning
                    finalPathStrip[currentVertex].position = p1 - offset;
                    finalPathStrip[currentVertex].color = pathColor;
                    currentVertex++;

                    finalPathStrip[currentVertex].position = p1 + offset;
                    finalPathStrip[currentVertex].color = pathColor;
                    currentVertex++;

                    finalPathStrip[currentVertex].position = p2 - offset;
                    finalPathStrip[currentVertex].color = pathColor;
                    currentVertex++;

                    finalPathStrip[currentVertex].position = p2 + offset;
                    finalPathStrip[currentVertex].color = pathColor;
                    currentVertex++;
                }
                else {
                    // Should not happen if pre-sizing is correct, but good for safety
                    std::cerr << "VertexArray index out of bounds!" << std::endl;
                    break;
                }
            } // End for loop through path segments

            // Trim unused vertices if segments were skipped
            finalPathStrip.resize(currentVertex);

            if (finalPathStrip.getVertexCount() > 0) {
                m_window.draw(finalPathStrip); // Draw the completed path strip
            }

        } // End if m_path.size() >= 2

        // --- Draw rubber band segment from last letter to mouse ---
        if (!m_path.empty()) {
            int lastIdx = m_path.back();
            if (lastIdx >= 0 && lastIdx < m_wheelCentres.size()) { // Check index validity
                sf::Vector2f p1 = m_wheelCentres[lastIdx]; // Start from last selected letter center
                sf::Vector2f p2 = mousePos;                // End at current mouse position

                sf::Vector2f direction = p2 - p1;
                float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);

                if (length > 0.1f) { // Only draw if mouse has moved away
                    sf::Vector2f unitDirection = direction / length;
                    sf::Vector2f unitPerpendicular = { -unitDirection.y, unitDirection.x };
                    sf::Vector2f offset = unitPerpendicular * halfThickness;

                    sf::VertexArray rubberBandStrip(stripType, 4); // 4 vertices for one quad
                    rubberBandStrip[0].position = p1 - offset;
                    rubberBandStrip[1].position = p1 + offset;
                    rubberBandStrip[2].position = p2 - offset;
                    rubberBandStrip[3].position = p2 + offset;

                    // Set color for all vertices
                    rubberBandStrip[0].color = pathColor;
                    rubberBandStrip[1].color = pathColor;
                    rubberBandStrip[2].color = pathColor;
                    rubberBandStrip[3].color = pathColor;

                    m_window.draw(rubberBandStrip);
                }
            }
        } // End rubber band drawing

    } // End if m_dragging

    // --- START: Draw Guess Display (Above Wheel) ---
    if (m_gameState == GState::Playing && !m_currentGuess.empty() && m_guessDisplay_Text && m_guessDisplay_Bg.getPointCount() > 0) {
        m_guessDisplay_Text->setCharacterSize(scaledGuessDisplayFontSize); // USE SCALED SIZE
        m_guessDisplay_Text->setString(m_currentGuess);

        sf::FloatRect textBounds = m_guessDisplay_Text->getLocalBounds(); // Depends on scaled size
        // Use scaled padding for background size
        sf::Vector2f bgSize = { textBounds.position.x + textBounds.size.x + 2 * scaledGuessDisplayPadX,
                                textBounds.position.y + textBounds.size.y + 2 * scaledGuessDisplayPadY };

        // Position above the wheel using scaled values
        float guessY = m_wheelY - (currentWheelRadius + scaledWheelPadding) - (bgSize.y / 2.f) - scaledGuessDisplayGap;

        m_guessDisplay_Bg.setSize(bgSize); // Use scaled size
        m_guessDisplay_Bg.setRadius(scaledGuessDisplayRadius); // Use scaled radius
        m_guessDisplay_Bg.setFillColor(m_currentTheme.gridFilledTile);
        m_guessDisplay_Bg.setOutlineColor(sf::Color(150, 150, 150, 200));
        m_guessDisplay_Bg.setOutlineThickness(scaledGuessDisplayOutline); // Use scaled thickness
        m_guessDisplay_Bg.setOrigin({ bgSize.x / 2.f, bgSize.y / 2.f });
        m_guessDisplay_Bg.setPosition({ m_wheelX, guessY }); // Use scaled X, Y

        m_guessDisplay_Text->setOrigin({ textBounds.position.x + textBounds.size.x / 2.f,
                                        textBounds.position.y + textBounds.size.y / 2.f });
        m_guessDisplay_Text->setPosition({ m_wheelX, guessY }); // Use scaled X, Y
        m_guessDisplay_Text->setFillColor(m_currentTheme.gridLetter);

        m_window.draw(m_guessDisplay_Bg);
        m_window.draw(*m_guessDisplay_Text);
    }  // --- END: Draw Guess Display ---


// --- Draw Wheel Letters --- (Using pre-calculated render pos/radius)
    // Font size still needs scaling based on ratio calculated in layout
    const float baseWheelRadius = S(this, WHEEL_R);
    float wheelRadiusRatio = 1.0f;
    if (baseWheelRadius > 1.0f && m_currentWheelRadius > 0.0f) {
        wheelRadiusRatio = m_currentWheelRadius / baseWheelRadius;
    }
    wheelRadiusRatio = std::clamp(wheelRadiusRatio, 0.7f, 1.0f);
    const unsigned int scaledWheelLetterFontSize = static_cast<unsigned int>(std::max(8.0f, S(this, 25.f) * wheelRadiusRatio));

    // Use the radius calculated and stored during layout
    const float letterCircleRadius = m_currentLetterRenderRadius;

    // Log once per render frame (removed tempCount logic)
    // std::cout << "  RENDER INFO: Using LetterRadius=" << letterCircleRadius << ", FontSize=" << scaledWheelLetterFontSize << std::endl;


    for (std::size_t i = 0; i < m_base.size(); ++i) {
        // Use render position vector size for safety check
        if (i >= m_wheelLetterRenderPos.size()) continue;

        bool isHilited = std::find(m_path.begin(), m_path.end(), static_cast<int>(i)) != m_path.end();

        // Get the pre-calculated render position
        sf::Vector2f renderPos = m_wheelLetterRenderPos[i];

        // --- Draw Letter Circle ---
        sf::CircleShape letterCircle(letterCircleRadius); // USE STORED RENDER RADIUS
        letterCircle.setOrigin({ letterCircleRadius, letterCircleRadius }); // Use radius for origin
        letterCircle.setPosition(renderPos); // USE STORED RENDER POSITION
        letterCircle.setFillColor(isHilited ? m_currentTheme.wheelOutline : m_currentTheme.letterCircleNormal); // Adjust theme colors if needed
        letterCircle.setOutlineColor(m_currentTheme.wheelOutline);
        letterCircle.setOutlineThickness(scaledLetterCircleOutline);
        m_window.draw(letterCircle);

        // --- Draw Letter Text ---
        sf::Text chTxt(m_font, std::string(1, static_cast<char>(std::toupper(m_base[i]))), scaledWheelLetterFontSize); // USE SCALED FONT SIZE
        chTxt.setFillColor(isHilited ? m_currentTheme.letterTextHighlight : m_currentTheme.letterTextNormal);
        sf::FloatRect txtBounds = chTxt.getLocalBounds();
        chTxt.setOrigin({ txtBounds.position.x + txtBounds.size.x / 2.f, txtBounds.position.y + txtBounds.size.y / 2.f });
        chTxt.setPosition(renderPos); // USE STORED RENDER POSITION
        m_window.draw(chTxt);
    }
    // --- End Wheel Letters ---


    //------------------------------------------------------------
    //  Draw UI Buttons / Hover - Assuming Sprites are positioned/scaled correctly in layout
    //------------------------------------------------------------
    if (m_gameState == GState::Playing) {
        if (m_scrambleSpr) { // Check pointer
            bool scrambleHover = m_scrambleSpr->getGlobalBounds().contains(mousePos);
            m_scrambleSpr->setColor(scrambleHover ? sf::Color::White : sf::Color(255, 255, 255, 200));
            m_window.draw(*m_scrambleSpr); // Draws using scale/position from layout
        }
        if (m_hintSpr) { // Check pointer
            bool hintHover = m_hintSpr->getGlobalBounds().contains(mousePos);
            sf::Color hintColor = (m_hintsAvailable > 0) ? (hintHover ? sf::Color::White : sf::Color(255, 255, 255, 200)) : sf::Color(128, 128, 128, 150);
            m_hintSpr->setColor(hintColor);
            m_window.draw(*m_hintSpr); // Draws using scale/position from layout
        }
    }

    //------------------------------------------------------------
    //  Draw HUD
    //------------------------------------------------------------
    // Start below scaled wheel radius + padding + offset
    float bottomHudStartY = m_wheelY + (m_currentWheelRadius + scaledWheelPadding) + scaledHudOffsetY;
    float currentTopY = bottomHudStartY; // Runner variable for the TOP Y of the current line

    // Found Text
    std::string foundCountStr = "Found: " + std::to_string(m_found.size()) + "/" + std::to_string(m_solutions.size());
    sf::Text foundTxt(m_font, foundCountStr, scaledFoundFontSize); // USE SCALED SIZE
    foundTxt.setFillColor(m_currentTheme.hudTextFound);
    sf::FloatRect foundBounds = foundTxt.getLocalBounds();
    // Use Mid-Top origin for easier vertical stacking
    foundTxt.setOrigin({ foundBounds.position.x + foundBounds.size.x / 2.f,
                         foundBounds.position.y }); // Origin Mid-Top is important here
    foundTxt.setPosition({ m_wheelX, currentTopY }); // Position the origin
    m_window.draw(foundTxt);

    // Calculate the Y position for the *top* of the next line
    // Add the actual height of the 'Found' text bounds + the desired line spacing
    currentTopY += foundBounds.size.y + scaledHudLineSpacing; // Use full height and full spacing

    // Bonus Word Counter
    if (!m_allPotentialSolutions.empty() || !m_foundBonusWords.empty()) {
        // *** Calculate total possible bonus words ***
        int totalPossibleBonus = 0;
        for (const auto& potentialWordInfo : m_allPotentialSolutions) {
            bool isOnGrid = false;
            for (const auto& gridWordInfo : m_solutions) {
                if (potentialWordInfo.text == gridWordInfo.text) {
                    isOnGrid = true;
                    break;
                }
            }
            if (!isOnGrid) {
                totalPossibleBonus++;
            }
        }
        // **********************************************

        std::string bonusCountStr = "Bonus Words: " + std::to_string(m_foundBonusWords.size()) + "/" + std::to_string(totalPossibleBonus);
        sf::Text bonusFoundTxt(m_font, bonusCountStr, scaledBonusFontSize); // USE SCALED SIZE
        bonusFoundTxt.setFillColor(sf::Color::Yellow); // Or theme color
        sf::FloatRect bonusBounds = bonusFoundTxt.getLocalBounds();
        // Use Mid-Top origin
        bonusFoundTxt.setOrigin({ bonusBounds.position.x + bonusBounds.size.x / 2.f,
                                  bonusBounds.position.y }); // Origin Mid-Top
        bonusFoundTxt.setPosition({ m_wheelX, currentTopY }); // Position the origin at the calculated Y
        m_window.draw(bonusFoundTxt);

        // Update Y for any potential subsequent lines (if needed)
        currentTopY += bonusBounds.size.y + scaledHudLineSpacing;
    }

    // Hint Count Text - Position relative to Hint Icon (Existing logic should be okay)
    if (m_hintCountTxt && m_hintSpr) { // Check pointers
        m_hintCountTxt->setCharacterSize(scaledHintFontSize); // SET SCALED SIZE
        m_hintCountTxt->setString("Hints: " + std::to_string(m_hintsAvailable));
        m_hintCountTxt->setFillColor(m_currentTheme.hudTextFound);
        sf::FloatRect hintTxtBounds = m_hintCountTxt->getLocalBounds();
        // Center horizontally relative to icon, use bottom Y of icon + offset for vertical
        m_hintCountTxt->setOrigin({ hintTxtBounds.position.x + hintTxtBounds.size.x / 2.f,
                                    hintTxtBounds.position.y }); // Origin Mid-Top

        sf::FloatRect hintSprBounds = m_hintSpr->getGlobalBounds();
        // Use sprite's actual center X and bottom Y + offset
        float hintIconCenterX = hintSprBounds.position.x + hintSprBounds.size.x / 2.f;
        float hintIconBottomY = hintSprBounds.position.y + hintSprBounds.size.y;
        m_hintCountTxt->setPosition({ hintIconCenterX - scaledHintOffsetX, hintIconBottomY + scaledHintOffsetY }); // Adjust positioning logic slightly if needed
        m_window.draw(*m_hintCountTxt);
    }

    // Hint Count Text
    if (m_hintCountTxt && m_hintSpr) { // Check pointers
        m_hintCountTxt->setCharacterSize(scaledHintFontSize); // <<< SET SCALED SIZE
        m_hintCountTxt->setString("Hints: " + std::to_string(m_hintsAvailable));
        m_hintCountTxt->setFillColor(m_currentTheme.hudTextFound);
        sf::FloatRect hintTxtBounds = m_hintCountTxt->getLocalBounds();
        m_hintCountTxt->setOrigin({ hintTxtBounds.position.x + hintTxtBounds.size.x / 2.f, hintTxtBounds.position.y });

        sf::FloatRect hintSprBounds = m_hintSpr->getGlobalBounds(); // Global bounds ARE scaled
        float hintIconCenterX = m_hintSpr->getPosition().x - hintSprBounds.size.x / 2.f; // Use scaled sprite pos
        float hintIconBottomY = m_hintSpr->getPosition().y + hintSprBounds.size.y / 2.f;
        // Use scaled offsets
        m_hintCountTxt->setPosition({ hintIconCenterX - scaledHintOffsetX, hintIconBottomY + scaledHintOffsetY });
        m_window.draw(*m_hintCountTxt);
    }


    //------------------------------------------------------------
    //  Draw Solved State overlay
    //------------------------------------------------------------
    if (m_currentScreen == GameScreen::GameOver) {
        // Prepare Elements
        sf::Text winTxt(m_font, "Puzzle Solved!", scaledSolvedFontSize); // USE SCALED SIZE
        winTxt.setFillColor(m_currentTheme.hudTextSolved);
        winTxt.setStyle(sf::Text::Bold);

        // Calculate bounds and required size
        sf::FloatRect winTxtBounds = winTxt.getLocalBounds(); // Depends on scaled size
        // Assume m_contBtn size/pos was set correctly in layout
        sf::Vector2f contBtnSize = m_contBtn.getSize();
        const float scaledPadding = S(this, 25.f); // Scale padding
        const float scaledSpacing = S(this, 20.f); // Scale spacing
        float overlayWidth = std::max(winTxtBounds.size.x, contBtnSize.x) + 2.f * scaledPadding;
        float overlayHeight = winTxtBounds.size.y + contBtnSize.y + scaledSpacing + 2.f * scaledPadding;

        // Set up the overlay background
        m_solvedOverlay.setSize({ overlayWidth, overlayHeight });
        m_solvedOverlay.setRadius(S(this, 15.f)); // Scale radius
        m_solvedOverlay.setFillColor(m_currentTheme.solvedOverlayBg);
        m_solvedOverlay.setOrigin({ overlayWidth / 2.f, overlayHeight / 2.f });
        // Use mapped center (mousePos is mapped, but window center needs mapping too if view is offset)
        // Safest: Use design space center mapped? Or just use screen center for overlay? Let's assume screen center for simplicity now.
        sf::Vector2f windowCenterPix = sf::Vector2f(m_window.getSize()) / 2.f;
        sf::Vector2f overlayCenter = m_window.mapPixelToCoords(sf::Vector2i(windowCenterPix));
        m_solvedOverlay.setPosition(overlayCenter);

        // Position elements centered within the overlay
        float winTxtCenterY = overlayCenter.y - overlayHeight / 2.f + scaledPadding + (winTxtBounds.position.y + winTxtBounds.size.y / 2.f);
        float contBtnPosY = winTxtCenterY + (winTxtBounds.size.y / 2.f) + scaledSpacing; // Y position relative to overlay center

        winTxt.setOrigin({ winTxtBounds.position.x + winTxtBounds.size.x / 2.f, winTxtBounds.position.y + winTxtBounds.size.y / 2.f });
        winTxt.setPosition({ overlayCenter.x, winTxtCenterY });

        // Ensure Continue button text size is also scaled before positioning
        if (m_contTxt) {
            m_contTxt->setCharacterSize(scaledContinueFontSize); // SET SCALED SIZE
            sf::FloatRect contTxtBounds = m_contTxt->getLocalBounds();
            m_contTxt->setOrigin({ contTxtBounds.position.x + contTxtBounds.size.x / 2.f, contTxtBounds.position.y + contTxtBounds.size.y / 2.f });
            // Position button relative to overlay center
            m_contBtn.setOrigin({ contBtnSize.x / 2.f, 0.f }); // Origin top-center
            m_contBtn.setPosition({ overlayCenter.x, contBtnPosY });
            // Position text ON the button
            m_contTxt->setPosition(m_contBtn.getPosition() + sf::Vector2f{ 0.f, contBtnSize.y / 2.f }); // Center Y on button
        }


        // Handle Continue Button Hover
        bool contHover = m_contBtn.getGlobalBounds().contains(mousePos);
        sf::Color continueHoverColor = adjustColorBrightness(m_currentTheme.continueButton, 1.2f);
        m_contBtn.setFillColor(contHover ? continueHoverColor : m_currentTheme.continueButton);

        // --- Draw Score/Bonus Particles (Positioning relies on target positions being scaled) ---
        // ... (particle drawing logic - ensure target positions are scaled) ...


        // Draw the overlay elements
        m_window.draw(m_solvedOverlay);
        m_window.draw(winTxt);
        m_window.draw(m_contBtn);
        if (m_contTxt) m_window.draw(*m_contTxt);
    }
}


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