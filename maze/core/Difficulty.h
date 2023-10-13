#pragma once

enum MoveState;
enum HouseState;

enum CharType
{
    CHAR_MR,
    CHAR_MS,

    CHAR_COUNT,
    CHAR_DEFAULT = CHAR_MR,
};

enum FruitType
{
    FRUIT_0,
    FRUIT_1,
    FRUIT_2,
    FRUIT_3,
    FRUIT_4,
    FRUIT_5,
    FRUIT_6,
    FRUIT_7,
    FRUIT_8,
    FRUIT_9,
    FRUIT_10,
    FRUIT_11,

    FRUIT_RANDOM,
    FRUIT_NONE,
};

struct Difficulty
{
    size_t _ghostCount;
    size_t _pacSpeed;
    size_t _pacSubtract;
    size_t _pacAdd;
    size_t _elroyDots;
    size_t _scaredSeconds;
    size_t _ghostModeSeconds[8]; // scatter, chase, scatter, ...
    size_t _ghostDotCounter[4]; // initial dots eaten before leaving house
    size_t _lastDotSeconds; // seconds after eating last dot before a ghost is released
    FruitType _fruit;

    // Helper functions

    Difficulty& operator=(const Difficulty& rhs);

    size_t GetPacSpeed(bool bPower) const;
    size_t GetFruitSpeed() const;
    size_t GetGhostSpeed(MoveState move, HouseState house, bool bTunnel, bool bElroy, size_t nDotsLeft) const;
    size_t GetScaredFrames() const;
    size_t GetEatenFrames() const;
    size_t GetGhostDotCounter(size_t nGhost) const;
    size_t GetGlobalDotCounter(size_t nIndex) const;
    size_t GetLastDotFrames() const;
    size_t GetFruitFrames(CharType type) const;
    FruitType GetFruit() const;
    bool IsFruitMoving(CharType type) const;
    bool HasRandomGhostMovement(CharType type) const;

    void GetGhostModeFrames(size_t nIndex, size_t& nScatter, size_t& nChase) const;
    void GetFruitDotCount(size_t nTotalDots, size_t& nFruit1, size_t& nFruit2) const;

    size_t GetDotPoints() const;
    size_t GetPowerPoints() const;
    size_t GetGhostPoints(size_t nGhost) const;
    size_t GetFruitPoints(FruitType fruit) const;
};

const Difficulty& GetEmptyDifficulty();
bool CreateDifficultiesFromDict(const ff::dict& dict, std::vector<Difficulty>& diffs);
