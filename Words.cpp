#include "Words.h"
#include "GameData.h" // Ensure WordInfo definition is available

// *** Standard Includes ***
#include <fstream>      // For std::ifstream
#include <sstream>      // For std::stringstream
#include <iostream>     // For std::cerr, std::cout
#include <string>       // For std::string, std::getline, std::stoi, std::stof
#include <vector>       // For std::vector
#include <stdexcept>    // For std::invalid_argument, std::out_of_range
#include <algorithm>    // For std::sort, std::all_of
#include <cctype>       // For std::tolower
#include <map>  
// ***********************************


namespace Words {

    namespace {
        std::string trimString(const std::string& input) {
            size_t first = input.find_first_not_of(" \t\n\r\f\v");
            if (first == std::string::npos) {
                return "";
            }
            size_t last = input.find_last_not_of(" \t\n\r\f\v");
            return input.substr(first, (last - first + 1));
        }

        std::vector<std::string> parseCsvLine(const std::string& line) {
            std::vector<std::string> fields;
            std::string field;
            bool inQuotes = false;

            for (size_t i = 0; i < line.size(); ++i) {
                char c = line[i];
                if (c == '"') {
                    if (inQuotes && i + 1 < line.size() && line[i + 1] == '"') {
                        field.push_back('"');
                        ++i;
                    }
                    else {
                        inQuotes = !inQuotes;
                    }
                }
                else if (c == ',' && !inQuotes) {
                    fields.push_back(field);
                    field.clear();
                }
                else {
                    field.push_back(c);
                }
            }
            fields.push_back(field);
            return fields;
        }

        bool readCsvRecord(std::istream& in, std::string& out, int& lineNum) {
            out.clear();
            std::string line;
            bool inQuotes = false;
            bool anyRead = false;

            while (std::getline(in, line)) {
                lineNum++;
                if (!out.empty()) out += "\n";
                out += line;
                anyRead = true;

                for (size_t i = 0; i < line.size(); ++i) {
                    if (line[i] == '"') {
                        if (inQuotes && i + 1 < line.size() && line[i + 1] == '"') {
                            ++i;
                        }
                        else {
                            inQuotes = !inQuotes;
                        }
                    }
                }

                if (!inQuotes) {
                    return true;
                }
            }

            return anyRead && !out.empty();
        }
    }

