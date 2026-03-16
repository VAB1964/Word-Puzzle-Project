#pragma once
#ifndef CROSSWORD_H
#define CROSSWORD_H

#include <vector>
#include <map>
#include <utility>
#include "GameData.h"

enum class Direction { Horizontal, Vertical };

struct CrosswordPlacement {
    int gridRow = 0;
    int gridCol = 0;
    Direction dir = Direction::Horizontal;
};

struct CrosswordResult {
    std::vector<WordInfo> placedWords;
    std::vector<CrosswordPlacement> placements;
    std::map<std::pair<int, int>, std::vector<std::pair<int, int>>> sharedCells;
    int gridRows = 0;
    int gridCols = 0;
};

CrosswordResult generateCrossword(const std::vector<WordInfo>& words);

#endif // CROSSWORD_H
