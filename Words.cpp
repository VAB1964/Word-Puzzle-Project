#include "Words.h" // Include the header file with declarations

#include <fstream>
#include <string>
#include <vector>
#include <unordered_set>
#include <algorithm> // For std::sort, std::all_of, std::remove, std::transform, std::clamp
#include <iostream>  // For std::cerr
#include <stdexcept> // For std::invalid_argument, std::out_of_range

//--------------------------------------------------------------------
//  Word logic helpers (Definitions)
//--------------------------------------------------------------------
namespace Words {

    // Function to load words AND rarity from CSV
    std::vector<WordInfo> loadWordListWithRarity(const std::string& file) {
        std::vector<WordInfo> wordList;
        std::ifstream in(file);
        if (!in) {
            std::cerr << "Error opening word list file: " << file << std::endl;
            return wordList; // Return empty list on error
        }

        std::string line;
        // Skip header line if your CSV has one (e.g., "word,rarity")
        // std::getline(in, line); // Uncomment if you have a header

        while (std::getline(in, line)) {
            // Basic check for empty lines or lines starting with non-alpha
            if (line.empty() || !std::isalpha(static_cast<unsigned char>(line[0]))) { // Added cast for safety
                continue;
            }

            std::size_t commaPos = line.find(',');
            if (commaPos != std::string::npos && commaPos > 0) {
                WordInfo info;
                info.text = line.substr(0, commaPos);
                // Clean up word
                info.text.erase(std::remove(info.text.begin(), info.text.end(), '\"'), info.text.end());
                info.text.erase(std::remove(info.text.begin(), info.text.end(), ' '), info.text.end());
                info.text.erase(std::remove(info.text.begin(), info.text.end(), '\r'), info.text.end()); // Remove potential carriage returns
                std::transform(info.text.begin(), info.text.end(), info.text.begin(), ::tolower);

                if (info.text.empty() || !std::all_of(info.text.begin(), info.text.end(), ::isalpha)) {
                    // std::cerr << "Warning: Skipping non-alphabetic word after cleaning: '" << info.text << "' from line: " << line << std::endl;
                    continue;
                }
                if (info.text.length() < 3 || info.text.length() > 7) {
                    // std::cerr << "Skipping word due to length: " << info.text << std::endl;
                    continue;
                }

                try {
                    std::string rarityStr = line.substr(commaPos + 1);
                    rarityStr.erase(std::remove(rarityStr.begin(), rarityStr.end(), '\"'), rarityStr.end());
                    rarityStr.erase(std::remove(rarityStr.begin(), rarityStr.end(), ' '), rarityStr.end());
                    rarityStr.erase(std::remove(rarityStr.begin(), rarityStr.end(), '\r'), rarityStr.end());

                    info.rarity = std::stoi(rarityStr);
                    info.rarity = std::clamp(info.rarity, 1, 4);
                }
                catch (const std::invalid_argument& e) {
                    std::cerr << "Warning: Invalid rarity format for word '" << info.text << "'. Defaulting to 4. Error: " << e.what() << std::endl;
                    info.rarity = 4;
                }
                catch (const std::out_of_range& e) {
                    std::cerr << "Warning: Rarity value out of range for word '" << info.text << "'. Defaulting to 4. Error: " << e.what() << std::endl;
                    info.rarity = 4;
                }
                wordList.push_back(info);
            }
            else {
                std::cerr << "Warning: Skipping invalid CSV line (no comma?): " << line << std::endl;
            }
        }
        std::cout << "Loaded " << wordList.size() << " valid words with rarity from " << file << std::endl;
        return wordList;
    }

    // Get words of a specific length
    std::vector<WordInfo> withLength(const std::vector<WordInfo>& wordList, std::size_t len) {
        std::vector<WordInfo> out;
        if (len == 0) return out;
        for (const auto& wi : wordList) {
            if (wi.text.size() == len) {
                out.push_back(wi);
            }
        }
        return out;
    }

    // Check if 'sub' can be formed from letters in 'base'
    bool isSubWord(const std::string& sub, const std::string& base) {
        if (sub.empty()) return true;
        if (sub.size() > base.size()) return false;
        std::string tempBase = base;
        for (char subChar : sub) {
            auto pos = tempBase.find(subChar);
            if (pos == std::string::npos) {
                return false;
            }
            tempBase.erase(pos, 1);
        }
        return true;
    }

    // Find all sub-words
    std::vector<WordInfo> subWords(const std::string& base, const std::vector<WordInfo>& wordList) {
        std::vector<WordInfo> out;
        if (base.empty()) return out;
        std::string lowerBase = base; // Work with lowercase base
        std::transform(lowerBase.begin(), lowerBase.end(), lowerBase.begin(), ::tolower);

        for (const auto& wi : wordList) {
            // Ensure wi.text is also lowercase for comparison if needed, though load function does it
            if (wi.text.size() <= lowerBase.size() && isSubWord(wi.text, lowerBase)) {
                out.push_back(wi);
            }
        }
        return out;
    }

    // Sort words for grid display
    std::vector<WordInfo> sortForGrid(std::vector<WordInfo> v) {
        std::sort(v.begin(), v.end(), [](const WordInfo& a, const WordInfo& b) {
            if (a.text.size() != b.text.size()) return a.text.size() < b.text.size();
            return a.text < b.text;
            });
        return v;
    }

} // End namespace Words