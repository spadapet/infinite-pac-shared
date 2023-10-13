#pragma once

class IMazes;
class IPlayer;
class IPlayingGameHost;
class IPlayingMaze;
struct Stats;

class IPlayingGame
{
public:
    virtual ~IPlayingGame() = default;

    static std::shared_ptr<IPlayingGame> Create(std::shared_ptr<IMazes> pMazes, size_t nPlayers, IPlayingGameHost* pHost);

    virtual void Advance() = 0;
    virtual void Render(ff::dxgi::draw_base& draw) = 0;

    virtual std::shared_ptr<IMazes> GetMazes() = 0;
    virtual ff::point_int GetSizeInTiles() const = 0;
    virtual size_t GetHighScore() const = 0;

    virtual size_t GetPlayers() const = 0;
    virtual size_t GetCurrentPlayer() const = 0;
    virtual std::shared_ptr<IPlayer> GetPlayer(size_t nPlayer) = 0;

    virtual bool IsGameOver() const = 0;
    virtual bool IsPaused() const = 0;
    virtual void TogglePaused() = 0;
    virtual void PausedAdvance() = 0;
};

class IPlayer
{
public:
    virtual ~IPlayer() = default;

    virtual size_t GetLevel() const = 0;
    virtual void SetLevel(size_t nLevel) = 0;
    virtual std::shared_ptr<IPlayingMaze> GetPlayingMaze() = 0;

    virtual size_t GetScore() const = 0;
    virtual size_t GetLives() const = 0;
    virtual bool IsGameOver() const = 0;
    virtual bool DidCheat() const = 0;
    virtual void AddStats(Stats& stats) const = 0;
};

class IPlayingGameHost
{
public:
    virtual bool IsShowingScoreBar(IPlayingGame* pGame) const = 0;
    virtual bool IsShowingStatusBar(IPlayingGame* pGame) const = 0;
    virtual void OnPlayerGameOver(IPlayingGame* pGame, std::shared_ptr<IPlayer>) = 0;
};
