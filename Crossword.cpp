#include "Crossword.h"

#include <algorithm>
#include <cctype>
#include <climits>
#include <iostream>
#include <random>
#include <string>

namespace {

struct PlacedWord {
    std::string text;
    int startRow;
    int startCol;
    Direction dir;
};

char cellAt(const std::vector<PlacedWord>& placed,
            const std::map<std::pair<int,int>, char>& occupiedCells,
            int r, int c)
{
    auto it = occupiedCells.find({r, c});
    return it != occupiedCells.end() ? it->second : '\0';
}

bool canPlace(const std::string& word, int startRow, int startCol, Direction dir,
              const std::map<std::pair<int,int>, char>& occupiedCells,
              int& intersectionCount)
{
    intersectionCount = 0;
    int len = static_cast<int>(word.size());

    for (int i = 0; i < len; ++i) {
        int r = (dir == Direction::Horizontal) ? startRow : startRow + i;
        int c = (dir == Direction::Horizontal) ? startCol + i : startCol;
        char existing = cellAt({}, occupiedCells, r, c);
        char wch = static_cast<char>(std::tolower(static_cast<unsigned char>(word[i])));

        if (existing != '\0') {
            if (existing != wch) return false;
            ++intersectionCount;
        } else {
            // Adjacent cells perpendicular to the word must be empty
            // (unless they are part of an intersecting word, handled by the intersection itself)
            if (dir == Direction::Horizontal) {
                char above = cellAt({}, occupiedCells, r - 1, c);
                char below = cellAt({}, occupiedCells, r + 1, c);
                if (above != '\0' || below != '\0') return false;
            } else {
                char left  = cellAt({}, occupiedCells, r, c - 1);
                char right = cellAt({}, occupiedCells, r, c + 1);
                if (left != '\0' || right != '\0') return false;
            }
        }
    }

    // Check cell before the word start
    int beforeR = (dir == Direction::Horizontal) ? startRow : startRow - 1;
    int beforeC = (dir == Direction::Horizontal) ? startCol - 1 : startCol;
    if (cellAt({}, occupiedCells, beforeR, beforeC) != '\0') return false;

    // Check cell after the word end
    int afterR = (dir == Direction::Horizontal) ? startRow : startRow + len;
    int afterC = (dir == Direction::Horizontal) ? startCol + len : startCol;
    if (cellAt({}, occupiedCells, afterR, afterC) != '\0') return false;

    return intersectionCount > 0;
}

void shuffleWithinLengthGroups(std::vector<WordInfo>& words, std::mt19937& rng) {
    size_t i = 0;
    while (i < words.size()) {
        size_t j = i;
        while (j < words.size() && words[j].text.size() == words[i].text.size()) ++j;
        if (j - i > 1) {
            std::shuffle(words.begin() + static_cast<std::ptrdiff_t>(i),
                         words.begin() + static_cast<std::ptrdiff_t>(j), rng);
        }
        i = j;
    }
}

} // anonymous namespace

