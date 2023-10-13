#include "pch.h"
#include "Core/Stats.h"

// STATIC_DATA(object)
static std::unordered_map<std::string, Stats> s_stats;

Stats::Stats()
{
    ZeroMemory(this, sizeof(*this));
}

Stats::Stats(const Stats& rhs)
{
    *this = rhs;
}

Stats& Stats::operator=(const Stats& rhs)
{
    if (this != &rhs)
    {
        CopyMemory(this, &rhs, sizeof(*this));
    }

    return *this;
}

Stats& Stats::operator+=(const Stats& rhs)
{
    _cheated = _cheated || rhs._cheated;

    _gamesStarted += rhs._gamesStarted;
    _levelsBeaten += rhs._levelsBeaten;
    _dotsEaten += rhs._dotsEaten;
    _powerEaten += rhs._powerEaten;
    _fruitsEaten += rhs._fruitsEaten;
    _tunnelsUsed += rhs._tunnelsUsed;
    _score += rhs._score;

    for (size_t i = 0; i < _countof(_ghostsEaten); i++)
    {
        _ghostsEaten[i] += rhs._ghostsEaten[i];
        _ghostDeathCount[i] += rhs._ghostDeathCount[i];
    }

    return *this;
}

size_t Stats::GetHighScoreSlot(size_t nScore)
{
    for (size_t i = 0; i < _countof(_highScores); i++)
    {
        if (nScore > _highScores[i]._score)
        {
            return i;
        }
    }

    return ff::constants::invalid_unsigned<size_t>();
}

void Stats::InsertHighScore(size_t nScore, size_t nLevel, const std::string& szName)
{
    size_t nSlot = GetHighScoreSlot(nScore);

    if (nSlot != ff::constants::invalid_unsigned<size_t>())
    {
        // Shift down existing scores

        for (size_t i = _countof(_highScores) - 1; i > nSlot; i--)
        {
            CopyMemory(&_highScores[i], &_highScores[i - 1], sizeof(_highScores[i]));
        }

        // Set the new score

        HighScore hs{};


        strncpy_s(hs._name, szName.c_str(), _TRUNCATE);
        hs._level = (DWORD)nLevel;
        hs._score = (DWORD)nScore;

        SYSTEMTIME st;
        GetSystemTime(&st);
        SystemTimeToFileTime(&st, &hs._time);

        _highScores[nSlot] = hs;
    }
}

static std::string_view s_scores = "Scores";

// static
void Stats::Load()
{
    ff::dict dict = ff::settings(s_scores);

    for (std::string_view key : dict.child_names())
    {
        std::shared_ptr<ff::data_base> data = dict.get<ff::data_base>(key);

        Stats stats;
        if (data && data->size() <= sizeof(stats))
        {
            CopyMemory(&stats, data->data(), data->size());
            s_stats.insert_or_assign(std::string(key), stats);
        }
    }
}

// static
void Stats::Save()
{
    ff::dict dict = ff::settings(s_scores);

    for (auto iter : s_stats)
    {
        const std::string& key = iter.first;
        const Stats& stats = iter.second;

        std::vector<uint8_t> bytes;
        bytes.insert(bytes.end(), (const uint8_t*)&stats, (const uint8_t*)&stats + sizeof(stats));
        auto data_vector = std::make_shared<ff::data_vector>(std::move(bytes));
        dict.set<ff::data_base>(key, data_vector, ff::saved_data_type::none);
    }

    ff::settings(s_scores, dict);
}

// static
Stats& Stats::Get(std::string_view mazesId)
{
    std::string key(mazesId);
    auto iter = s_stats.find(key);

    if (iter == s_stats.end())
    {
        iter = s_stats.insert_or_assign(key, Stats()).first;
    }

    return iter->second;
}
