#pragma once
#ifndef WORDS_H
#define WORDS_H

#include <string>
#include <vector>
#include <unordered_set> // Keep if used by other functions
#include "GameData.h" // Include necessary struct definition (WordInfo)

//--------------------------------------------------------------------
//  Word logic helpers (Declarations)
//--------------------------------------------------------------------
namespace Words {

    // Function to load words AND rarity from CSV (Original, keep if needed elsewhere)
    // std::vector<WordInfo> loadWordListWithRarity(const std::string& file); // Can likely be removed if not used

    // *** ADD THIS DECLARATION FOR THE NEW FUNCTION ***
    std::vector<WordInfo> loadProcessedWordList(const std::string& filename);
    // ************************************************

    // Function to get words of a specific length from the loaded list
    std::vector<WordInfo> withLength(const std::vector<WordInfo>& wordList, std::size_t len);

    // Function to check if 'sub' can be formed from letters in 'base'
    bool isSubWord(const std::string& sub, const std::string& base);

    // Function to find all sub-words of 'base' within the loaded list
    // Note: Ensure this function works correctly with the extended WordInfo if it relies on more than text/rarity
    std::vector<WordInfo> subWords(const std::string& base, const std::vector<WordInfo>& wordList);

    // Function to sort WordInfo objects for grid display (by length, then alpha)
    std::vector<WordInfo> sortForGrid(std::vector<WordInfo> v); // Pass by value is okay if you modify copy

} // End namespace Words

#endif // WORDS_H