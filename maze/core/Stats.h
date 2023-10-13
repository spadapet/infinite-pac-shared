#pragma once

struct Stats
{
    struct HighScore
    {
        DWORD _score;
        DWORD _level;
        FILETIME _time;
        char _name[10];
    };

    bool _cheated;
    DWORD _gamesStarted;
    DWORD _levelsBeaten;
    DWORD _dotsEaten;
    DWORD _powerEaten;
    DWORD _fruitsEaten;
    DWORD _ghostsEaten[4];
    DWORD _ghostDeathCount[4];
    DWORD _tunnelsUsed;
    DWORD _score;
    HighScore _highScores[10];

    // Helper functions

    Stats();
    Stats(const Stats& rhs);

    Stats& operator=(const Stats& rhs);
    Stats& operator+=(const Stats& rhs);

    size_t GetHighScoreSlot(size_t nScore);
    void InsertHighScore(size_t nScore, size_t nLevel, const std::string& szName);

    static void Load();
    static void Save();
    static Stats& Get(std::string_view mazesId);
};
