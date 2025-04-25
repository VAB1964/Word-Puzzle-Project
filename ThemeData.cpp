#include "ThemeData.h"
#include "Theme.h" // Include ColorTheme definition again
#include <SFML/Graphics/Color.hpp> // Include sf::Color

std::vector<ColorTheme> loadThemes() {
    std::vector<ColorTheme> m_themes;

    // Define and add themes here using the commented format from above
// Theme 1: Default Blue/Green/Orange
    ColorTheme theme1 = {
        sf::Color(30, 37, 51),       // winBg
        sf::Color(45, 52, 67),       // decorBase
        sf::Color(208, 232, 242),    // decorAccent1
        sf::Color(155, 211, 255),    // decorAccent2
        sf::Color(100, 110, 120, 180),// gridEmptyTile (Adjusted example slightly for clarity)
        sf::Color(34, 161, 57),      // gridFilledTile
        sf::Color::White,            // gridLetter
        sf::Color(45, 52, 67, 150),  // wheelBg (Adjusted example slightly for clarity)
        sf::Color(155, 211, 255),    // wheelOutline (Adjusted example slightly for clarity)
        sf::Color(240, 240, 240, 220),// letterCircleNormal
        sf::Color(34, 161, 57, 230), // letterCircleHighlight
        sf::Color(30, 37, 51),       // letterTextNormal (Adjusted example slightly for clarity)
        sf::Color::White,            // letterTextHighlight
        sf::Color(0, 105, 255, 200), // dragLine
        sf::Color(100, 200, 100),    // continueButton
        sf::Color(255, 100, 0),      // hudTextGuess
        sf::Color(255, 100, 0),      // hudTextFound
        sf::Color::Green,            // hudTextSolved
        sf::Color(40, 50, 70, 210),  // solvedOverlayBg
        sf::Color(45, 52, 67, 230),  // scoreBarBg
        sf::Color(200, 200, 200),    // scoreTextLabel
        sf::Color::White,            // scoreTextValue
        sf::Color(40, 50, 70, 210),  // menuBg
        sf::Color(208, 232, 242),    // menuTitleText
        sf::Color(80, 90, 110),      // menuButtonNormal (Adjusted example slightly for clarity)
        sf::Color(120, 135, 150),    // menuButtonHover (Adjusted example slightly for clarity)
        sf::Color::White             // menuButtonText
    };
    m_themes.push_back(theme1);

    // Theme 2: Sunset Orange/Purple
    ColorTheme theme2 = {
        sf::Color(46, 34, 53),       // winBg
        sf::Color(69, 48, 78),       // decorBase
        sf::Color(236, 150, 127),    // decorAccent1
        sf::Color(255, 199, 138),    // decorAccent2
        sf::Color(100, 80, 90, 180), // gridEmptyTile
        sf::Color(217, 95, 67),      // gridFilledTile
        sf::Color::White,            // gridLetter
        sf::Color(69, 48, 78, 150),  // wheelBg
        sf::Color(236, 150, 127),    // wheelOutline
        sf::Color(255, 255, 255, 220),// letterCircleNormal
        sf::Color(217, 95, 67, 230), // letterCircleHighlight
        sf::Color(46, 34, 53),       // letterTextNormal
        sf::Color::White,            // letterTextHighlight
        sf::Color(247, 156, 41, 200),// dragLine
        sf::Color(217, 95, 67),      // continueButton
        sf::Color(255, 199, 138),    // hudTextGuess
        sf::Color(255, 199, 138),    // hudTextFound
        sf::Color(247, 156, 41),     // hudTextSolved
        sf::Color(69, 48, 78, 220),  // solvedOverlayBg
        sf::Color(69, 48, 78, 230),  // scoreBarBg
        sf::Color(236, 150, 127),    // scoreTextLabel
        sf::Color::White,            // scoreTextValue
        sf::Color(69, 48, 78, 210),  // menuBg
        sf::Color(236, 150, 127),    // menuTitleText
        sf::Color(100, 80, 90),      // menuButtonNormal
        sf::Color(130, 110, 120),    // menuButtonHover
        sf::Color::White             // menuButtonText
    };
    m_themes.push_back(theme2);

    // Theme 3: Forest Green/Brown (Matches the example you provided first)
    ColorTheme theme3 = {
        sf::Color(61, 44, 33),       // winBg
        sf::Color(94, 68, 54),       // decorBase
        sf::Color(245, 235, 220),    // decorAccent1
        sf::Color(218, 145, 70),     // decorAccent2
        sf::Color(188, 169, 147, 180),// gridEmptyTile
        sf::Color(218, 145, 70),     // gridFilledTile (Using accent2 like example)
        sf::Color(245, 235, 220),    // gridLetter
        sf::Color(138, 104, 73, 150),// wheelBg
        sf::Color(245, 235, 220),    // wheelOutline
        sf::Color(245, 235, 220, 220),// letterCircleNormal
        sf::Color(218, 145, 70, 230),// letterCircleHighlight
        sf::Color(61, 44, 33),       // letterTextNormal
        sf::Color(61, 44, 33),       // letterTextHighlight
        sf::Color(245, 235, 220, 200),// dragLine
        sf::Color(218, 145, 70),     // continueButton
        sf::Color(245, 235, 220),    // hudTextGuess
        sf::Color(245, 235, 220),    // hudTextFound
        sf::Color(245, 235, 220),    // hudTextSolved (Maybe change this one?)
        sf::Color(94, 68, 54, 210),  // solvedOverlayBg
        sf::Color(94, 68, 54, 230),  // scoreBarBg
        sf::Color(245, 235, 220),    // scoreTextLabel
        sf::Color::White,            // scoreTextValue
        sf::Color(94, 68, 54, 210),  // menuBg
        sf::Color(245, 235, 220),    // menuTitleText (Adjusted example slightly for clarity)
        sf::Color(138, 104, 73),     // menuButtonNormal (Adjusted example slightly for clarity)
        sf::Color(168, 148, 121),    // menuButtonHover (Adjusted example slightly for clarity)
        sf::Color(245, 235, 220)     // menuButtonText (Adjusted example slightly for clarity)
    };
    m_themes.push_back(theme3); // Assuming this was the original 'Coffee Shop'/Example

    // Theme 5: Retro/Synthwave (Dark Magenta/Cyan/Yellow)
    ColorTheme theme5 = {
        sf::Color(25, 20, 35),       // winBg
        sf::Color(40, 35, 50),       // decorBase
        sf::Color(0, 255, 255),      // decorAccent1 (Cyan)
        sf::Color(255, 0, 255),      // decorAccent2 (Magenta)
        sf::Color(60, 60, 70, 180),  // gridEmptyTile
        sf::Color(255, 0, 255),      // gridFilledTile (Magenta)
        sf::Color::White,            // gridLetter
        sf::Color(50, 50, 60, 150),  // wheelBg
        sf::Color(0, 255, 255),      // wheelOutline (Cyan)
        sf::Color(70, 60, 80, 220),  // letterCircleNormal
        sf::Color(0, 255, 255, 230), // letterCircleHighlight (Cyan)
        sf::Color::Yellow,           // letterTextNormal
        sf::Color(25, 20, 35),       // letterTextHighlight
        sf::Color(255, 255, 0, 200), // dragLine (Yellow)
        sf::Color(255, 0, 255),      // continueButton (Magenta)
        sf::Color(0, 255, 255),      // hudTextGuess (Cyan)
        sf::Color(0, 255, 255),      // hudTextFound (Cyan)
        sf::Color::Yellow,           // hudTextSolved
        sf::Color(40, 35, 50, 220),  // solvedOverlayBg
        sf::Color(40, 35, 50, 230),  // scoreBarBg
        sf::Color(0, 255, 255),      // scoreTextLabel (Cyan)
        sf::Color::Yellow,           // scoreTextValue
        sf::Color(40, 35, 50, 210),  // menuBg
        sf::Color(0, 255, 255),      // menuTitleText (Cyan)
        sf::Color(70, 60, 80),       // menuButtonNormal
        sf::Color(100, 90, 110),     // menuButtonHover
        sf::Color::Yellow            // menuButtonText
    };
    m_themes.push_back(theme5);

    // Theme 6: Grayscale/Monochrome (with Blue Highlight)
    ColorTheme theme6 = {
        sf::Color(40, 40, 40),       // winBg
        sf::Color(70, 70, 70),       // decorBase
        sf::Color(150, 150, 150),    // decorAccent1
        sf::Color(220, 220, 220),    // decorAccent2
        sf::Color(100, 100, 100, 180),// gridEmptyTile
        sf::Color(180, 180, 180),    // gridFilledTile 
        sf::Color::Black,            // gridLetter
        sf::Color(60, 60, 60, 150),  // wheelBg
        sf::Color(180, 180, 180),    // wheelOutline
        sf::Color(190, 190, 190, 220),// letterCircleNormal
        sf::Color(100, 150, 200, 230),// letterCircleHighlight (Blueish)
        sf::Color::Black,            // letterTextNormal
        sf::Color::White,            // letterTextHighlight
        sf::Color(100, 150, 200, 200),// dragLine (Blueish)
        sf::Color(100, 150, 200),    // continueButton (Blueish)
        sf::Color::White,            // hudTextGuess
        sf::Color::White,            // hudTextFound
        sf::Color(100, 150, 200),    // hudTextSolved (Blueish)
        sf::Color(70, 70, 70, 210),  // solvedOverlayBg
        sf::Color(70, 70, 70, 230),  // scoreBarBg
        sf::Color(220, 220, 220),    // scoreTextLabel
        sf::Color::White,            // scoreTextValue
        sf::Color(70, 70, 70, 210),  // menuBg
        sf::Color(220, 220, 220),    // menuTitleText
        sf::Color(100, 100, 100),    // menuButtonNormal
        sf::Color(130, 130, 130),    // menuButtonHover
        sf::Color::White             // menuButtonText
    };
    m_themes.push_back(theme6);
    // ... add others ...

    return m_themes;
}