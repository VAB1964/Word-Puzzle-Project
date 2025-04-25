// ThemeData.h
#pragma once // Or use include guards (#ifndef THEME_DATA_H ... #endif)

#include "theme.h" // Include the definition of ColorTheme struct
#include <vector>

// Declare the function that will load and return all defined themes
std::vector<ColorTheme> loadThemes();