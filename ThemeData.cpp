#include "ThemeData.h"
#include "theme.h" // Include ColorTheme definition again
#include <vector> 
#include <SFML/Graphics/Color.hpp> // Include sf::Color

std::vector<ColorTheme> loadThemes()
{
    std::vector<ColorTheme> m_themes;

    // Define and add themes here using the commented format from above
// Theme 1: Default Blue/Green/Orange
    ColorTheme theme1 = {
        sf::Color(30, 37, 51),       // winBg
        sf::Color(45, 52, 67),       // decorBase
        sf::Color(208, 232, 242),    // decorAccent1
        sf::Color(155, 211, 255),    // decorAccent2
        sf::Color(100, 110, 120, 180),// gridEmptyTile (Adjusted example slightly for clarity)
        sf::Color(155, 211, 255),      // gridFilledTile
        sf::Color::Black,            // gridLetter
        sf::Color(45, 52, 67, 150),  // wheelBg (Adjusted example slightly for clarity)
        sf::Color(155, 211, 255),    // wheelOutline (Adjusted example slightly for clarity)
        sf::Color(240, 240, 240, 220),// letterCircleNormal
        sf::Color(34, 161, 57, 230), // letterCircleHighlight
        sf::Color(30, 37, 51),       // letterTextNormal (Adjusted example slightly for clarity)
        sf::Color::White,            // letterTextHighlight
        sf::Color(0, 105, 255, 200), // dragLine
        sf::Color(100, 200, 100),    // continueButton
        sf::Color(155, 211, 255),      // hudTextGuess
        sf::Color(255, 100, 0),      // hudTextFound
        sf::Color(155, 211, 255),    // hudTextSolved
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
        sf::Color::White,            // gridLetter
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

    // --- Theme 7: "Spring Meadow" (Light Greens/Blues) ---
    ColorTheme theme7 = {
        sf::Color(105, 184, 110),      // winBg (Very light minty green/blue)
        sf::Color(245, 247, 22),      // decorBase (Soft green)
        sf::Color(255, 190, 200, 230),      // decorAccent1 (Slightly darker green)
        sf::Color(255, 255, 200),      // decorAccent2 (Pale yellow)
        sf::Color(190, 205, 195, 180), // gridEmptyTile (Light greenish gray)
        sf::Color(90, 170, 100),       // gridFilledTile (Leafy green)
        sf::Color(190, 205, 195),         // gridLetter (Dark forest green)
        sf::Color(180, 220, 190, 150), // wheelBg (Light green, semi-transparent)
        sf::Color(110, 160, 120),      // wheelOutline (Medium green)
        sf::Color(240, 255, 245, 220), // letterCircleNormal (Off-white)
        sf::Color(255, 190, 200, 230), // letterCircleHighlight (Soft pink/peach)
        sf::Color(40, 70, 45),         // letterTextNormal (Dark forest green)
        sf::Color(40, 70, 45),         // letterTextHighlight (Dark forest green)
        sf::Color(80, 180, 90, 200),   // dragLine (Bright green)
        sf::Color(90, 170, 100),       // continueButton (Leafy green)
        sf::Color(50, 100, 55),        // hudTextGuess (Darker green)
        sf::Color(50, 100, 55),        // hudTextFound (Darker green)
        sf::Color(0, 180, 0),          // hudTextSolved (Bright green)
        sf::Color(180, 220, 190, 210), // solvedOverlayBg (Light green overlay)
        sf::Color(170, 210, 180, 230), // scoreBarBg (Soft green)
        sf::Color(70, 110, 75),        // scoreTextLabel (Medium dark green)
        sf::Color(40, 70, 45),         // scoreTextValue (Dark forest green)
        sf::Color(180, 220, 190, 220), // menuBg
        sf::Color(40, 70, 45),         // menuTitleText
        sf::Color(140, 190, 150),      // menuButtonNormal
        sf::Color(110, 160, 120),      // menuButtonHover
        sf::Color(40, 70, 45)          // menuButtonText
    };
    m_themes.push_back(theme7);

    // --- Theme 8: "Deep Ocean" (Dark Blues/Teal) ---
    ColorTheme theme8 = {
        sf::Color(15, 25, 45),         // winBg (Deep midnight blue)
        sf::Color(20, 35, 60),         // decorBase (Darker blue)
        sf::Color(35, 55, 85),         // decorAccent1 (Medium dark blue)
        sf::Color(0, 130, 140),        // decorAccent2 (Teal)
        sf::Color(50, 60, 80, 180),    // gridEmptyTile (Dark blue-gray)
        sf::Color(0, 180, 170),        // gridFilledTile (Bright Aqua/Turquoise)
        sf::Color(210, 245, 255),      // gridLetter (Very light cyan/white)
        sf::Color(40, 60, 90, 150),    // wheelBg (Medium blue, semi-transparent)
        sf::Color(80, 120, 180),       // wheelOutline (Lighter dusty blue)
        sf::Color(190, 210, 225, 220), // letterCircleNormal (Light blue-gray)
        sf::Color(0, 180, 170, 230),   // letterCircleHighlight (Aqua highlight)
        sf::Color(15, 25, 45),         // letterTextNormal (Matches background)
        sf::Color(15, 25, 45),         // letterTextHighlight (Matches background)
        sf::Color(0, 200, 190, 200),   // dragLine (Aqua)
        sf::Color(0, 150, 130),        // continueButton (Teal)
        sf::Color(180, 230, 240),      // hudTextGuess (Light cyan)
        sf::Color(180, 230, 240),      // hudTextFound (Light cyan)
        sf::Color(50, 220, 210),       // hudTextSolved (Bright Aqua)
        sf::Color(25, 40, 65, 210),    // solvedOverlayBg (Dark blue overlay)
        sf::Color(35, 55, 85, 230),    // scoreBarBg (Medium dark blue)
        sf::Color(150, 190, 220),      // scoreTextLabel (Light dusty blue)
        sf::Color(210, 245, 255),      // scoreTextValue (Light cyan/white)
        sf::Color(25, 40, 65, 220),    // menuBg
        sf::Color(210, 245, 255),      // menuTitleText
        sf::Color(50, 80, 120),        // menuButtonNormal
        sf::Color(80, 120, 180),       // menuButtonHover
        sf::Color(210, 245, 255)       // menuButtonText
    };
    m_themes.push_back(theme8);

    // --- Theme 9: "Paper & Ink" (Cream/Black/Gray) ---
    ColorTheme theme9 = {
        sf::Color(78, 156, 199),      // winBg 
        sf::Color(225, 225, 215),      // decorBase (Very light gray)
        sf::Color(205, 205, 195),      // decorAccent1 (Slightly darker gray)
        sf::Color(185, 185, 175),      // decorAccent2 (Medium light gray)
        sf::Color(154, 199, 198, 180), // gridEmptyTile (Light warm gray)
        sf::Color::White,              // gridFilledTile (Dark gray)
        sf::Color::Black,              // gridLetter
        sf::Color(215, 215, 205, 150), // wheelBg (Light gray, semi-transparent)
        sf::Color(140, 140, 130),      // wheelOutline (Medium gray)
        sf::Color(255, 255, 255, 220), // letterCircleNormal (White)
        sf::Color(190, 210, 240, 230), // letterCircleHighlight (Subtle light blue)
        sf::Color::Black,              // letterTextNormal
        sf::Color::Black,              // letterTextHighlight
        sf::Color(80, 80, 80, 200),    // dragLine (Dark gray)
        sf::Color(90, 90, 90),         // continueButton (Dark gray)
        sf::Color(181, 179, 152),         // hudTextGuess (Near black)
        sf::Color(40, 40, 40),         // hudTextFound (Near black)
        sf::Color(0, 100, 0),          // hudTextSolved (Dark green for contrast)
        sf::Color(215, 215, 205, 210), // solvedOverlayBg (Light gray overlay)
        sf::Color(205, 205, 195, 230), // scoreBarBg (Slightly darker gray)
        sf::Color(80, 80, 80),         // scoreTextLabel (Dark gray)
        sf::Color(20, 20, 20),         // scoreTextValue (Near black)
        sf::Color(215, 215, 205, 220), // menuBg
        sf::Color(20, 20, 20),         // menuTitleText
        sf::Color(170, 170, 160),      // menuButtonNormal
        sf::Color(140, 140, 130),      // menuButtonHover
        sf::Color(20, 20, 20)          // menuButtonText
    };
    m_themes.push_back(theme9);

    // --- Theme 10: "Autumn Forest" (Browns/Oranges/Reds) ---
    ColorTheme theme10 = {
        sf::Color(55, 30, 15),         // winBg (Deep brown)
        sf::Color(85, 50, 25),         // decorBase (Medium brown)
        sf::Color(125, 70, 35),        // decorAccent1 (Lighter wood brown)
        sf::Color(190, 80, 30),        // decorAccent2 (Burnt orange)
        sf::Color(100, 70, 45, 180),   // gridEmptyTile (Medium brown, transparent)
        sf::Color(200, 100, 30),       // gridFilledTile (Burnt orange)
        sf::Color(255, 245, 210),      // gridLetter (Pale yellow/cream)
        sf::Color(75, 45, 20, 150),    // wheelBg (Darker brown, semi-transparent)
        sf::Color(140, 90, 50),        // wheelOutline (Lighter wood brown)
        sf::Color(240, 230, 200, 220), // letterCircleNormal (Cream)
        sf::Color(230, 130, 40, 230),  // letterCircleHighlight (Vibrant orange)
        sf::Color(55, 30, 15),         // letterTextNormal (Deep brown)
        sf::Color(55, 30, 15),         // letterTextHighlight (Deep brown)
        sf::Color(230, 130, 40, 200),  // dragLine (Vibrant orange)
        sf::Color(180, 90, 25),        // continueButton (Dark orange/brown)
        sf::Color(255, 225, 180),      // hudTextGuess (Light cream/peach)
        sf::Color(255, 225, 180),      // hudTextFound (Light cream/peach)
        sf::Color(255, 150, 0),        // hudTextSolved (Bright orange)
        sf::Color(85, 50, 25, 210),    // solvedOverlayBg (Medium brown overlay)
        sf::Color(85, 50, 25, 230),    // scoreBarBg (Medium brown)
        sf::Color(200, 180, 150),      // scoreTextLabel (Light brown/beige)
        sf::Color(255, 245, 210),      // scoreTextValue (Pale yellow/cream)
        sf::Color(85, 50, 25, 220),    // menuBg
        sf::Color(255, 245, 210),      // menuTitleText
        sf::Color(125, 70, 35),        // menuButtonNormal
        sf::Color(155, 90, 45),        // menuButtonHover
        sf::Color(255, 245, 210)       // menuButtonText
    };
    m_themes.push_back(theme10);
    // ... add others ...

    return m_themes;
}