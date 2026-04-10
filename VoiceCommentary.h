#ifndef VOICE_COMMENTARY_H
#define VOICE_COMMENTARY_H

#include <string>
#include <vector>

struct ISpVoice;

class VoiceCommentary {
public:
    VoiceCommentary();
    ~VoiceCommentary();

    VoiceCommentary(const VoiceCommentary&) = delete;
    VoiceCommentary& operator=(const VoiceCommentary&) = delete;

    bool isEnabled() const { return m_enabled; }
    void toggle();

    void onWordFound(const std::string& word, int rarity);
    void onPuzzleSolved();
    void onSessionComplete();

private:
    void speak(const std::string& text);
    std::string pickTemplate(std::vector<int>& order, const std::vector<std::string>& pool);
    void shuffleOrder(std::vector<int>& order, int size);
    static std::string capitalize(const std::string& word);

    bool m_enabled = true;
    ISpVoice* m_voice = nullptr;
    bool m_comInitialized = false;

    std::vector<std::string> m_wordTemplates;
    std::vector<std::string> m_rareWordTemplates;
    std::vector<std::string> m_puzzleTemplates;
    std::vector<std::string> m_sessionTemplates;

    std::vector<int> m_wordOrder;
    std::vector<int> m_rareWordOrder;
    std::vector<int> m_puzzleOrder;
    std::vector<int> m_sessionOrder;
};

#endif // VOICE_COMMENTARY_H
