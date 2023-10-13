#include "pch.h"
#include "Core/Actors.h"
#include "Core/Difficulty.h"

static const Difficulty s_emptyDifficulty
{
    4, // _ghostCount
    80, // _pacSpeed
    0, // _pacSubtract
    0, // _pacAdd
    20, // _elroyDots
    6, // _scaredSeconds
    { 7, 20, 7, 20, 5, 20, 5, 0 }, // _ghostModeSeconds
    { 0, 0, 30, 60 }, // _ghostDotCounter
    4, // _lastDotSeconds
    FRUIT_0 // _fruit
};

const Difficulty& GetEmptyDifficulty()
{
    return s_emptyDifficulty;
}

static std::string_view PROP_GHOSTS("ghosts");
static std::string_view PROP_SPEED("speed");
static std::string_view PROP_SUBTRACT("speedSubtract");
static std::string_view PROP_ADD("speedAdd");
static std::string_view PROP_ELROY("elroyDots");
static std::string_view PROP_SCARED("scaredSeconds");
static std::string_view PROP_LAST_DOT("lastDotSecondsUntilGhost");
static std::string_view PROP_FRUIT("fruit");
static std::string_view PROP_GHOST_MODE("ghostModeSeconds");
static std::string_view PROP_GHOST_DOTS("ghostDots");

static bool GetDifficultyBasics(const ff::dict& dict, std::vector<Difficulty>& diffs)
{
    Difficulty diff = GetEmptyDifficulty();

    diff._ghostCount = dict.get<size_t>(PROP_GHOSTS, diff._ghostCount);
    diff._pacSpeed = dict.get<size_t>(PROP_SPEED, diff._pacSpeed);
    diff._pacSubtract = dict.get<size_t>(PROP_SUBTRACT, diff._pacSubtract);
    diff._pacAdd = dict.get<size_t>(PROP_ADD, diff._pacAdd);
    diff._elroyDots = dict.get<size_t>(PROP_ELROY, diff._elroyDots);
    diff._scaredSeconds = dict.get<size_t>(PROP_SCARED, diff._scaredSeconds);
    diff._lastDotSeconds = dict.get<size_t>(PROP_LAST_DOT, diff._lastDotSeconds);
    diff._fruit = dict.get_enum<FruitType>(PROP_FRUIT, diff._fruit);

    assert_ret_val(diff._fruit >= FRUIT_0 && diff._fruit <= FRUIT_NONE, false);
    diffs.push_back(diff);

    return true;
}

static bool GetDifficultyVectors(const ff::dict& dict, std::vector<Difficulty>& diffs)
{
    std::vector<int> ghosts = dict.get<std::vector<int>>(PROP_GHOSTS);
    std::vector<int> speeds = dict.get<std::vector<int>>(PROP_SPEED);
    std::vector<int> subs = dict.get<std::vector<int>>(PROP_SUBTRACT);
    std::vector<int> adds = dict.get<std::vector<int>>(PROP_ADD);
    std::vector<int> elroys = dict.get<std::vector<int>>(PROP_ELROY);
    std::vector<int> scareds = dict.get<std::vector<int>>(PROP_SCARED);
    std::vector<int> lastDots = dict.get<std::vector<int>>(PROP_LAST_DOT);
    std::vector<int> fruits = dict.get<std::vector<int>>(PROP_FRUIT);

    const size_t size = ghosts.size();
    assert_ret_val(size, false);
    assert_ret_val(speeds.size() == size, false);
    assert_ret_val(subs.size() == size, false);
    assert_ret_val(adds.size() == size, false);
    assert_ret_val(elroys.size() == size, false);
    assert_ret_val(scareds.size() == size, false);
    assert_ret_val(lastDots.size() == size, false);
    assert_ret_val(fruits.size() == size, false);

    for (size_t i = 0; i < size; i++)
    {
        Difficulty diff = GetEmptyDifficulty();

        diff._ghostCount = (size_t)ghosts[i];
        diff._pacSpeed = (size_t)speeds[i];
        diff._pacSubtract = (size_t)subs[i];
        diff._pacAdd = (size_t)adds[i];
        diff._elroyDots = (size_t)elroys[i];
        diff._scaredSeconds = (size_t)scareds[i];
        diff._lastDotSeconds = (size_t)lastDots[i];
        diff._fruit = (FruitType)fruits[i];

        diffs.push_back(diff);
    }

    return !diffs.empty();
}

bool CreateDifficultiesFromDict(const ff::dict& dict, std::vector<Difficulty>& diffs)
{
    diffs.clear();

    if (dict.get(PROP_SPEED)->is_type<std::vector<ff::value_ptr>>())
    {
        assert_ret_val(GetDifficultyVectors(dict, diffs), false);
    }
    else
    {
        assert_ret_val(GetDifficultyBasics(dict, diffs), false);
    }

    ff::value_ptr modeValue = dict.get(PROP_GHOST_MODE);
    if (modeValue)
    {
        std::vector<int> vectorValue = modeValue->convert_or_default<std::vector<int>>()->get<std::vector<int>>();

        for (size_t i = 0; i < _countof(Difficulty::_ghostModeSeconds) && i < vectorValue.size(); i++)
        {
            for (Difficulty& diff : diffs)
            {
                diff._ghostModeSeconds[i] = (size_t)vectorValue[i];
            }
        }
    }

    ff::value_ptr dotsValue = dict.get(PROP_GHOST_DOTS);
    if (dotsValue)
    {
        std::vector<int> vectorValue = dotsValue->convert_or_default<std::vector<int>>()->get<std::vector<int>>();

        for (size_t i = 0; i < _countof(Difficulty::_ghostDotCounter) && i < vectorValue.size(); i++)
        {
            for (Difficulty& diff : diffs)
            {
                diff._ghostDotCounter[i] = (size_t)vectorValue[i];
            }
        }
    }

    return !diffs.empty();
}

