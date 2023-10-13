#pragma once
#include "Core/Difficulty.h"
#include "Core/Stats.h"

class IMaze;
class IRenderMaze;
class IPlayingActor;
class IPlayingMazeHost;
class CustomActor;
class PointActor;
enum AudioEffect;
enum FruitType;

enum PacState
{
    PAC_INVALID,
    PAC_NORMAL,
    PAC_DYING,
    PAC_DEAD,
};

enum GhostState
{
    GHOST_INVALID,
    GHOST_CHASE,
    GHOST_SCATTER,
    GHOST_SCARED,
    GHOST_SCARED_FLASH,
    GHOST_EYES,
};

enum FruitState
{
    FRUIT_INVALID,
    FRUIT_NORMAL,
    FRUIT_EXITING,
};

enum GameState
{
    GS_BEFORE_TIME,
    GS_PLAYER_READY,
    GS_READY,
    GS_PLAYING,
    GS_CAUGHT,
    GS_DYING,
    GS_DEAD,
    GS_DIED,
    GS_WINNING,
    GS_WON,
};

class IPlayingMaze
{
public:
    virtual ~IPlayingMaze() = default;

    static std::shared_ptr<IPlayingMaze> Create(std::shared_ptr<IMaze> pMaze, const Difficulty& difficulty, IPlayingMazeHost* pHost);

    virtual void Advance() = 0;
    virtual void Render(ff::dxgi::draw_base& draw) = 0;
    virtual void Reset() = 0;

    virtual GameState GetGameState() const = 0;
    virtual const Stats& GetStats() const = 0;
    virtual std::shared_ptr<IMaze> GetMaze() const = 0;
    virtual std::shared_ptr<IRenderMaze> GetRenderMaze() = 0;
    virtual const Difficulty& GetDifficulty() const = 0;

    virtual PacState GetPacState() const = 0;
    virtual std::shared_ptr<IPlayingActor> GetPac() = 0;
    virtual CharType GetCharType() const = 0;
    virtual bool IsPowerPac() const = 0;

    virtual size_t GetGhostCount() const = 0;
    virtual ff::point_int GetGhostEyeDir(size_t nGhost) const = 0;
    virtual GhostState GetGhostState(size_t nGhost) const = 0;
    virtual std::shared_ptr<IPlayingActor> GetGhost(size_t nGhost) = 0;

    virtual ff::point_int GetGhostStartPixel() const = 0;
    virtual ff::point_int GetGhostStartTile() const = 0;

    virtual FruitState GetFruitState() const = 0;
    virtual std::shared_ptr<IPlayingActor> GetFruit() = 0;
    virtual FruitType GetFruitType() const = 0;
    virtual ff::point_int GetFruitExitTile() const = 0;

    virtual std::shared_ptr<PointActor> const* GetPointDisplays(size_t& nCount) const = 0;
    virtual std::shared_ptr<CustomActor> const* GetCustomActors(size_t& nCount) const = 0;
};

class IPlayingMazeHost
{
public:
    virtual void OnStateChanged(GameState oldState, GameState newState) = 0;
    virtual size_t GetMazePlayer() = 0;
    virtual bool IsPlayingLevel() = 0;
    virtual bool IsEffectEnabled(AudioEffect effect) = 0;
    virtual void OnPacUsingTunnel() = 0;
};
