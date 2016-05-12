#pragma once

class IMaze;
struct Difficulty;
struct Stats;

class __declspec(uuid("7fe7f3fb-6f7d-48d1-8dec-dacc8448d5ad")) __declspec(novtable)
	IMazes : public IUnknown
{
public:
	static bool Create(IMazes **ppMazes);

	virtual ff::StringRef GetID() const = 0;
	virtual void SetID(ff::StringRef id) = 0;

	virtual size_t GetMazeCount() const = 0;
	virtual IMaze* GetMaze(size_t nMaze) const = 0;
	virtual size_t FindMaze(IMaze *pMaze) const = 0;

	virtual void AddMaze (size_t nMaze, IMaze *pMaze) = 0;
	virtual void RemoveMaze(size_t nMaze, IMaze **ppMaze) = 0;

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

bool CreateMazesFromId(ff::StringRef id, IMazes **obj);
bool CreateMazesFromResource(ff::String name, IMazes **obj);