static CrosswordResult generateCrosswordTrial(const std::vector<WordInfo>& words, std::mt19937& rng) {
    CrosswordResult result;
    if (words.empty()) return result;

    std::vector<WordInfo> sortedWords = words;
    std::sort(sortedWords.begin(), sortedWords.end(),
        [](const WordInfo& a, const WordInfo& b) {
            return a.text.size() > b.text.size();
        });
    shuffleWithinLengthGroups(sortedWords, rng);

    std::uniform_real_distribution<double> jitterDist(0.0, 4.0);

    std::vector<PlacedWord> placed;
    std::map<std::pair<int,int>, char> occupiedCells;

    {
        const std::string& firstWord = sortedWords[0].text;
        PlacedWord pw;
        pw.text = firstWord;
        pw.startRow = 0;
        pw.startCol = 0;
        pw.dir = Direction::Horizontal;
        placed.push_back(pw);

        for (int i = 0; i < static_cast<int>(firstWord.size()); ++i) {
            char ch = static_cast<char>(std::tolower(static_cast<unsigned char>(firstWord[i])));
            occupiedCells[{0, i}] = ch;
        }
    }

    std::vector<bool> isPlaced(sortedWords.size(), false);
    isPlaced[0] = true;

    int curMinRow = 0, curMaxRow = 0;
    int curMinCol = 0, curMaxCol = static_cast<int>(sortedWords[0].text.size()) - 1;

    for (size_t wi = 1; wi < sortedWords.size(); ++wi) {
        const std::string& candidate = sortedWords[wi].text;
        std::string lowerCandidate = candidate;
        std::transform(lowerCandidate.begin(), lowerCandidate.end(), lowerCandidate.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        double bestScore = -1.0;
        int bestRow = 0, bestCol = 0;
        Direction bestDir = Direction::Horizontal;

        for (size_t pi = 0; pi < placed.size(); ++pi) {
            const PlacedWord& pw = placed[pi];
            std::string lowerPlaced = pw.text;
            std::transform(lowerPlaced.begin(), lowerPlaced.end(), lowerPlaced.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

            Direction newDir = (pw.dir == Direction::Horizontal) ? Direction::Vertical : Direction::Horizontal;

            for (int ci = 0; ci < static_cast<int>(lowerCandidate.size()); ++ci) {
                for (int pj = 0; pj < static_cast<int>(lowerPlaced.size()); ++pj) {
                    if (lowerCandidate[ci] != lowerPlaced[pj]) continue;

                    int placedCellR = (pw.dir == Direction::Horizontal) ? pw.startRow : pw.startRow + pj;
                    int placedCellC = (pw.dir == Direction::Horizontal) ? pw.startCol + pj : pw.startCol;

                    int startRow, startCol;
                    if (newDir == Direction::Horizontal) {
                        startRow = placedCellR;
                        startCol = placedCellC - ci;
                    } else {
                        startRow = placedCellR - ci;
                        startCol = placedCellC;
                    }

                    int intersections = 0;
                    if (canPlace(lowerCandidate, startRow, startCol, newDir, occupiedCells, intersections)) {
                        int candLen = static_cast<int>(lowerCandidate.size());
                        int endR = (newDir == Direction::Horizontal) ? startRow : startRow + candLen - 1;
                        int endC = (newDir == Direction::Horizontal) ? startCol + candLen - 1 : startCol;
                        int newMinR = std::min(curMinRow, startRow);
                        int newMaxR = std::max(curMaxRow, endR);
                        int newMinC = std::min(curMinCol, startCol);
                        int newMaxC = std::max(curMaxCol, endC);
                        int newRows = newMaxR - newMinR + 1;
                        int newCols = newMaxC - newMinC + 1;

                        if (newRows > newCols) continue;

                        int widthBonus = (newCols - newRows) * 5;
                        double randomJitter = jitterDist(rng);
                        double score = intersections * 10.0 + static_cast<double>(lowerCandidate.size()) + widthBonus + randomJitter;
                        if (score > bestScore) {
                            bestScore = score;
                            bestRow = startRow;
                            bestCol = startCol;
                            bestDir = newDir;
                        }
                    }
                }
            }
        }

        if (bestScore > 0) {
            PlacedWord pw;
            pw.text = candidate;
            pw.startRow = bestRow;
            pw.startCol = bestCol;
            pw.dir = bestDir;
            placed.push_back(pw);
            isPlaced[wi] = true;

            int candLen = static_cast<int>(lowerCandidate.size());
            int endR = (bestDir == Direction::Horizontal) ? bestRow : bestRow + candLen - 1;
            int endC = (bestDir == Direction::Horizontal) ? bestCol + candLen - 1 : bestCol;
            curMinRow = std::min(curMinRow, bestRow);
            curMaxRow = std::max(curMaxRow, endR);
            curMinCol = std::min(curMinCol, bestCol);
            curMaxCol = std::max(curMaxCol, endC);

            for (int i = 0; i < static_cast<int>(lowerCandidate.size()); ++i) {
                int r = (bestDir == Direction::Horizontal) ? bestRow : bestRow + i;
                int c = (bestDir == Direction::Horizontal) ? bestCol + i : bestCol;
                char ch = static_cast<char>(std::tolower(static_cast<unsigned char>(candidate[i])));
                occupiedCells[{r, c}] = ch;
            }
        }
    }

    int minRow = INT_MAX, minCol = INT_MAX;
    int maxRow = INT_MIN, maxCol = INT_MIN;

    for (const auto& pw : placed) {
        int endRow = (pw.dir == Direction::Horizontal) ? pw.startRow : pw.startRow + static_cast<int>(pw.text.size()) - 1;
        int endCol = (pw.dir == Direction::Horizontal) ? pw.startCol + static_cast<int>(pw.text.size()) - 1 : pw.startCol;
        minRow = std::min(minRow, pw.startRow);
        minCol = std::min(minCol, pw.startCol);
        maxRow = std::max(maxRow, endRow);
        maxCol = std::max(maxCol, endCol);
    }

    result.gridRows = maxRow - minRow + 1;
    result.gridCols = maxCol - minCol + 1;

    for (size_t i = 0; i < placed.size(); ++i) {
        const PlacedWord& pw = placed[i];
        CrosswordPlacement cp;
        cp.gridRow = pw.startRow - minRow;
        cp.gridCol = pw.startCol - minCol;
        cp.dir = pw.dir;
        result.placements.push_back(cp);

        for (const auto& wi : sortedWords) {
            std::string lowerWi = wi.text;
            std::transform(lowerWi.begin(), lowerWi.end(), lowerWi.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            std::string lowerPw = pw.text;
            std::transform(lowerPw.begin(), lowerPw.end(), lowerPw.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (lowerWi == lowerPw) {
                result.placedWords.push_back(wi);
                break;
            }
        }
    }

    std::map<std::pair<int,int>, std::vector<std::pair<int,int>>> cellOwners;

    for (size_t wi = 0; wi < result.placements.size(); ++wi) {
        const CrosswordPlacement& cp = result.placements[wi];
        const std::string& word = result.placedWords[wi].text;

        for (int ci = 0; ci < static_cast<int>(word.size()); ++ci) {
            int r = (cp.dir == Direction::Horizontal) ? cp.gridRow : cp.gridRow + ci;
            int c = (cp.dir == Direction::Horizontal) ? cp.gridCol + ci : cp.gridCol;
            cellOwners[{r, c}].push_back({static_cast<int>(wi), ci});
        }
    }

    for (const auto& pair : cellOwners) {
        if (pair.second.size() > 1) {
            result.sharedCells[pair.first] = pair.second;
        }
    }

    return result;
}

CrosswordResult generateCrossword(const std::vector<WordInfo>& words) {
    if (words.empty()) return CrosswordResult{};

    std::random_device rd;
    std::mt19937 rng(rd());

    const int NUM_TRIALS = 20;
    CrosswordResult bestResult;
    double bestScore = -1e9;

    for (int trial = 0; trial < NUM_TRIALS; ++trial) {
        CrosswordResult result = generateCrosswordTrial(words, rng);
        int placed = static_cast<int>(result.placedWords.size());
        double ratio = result.gridCols / static_cast<double>(std::max(result.gridRows, 1));
        double score = placed * 1000.0 + ratio * 100.0 - result.gridRows * 10.0;
        if (score > bestScore) {
            bestScore = score;
            bestResult = std::move(result);
        }
    }

    std::cout << "Crossword: placed " << bestResult.placedWords.size()
              << " of " << words.size() << " words into a "
              << bestResult.gridRows << "x" << bestResult.gridCols << " grid with "
              << bestResult.sharedCells.size() << " intersections"
              << " (best of " << NUM_TRIALS << " trials)." << std::endl;

    return bestResult;
}
