#pragma once

class IMaze;
struct Difficulty;
struct Stats;

class IMazes
{
public:
    virtual ~IMazes() = default;

	static std::shared_ptr<IMazes> Create();

	virtual std::string_view GetID() const = 0;
	virtual void SetID(std::string_view id) = 0;

	virtual size_t GetMazeCount() const = 0;
	virtual std::shared_ptr<IMaze> GetMaze(size_t nMaze) const = 0;
	virtual size_t FindMaze(IMaze *pMaze) const = 0;

	virtual void AddMaze (size_t nMaze, std::shared_ptr<IMaze> pMaze) = 0;
	virtual std::shared_ptr<IMaze> RemoveMaze(size_t nMaze) = 0;

	virtual size_t GetDifficultyCount() const = 0;
	virtual const Difficulty& GetDifficulty (size_t nDiff) const = 0;
	virtual void AddDifficulty (size_t nDiff, const Difficulty &diff) = 0;
	virtual void RemoveDifficulty(size_t nDiff) = 0;

	virtual size_t GetStartingLives() const = 0;
	virtual void SetStartingLives(size_t nLives) = 0;

	virtual size_t GetFreeLifeScore() const = 0;
	virtual size_t GetFreeLifeRepeat() const = 0;
	virtual size_t GetMaxFreeLives() const = 0;
	virtual void SetFreeLifeScore(size_t nScore, size_t nRepeat, size_t nMaxFree) = 0;

	virtual const Stats& GetStats() const = 0;
	virtual void SetStats(const Stats &stats) = 0;
};

std::shared_ptr<IMazes> CreateMazesFromId(std::string_view id);
