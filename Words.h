#pragma once
#ifndef WORDS_H
#define WORDS_H

#include <string>
#include <vector>
#include <unordered_set>
#include "GameData.h" // Include necessary struct definition (WordInfo)

//--------------------------------------------------------------------
//  Word logic helpers (Declarations)
//--------------------------------------------------------------------
namespace Words {

    // Function to load words AND rarity from CSV
    std::vector<WordInfo> loadWordListWithRarity(const std::string& file);

    // Function to get words of a specific length from the loaded list
    std::vector<WordInfo> withLength(const std::vector<WordInfo>& wordList, std::size_t len);

    // Function to check if 'sub' can be formed from letters in 'base'
    bool isSubWord(const std::string& sub, const std::string& base);

    // Function to find all sub-words of 'base' within the loaded list
    std::vector<WordInfo> subWords(const std::string& base, const std::vector<WordInfo>& wordList);

    // Function to sort WordInfo objects for grid display (by length, then alpha)
    std::vector<WordInfo> sortForGrid(std::vector<WordInfo> v);

} // End namespace Words

#endif // WORDS_H