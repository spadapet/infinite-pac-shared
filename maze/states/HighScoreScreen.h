#pragma once

#include "Core/Actors.h"
#include "Core/PlayingGame.h"
#include "Core/PlayingMaze.h"

class PacApplication;
class IRenderText;

class HighScoreScreen
    : public std::enable_shared_from_this<HighScoreScreen>
    , public IPlayingGame
    , public IPlayingMazeHost
    , public IPlayingMaze
    , public IPlayingActor
{
public:
    HighScoreScreen(std::string_view mazesId, std::shared_ptr<IPlayer> pPlayer);

    std::string GetName() const;

    // IPlayingGame

    virtual void Advance() override;
    virtual void Render(ff::dxgi::draw_base& draw) override;

    virtual std::shared_ptr<IMazes> GetMazes() override;
    virtual ff::point_int GetSizeInTiles() const override;
    virtual size_t GetHighScore() const override;

    virtual size_t GetPlayers() const override;
    virtual size_t GetCurrentPlayer() const override;
    virtual std::shared_ptr<IPlayer> GetPlayer(size_t nPlayer) override;

    virtual bool IsGameOver() const override;
    virtual bool IsPaused() const override;
    virtual void TogglePaused() override;
    virtual void PausedAdvance() override;

    // IPlayingMazeHost

    virtual void OnStateChanged(GameState oldState, GameState newState) override;
    virtual size_t GetMazePlayer() override;
    virtual bool IsPlayingLevel() override;
    virtual bool IsEffectEnabled(AudioEffect effect) override;
    virtual void OnPacUsingTunnel() override;

    // IPlayingMaze

    //virtual void Advance() override;
    //virtual void Render(ff::I2dRenderer *draw) override;
    virtual void Reset() override;

    virtual GameState GetGameState() const override;
    virtual const Stats& GetStats() const override;
    virtual std::shared_ptr<IMaze> GetMaze() const override;
    virtual std::shared_ptr<IRenderMaze> GetRenderMaze() override;
    virtual const Difficulty& GetDifficulty() const override;

    virtual PacState GetPacState() const override;
    virtual std::shared_ptr<IPlayingActor> GetPac() override;
    virtual CharType GetCharType() const override;
    virtual bool IsPowerPac() const override;

    virtual size_t GetGhostCount() const override;
    virtual ff::point_int GetGhostEyeDir(size_t nGhost) const override;
    virtual GhostState GetGhostState(size_t nGhost) const override;
    virtual std::shared_ptr<IPlayingActor> GetGhost(size_t nGhost) override;

    virtual ff::point_int GetGhostStartPixel() const override;
    virtual ff::point_int GetGhostStartTile() const override;

    virtual FruitState GetFruitState() const override;
    virtual std::shared_ptr<IPlayingActor> GetFruit() override;
    virtual FruitType GetFruitType() const override;
    virtual ff::point_int GetFruitExitTile() const override;

    virtual std::shared_ptr<PointActor> const* GetPointDisplays(size_t& nCount) const override;
    virtual std::shared_ptr<CustomActor> const* GetCustomActors(size_t& nCount) const override;

    // IPlayingActor

    virtual ff::point_int GetTile() const override;
    virtual ff::point_int GetPixel() const override;
    virtual void SetPixel(ff::point_int pixel) override;

    virtual ff::point_int GetDir() const override;
    virtual void SetDir(ff::point_int dir) override;

    virtual ff::point_int GetPressDir() const override;
    virtual void SetPressDir(ff::point_int dir) override;

    virtual bool IsActive() const override;
    virtual void SetActive(bool bActive) override;

private:
    std::shared_ptr<ff::input_event_provider> _inputRes;
    std::shared_ptr<IMazes> _mazes;
    std::shared_ptr<IRenderMaze> _render;
    std::shared_ptr<IRenderText> _text;
    std::shared_ptr<IPlayer> _player;
    std::string _intro;
    std::string _name;
    size_t _counter{};
    std::string _mazesID;
    bool _done{};
    bool _showNewLetter{ true };

    struct Letter
    {
        char _letter[5];
        ff::point_float _pos;
        int _vk;
    };

    Letter* GetLetter(char ch);
    Letter* HitTestLetter(ff::point_float pos);
    ff::point_float GetLetterPos(char ch);

    std::vector<Letter> _letters;
    size_t _letter{};
};
