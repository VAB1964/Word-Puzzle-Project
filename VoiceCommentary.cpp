#include "VoiceCommentary.h"

#include <windows.h>
#include <sapi.h>
#include <algorithm>
#include <random>
#include <iostream>

static std::mt19937& rng() {
    static std::mt19937 gen(std::random_device{}());
    return gen;
}

VoiceCommentary::VoiceCommentary() {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    m_comInitialized = SUCCEEDED(hr) || hr == S_FALSE; // S_FALSE means already initialized

    if (m_comInitialized) {
        hr = CoCreateInstance(CLSID_SpVoice, nullptr, CLSCTX_ALL, IID_ISpVoice, reinterpret_cast<void**>(&m_voice));
        if (FAILED(hr)) {
            std::cerr << "VoiceCommentary: Failed to create SAPI voice instance.\n";
            m_voice = nullptr;
        }
    }

    m_wordTemplates = {
        "Nice one! WORD is a great find!",
        "You got WORD!",
        "WORD, impressive vocabulary!",
        "WORD, well spotted!",
        "Great catch, WORD!",
        "WORD, that's a good one!",
        "Look at that, WORD!",
        "WORD, nicely done!",
        "Bravo! WORD!",
        "WORD, you're on fire!",
        "Excellent, WORD!",
        "WORD, keep it up!",
        "WORD, what a word!",
        "You found WORD, amazing!",
        "There's WORD, wonderful!"
    };

    m_rareWordTemplates = {
        "Wow, WORD! That's a rare one!",
        "WORD! Impressive, that's uncommon!",
        "WORD, what a find! Very rare!",
        "WORD! Not many would get that!",
        "Outstanding! WORD is a tough one!",
        "WORD! You really know your words!",
        "Incredible, WORD! That's a gem!",
        "WORD, brilliant! A rare discovery!"
    };

    m_puzzleTemplates = {
        "Puzzle complete! You nailed it!",
        "All words found, well done!",
        "Puzzle solved! Fantastic work!",
        "You cleared the whole puzzle!",
        "Every word found, amazing!",
        "Puzzle finished! Brilliant!",
        "That's a wrap! Great job!",
        "All done! You're a natural!",
        "Perfect puzzle clearance!",
        "Solved it! Nothing gets past you!"
    };

    m_sessionTemplates = {
        "Session finished! You're a word master!",
        "What a performance, every puzzle solved!",
        "Session complete! Absolutely phenomenal!",
        "You conquered every puzzle! Incredible!",
        "All puzzles done! You're unstoppable!",
        "Session cleared! Take a bow!",
        "That was brilliant, session complete!",
        "Every puzzle beaten! What a champion!",
        "Full session victory! Outstanding!",
        "You did it! All puzzles mastered!"
    };

    shuffleOrder(m_wordOrder, static_cast<int>(m_wordTemplates.size()));
    shuffleOrder(m_rareWordOrder, static_cast<int>(m_rareWordTemplates.size()));
    shuffleOrder(m_puzzleOrder, static_cast<int>(m_puzzleTemplates.size()));
    shuffleOrder(m_sessionOrder, static_cast<int>(m_sessionTemplates.size()));
}

VoiceCommentary::~VoiceCommentary() {
    if (m_voice) {
        m_voice->Release();
        m_voice = nullptr;
    }
    if (m_comInitialized) {
        CoUninitialize();
    }
}

void VoiceCommentary::toggle() {
    m_enabled = !m_enabled;
    if (!m_enabled && m_voice) {
        m_voice->Speak(L"", SPF_ASYNC | SPF_PURGEBEFORESPEAK, nullptr);
    }
}

void VoiceCommentary::speak(const std::string& text) {
    if (!m_enabled || !m_voice) return;

    int wideLen = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (wideLen <= 0) return;
    std::vector<wchar_t> wideText(wideLen);
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, wideText.data(), wideLen);

    m_voice->Speak(wideText.data(), SPF_ASYNC | SPF_PURGEBEFORESPEAK, nullptr);
}

void VoiceCommentary::shuffleOrder(std::vector<int>& order, int size) {
    order.resize(size);
    for (int i = 0; i < size; ++i) order[i] = i;
    std::shuffle(order.begin(), order.end(), rng());
}

std::string VoiceCommentary::pickTemplate(std::vector<int>& order, const std::vector<std::string>& pool) {
    if (order.empty()) {
        shuffleOrder(order, static_cast<int>(pool.size()));
    }
    int idx = order.back();
    order.pop_back();
    return pool[idx];
}

std::string VoiceCommentary::capitalize(const std::string& word) {
    if (word.empty()) return word;
    std::string result = word;
    result[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(result[0])));
    for (size_t i = 1; i < result.size(); ++i) {
        result[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(result[i])));
    }
    return result;
}

void VoiceCommentary::onWordFound(const std::string& word, int rarity) {
    std::string displayWord = capitalize(word);
    std::string tmpl = (rarity > 1)
        ? pickTemplate(m_rareWordOrder, m_rareWordTemplates)
        : pickTemplate(m_wordOrder, m_wordTemplates);

    std::string text = tmpl;
    size_t pos = text.find("WORD");
    while (pos != std::string::npos) {
        text.replace(pos, 4, displayWord);
        pos = text.find("WORD", pos + displayWord.size());
    }
    speak(text);
}

void VoiceCommentary::onPuzzleSolved() {
    speak(pickTemplate(m_puzzleOrder, m_puzzleTemplates));
}

void VoiceCommentary::onSessionComplete() {
    speak(pickTemplate(m_sessionOrder, m_sessionTemplates));
}