Difficulty& Difficulty::operator=(const Difficulty& rhs)
{
    if (this != &rhs)
    {
        CopyMemory(this, &rhs, sizeof(*this));
    }

    return *this;
}

size_t Difficulty::GetPacSpeed(bool bPower) const
{
    size_t speed = _pacSpeed - _pacSubtract + _pacAdd;

    if (bPower)
    {
        size_t speed2 = (_pacSpeed + 100) / 2;
        speed = std::max(speed, speed2);
    }

    return speed;
}

size_t Difficulty::GetFruitSpeed() const
{
    return _pacSpeed / 2;
}

size_t Difficulty::GetGhostSpeed(
        MoveState move,
        HouseState house,
        bool bTunnel,
        bool bElroy,
        size_t nDotsLeft) const
{
    if (move == MOVE_EYES)
    {
        return std::max<size_t>(150, _pacSpeed - 5);
    }
    else if (bTunnel || house != HOUSE_OUTSIDE)
    {
        return _pacSpeed / 2;
    }
    else if (move == MOVE_SCARED)
    {
        return _pacSpeed / 2 + 10;
    }
    if (bElroy && nDotsLeft <= _elroyDots / 2)
    {
        return _pacSpeed + 5;
    }
    else if (bElroy && nDotsLeft <= _elroyDots)
    {
        return _pacSpeed;
    }
    else
    {
        return _pacSpeed - 5;
    }
}

size_t Difficulty::GetScaredFrames() const
{
    return _scaredSeconds * ff::constants::advances_per_second<size_t>();
}

size_t Difficulty::GetEatenFrames() const
{
    return ff::constants::advances_per_second<size_t>() * 9 / 10;
}

size_t Difficulty::GetGhostDotCounter(size_t nGhost) const
{
    assert_ret_val(nGhost >= 0 && nGhost < _countof(_ghostDotCounter), 0);

    return _ghostDotCounter[nGhost];
}

size_t Difficulty::GetGlobalDotCounter(size_t nIndex) const
{
    switch (nIndex)
    {
        default:
        case 0: return 0;
        case 1: return 7;
        case 2: return 17;
        case 3: return 32;
    }
}

size_t Difficulty::GetLastDotFrames() const
{
    return _lastDotSeconds * ff::constants::advances_per_second<size_t>();
}

size_t Difficulty::GetFruitFrames(CharType type) const
{
    int seconds = IsFruitMoving(type) ? 14 : 9;
    return seconds * ff::constants::advances_per_second<size_t>() + (rand() % ff::constants::advances_per_second<size_t>());
}

FruitType Difficulty::GetFruit() const
{
    return _fruit;
}

bool Difficulty::IsFruitMoving(CharType type) const
{
    return true;
}

bool Difficulty::HasRandomGhostMovement(CharType type) const
{
    return type == CHAR_MS;
}

void Difficulty::GetGhostModeFrames(size_t nIndex, size_t& nScatter, size_t& nChase) const
{
    size_t nMinScatter = 1 * ff::constants::advances_per_second<size_t>();
    size_t nMaxChase = 5 * 60 * ff::constants::advances_per_second<size_t>();

    if (nIndex >= 0 && nIndex < _countof(_ghostModeSeconds) / 2)
    {
        nScatter = _ghostModeSeconds[nIndex * 2] * ff::constants::advances_per_second<size_t>();
        nChase = _ghostModeSeconds[nIndex * 2 + 1] * ff::constants::advances_per_second<size_t>();

        nScatter = nScatter ? std::max<size_t>(nMinScatter, nScatter) : nMinScatter;
        nChase = nChase ? std::min<size_t>(nMaxChase, nChase) : nMaxChase;
    }
    else
    {
        nScatter = nMinScatter;
        nChase = nMaxChase;
    }
}

void Difficulty::GetFruitDotCount(size_t nTotalDots, size_t& nFruit1, size_t& nFruit2) const
{
    nFruit1 = nTotalDots * 143 / 200;
    nFruit2 = nTotalDots * 61 / 200;
}

size_t Difficulty::GetDotPoints() const
{
    return 10;
}

size_t Difficulty::GetPowerPoints() const
{
    return 50;
}

size_t Difficulty::GetGhostPoints(size_t nGhost) const
{
    switch (nGhost)
    {
        default:
        case 0: return 200;
        case 1: return 400;
        case 2: return 800;
        case 3: return 1600;
    }
}

size_t Difficulty::GetFruitPoints(FruitType fruit) const
{
    switch (fruit)
    {
        case FRUIT_0: return 100;
        case FRUIT_1: return 300;
        case FRUIT_2: return 500;
        case FRUIT_3: return 700;
        case FRUIT_4: return 1000;
        case FRUIT_5: return 1500;
        case FRUIT_6: return 2000;
        case FRUIT_7: return 2500;
        case FRUIT_8: return 3000;
        case FRUIT_9: return 3500;
        case FRUIT_10: return 4000;
        case FRUIT_11: return 5000;
        default: return 0;
    }
}
