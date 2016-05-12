#pragma once

namespace ff
{
	class I2dRenderer;
}

class IMazes;
class IPlayer;
class IPlayingGameHost;
class IPlayingMaze;
struct Stats;

class __declspec(uuid("267c6af0-9ff2-4e77-afa1-9bd2df47e767")) __declspec(novtable)
	IPlayingGame : public IUnknown
{
public:
	static bool Create(
		IMazes* pMazes,
		size_t nPlayers,
		IPlayingGameHost* pHost,
		IPlayingGame** ppGame);

	virtual void Advance() = 0;
	virtual void Render(ff::I2dRenderer *pRenderer) = 0;

	virtual IMazes* GetMazes() = 0;
	virtual ff::PointInt GetSizeInTiles() const = 0;
	virtual size_t GetHighScore() const = 0;

	virtual size_t GetPlayers() const = 0;
	virtual size_t GetCurrentPlayer() const = 0;
	virtual IPlayer* GetPlayer(size_t nPlayer) = 0;

	virtual bool IsGameOver() const = 0;
	virtual bool IsPaused() const = 0;
	virtual void TogglePaused() = 0;
	virtual void PausedAdvance() = 0;
};

class __declspec(uuid("2a04e780-e51f-4f22-9140-9dbf46280b79")) __declspec(novtable)
	IPlayer : public IUnknown
{
public:
	virtual size_t GetLevel() const = 0;
	virtual void SetLevel(size_t nLevel) = 0;
	virtual IPlayingMaze* GetPlayingMaze() = 0;

	virtual size_t GetScore() const = 0;
	virtual size_t GetLives() const = 0;
	virtual bool IsGameOver() const = 0;
	virtual bool DidCheat() const = 0;
	virtual void AddStats(Stats &stats) const = 0;
};

class IPlayingGameHost
{
public:
	virtual bool IsShowingScoreBar(IPlayingGame *pGame) const = 0;
	virtual bool IsShowingStatusBar(IPlayingGame *pGame) const = 0;
	virtual void OnPlayerGameOver(IPlayingGame *pGame, IPlayer *pPlayer) = 0;
};