    // Function to load the pre-processed word list
    std::vector<WordInfo> loadProcessedWordList(const std::string& filename) {
        std::vector<WordInfo> wordList;
        std::ifstream file(filename);
        std::string line;

        if (!file.is_open()) {
            std::cerr << "Error: Could not open processed word list file: " << filename << std::endl;
            return wordList; // Return empty list on failure
        }

        int lineNum = 0; // For error reporting (physical lines)
        // Optional: Skip header row if your CSV has one
        readCsvRecord(file, line, lineNum);

        while (readCsvRecord(file, line, lineNum)) {
            WordInfo info;

            try {
                std::vector<std::string> fields = parseCsvLine(line);
                int fieldIndex = static_cast<int>(fields.size());

                if (fieldIndex > 0) {
                    info.text = trimString(fields[0]);
                    std::transform(info.text.begin(), info.text.end(), info.text.begin(),
                        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                }
                if (fieldIndex > 1) info.rarity = trimString(fields[1]).empty() ? 0 : std::stoi(trimString(fields[1]));
                if (fieldIndex > 2) info.pos = trimString(fields[2]);
                if (fieldIndex > 3) info.definition = trimString(fields[3]);
                if (fieldIndex > 4) info.sentence = trimString(fields[4]);

                // Basic validation
                if (fieldIndex >= 2 && !info.text.empty()) {
                    wordList.push_back(info);
                }
                else if (!line.empty()) {
                    std::cerr << "Warning: Skipping malformed line " << lineNum << " in " << filename << " (parsed " << fieldIndex << " fields)" << std::endl;
                }

            }
            catch (const std::invalid_argument& ia) {
                std::cerr << "Warning: Invalid number format on line " << lineNum << " in " << filename << ". Line content: [" << line << "]. Skipping line. Error: " << ia.what() << std::endl;
            }
            catch (const std::out_of_range& oor) {
                std::cerr << "Warning: Number out of range on line " << lineNum << " in " << filename << ". Line content: [" << line << "]. Skipping line. Error: " << oor.what() << std::endl;
            }
            catch (const std::exception& e) {
                std::cerr << "Warning: Unexpected error parsing line " << lineNum << " in " << filename << ". Skipping line. Error: " << e.what() << std::endl;
            }
        }

        file.close();
        std::cout << "Successfully loaded " << wordList.size() << " words from processed file: " << filename << std::endl;
        return wordList;
    }


    // *** DEFINITION for withLength ***
    std::vector<WordInfo> withLength(const std::vector<WordInfo>& wordList, std::size_t len) {
        std::vector<WordInfo> result;
        for (const auto& info : wordList) {
            if (info.text.length() == len) {
                result.push_back(info);
            }
        }
        return result;
    }


    // *** DEFINITION for isSubWord (Helper) ***
    // Checks if 'sub' can be formed using only letters from 'base'
    // Case-insensitive comparison
    bool isSubWord(const std::string& sub, const std::string& base) {
        if (sub.empty() || sub.length() > base.length()) {
            return false; // Cannot be sub-word if empty or longer
        }
        if (sub == base) {
            return false; // A word isn't its own sub-word in this context
        }

        // Use frequency maps for efficient checking
        std::map<char, int> baseFreq;
        for (char c : base) {
            baseFreq[std::tolower(c)]++;
        }

        std::map<char, int> subFreq;
        for (char c : sub) {
            subFreq[std::tolower(c)]++;
        }

        // Check if every character in 'sub' is present in 'base' with sufficient frequency
        for (const auto& pair : subFreq) {
            char subChar = pair.first;
            int subCount = pair.second;
            if (baseFreq.find(subChar) == baseFreq.end() || baseFreq[subChar] < subCount) {
                return false; // Character not found in base or not enough occurrences
            }
        }

        return true; // All characters in 'sub' are accounted for in 'base'
    }


    // *** CORRECTED DEFINITION for subWords ***
    // Finds all words in the dictionary that can be formed from the letters of 'base'
    // (excluding the base word itself).
    std::vector<WordInfo> subWords(const std::string& base, const std::vector<WordInfo>& wordList) {
        std::vector<WordInfo> result;
        if (base.empty()) { // Handle empty base case
            return result;
        }

        std::string lowerBase = base;
        std::transform(lowerBase.begin(), lowerBase.end(), lowerBase.begin(),
            [](unsigned char c) { return std::tolower(c); }); // Use lambda for safety

        std::map<char, int> baseFreq;
        for (char c : lowerBase) {
            baseFreq[c]++;
        }

        for (const auto& info : wordList) { // Iterate through the entire dictionary
            const std::string& potentialWord = info.text;

            // Skip empty words or words longer than the base
            if (potentialWord.empty() || potentialWord.length() > base.length()) {
                continue;
            }

            std::string lowerWord = potentialWord;
            std::transform(lowerWord.begin(), lowerWord.end(), lowerWord.begin(),
                [](unsigned char c) { return std::tolower(c); });

            // Check if it's the base word itself (case-insensitive) - skip if it is
            if (lowerWord == lowerBase) {
                continue;
            }

            // Check using frequency map method
            std::map<char, int> wordFreq;
            bool possible = true;
            for (char c : lowerWord) {
                wordFreq[c]++;
                // Early exit: If char not in base, or count exceeds base count, impossible.
                auto it = baseFreq.find(c);
                if (it == baseFreq.end() || wordFreq[c] > it->second) {
                    possible = false;
                    break;
                }
            }

            if (possible) {
                result.push_back(info); // Add the original WordInfo object
            }
        }
        std::cout << "DEBUG: Words::subWords found " << result.size() << " valid sub-words for base '" << base << "' (excluding base)." << std::endl; // Add debug output
        return result;
    }


    // *** DEFINITION for sortForGrid (FIXED) ***
    // Sorts by length ascending, then alphabetically (case-insensitive)
    std::vector<WordInfo> sortForGrid(std::vector<WordInfo> v) { // Pass by value ok
        std::sort(v.begin(), v.end(), [](const WordInfo& a, const WordInfo& b) {
            if (a.text.length() != b.text.length()) {
                return a.text.length() < b.text.length(); // Shorter first
            }
            // Case-insensitive comparison for same length words
            return std::lexicographical_compare(
                a.text.begin(), a.text.end(),
                b.text.begin(), b.text.end(),
                [](unsigned char c1, unsigned char c2) {
                    return std::tolower(c1) < std::tolower(c2);
                });
            // Original (case-sensitive): return a.text < b.text;
            });
        return v; // Return the sorted vector
    }


    // Optional: Definition for the original loadWordListWithRarity if still needed
    /*
    std::vector<WordInfo> loadWordListWithRarity(const std::string& file) {
        // ... implementation to read simple "word,rarity" CSV ...
    }
    */

} // namespace Words