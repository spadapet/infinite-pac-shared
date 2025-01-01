#pragma once

#include "Core/Actors.h"
#include "Core/PlayingGame.h"
#include "Core/PlayingMaze.h"

class PacApplication;
class IRenderMaze;
class IRenderText;
class ISoundEffects;

class TitleScreen
    : public std::enable_shared_from_this<TitleScreen>
    , public IPlayingGame
    , public IPlayingMazeHost
    , public IPlayingMaze
    , public IPlayingActor
{
public:
    TitleScreen();

    static size_t GetHighScoreDisplayCount();
    static std::string GetMazesID();

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
    enum EOption
    {
        OPT_PAC,
        OPT_MSPAC,
        OPT_PLAYERS,
        OPT_DIFF,
        OPT_SOUND,
        OPT_VIBRATE,
        OPT_FULL_SCREEN,
        OPT_ABOUT,
        OPT_NONE,
    };

    typedef std::function<std::string()> GetTextFunc;

    struct Option
    {
        Option();
        Option(EOption type, ff::point_int pos, GetTextFunc textFunc, int nMazes = -1);
        std::string GetText() const;
        ff::rect_float GetLevelRect() const;
        ff::rect_float GetTargetRect() const;

        EOption _type;
        ff::point_int _selectedPos;
        GetTextFunc _textFunc;
        float _hoverOpacity;
        int _mazes;
    };

    void SelectDefaultOption();
    void UpdateHighScores();
    void CreateOptions();
    bool CreateFrontMaze();
    bool CreateBackMaze();
    void Execute(EOption option, bool pressedLeft);
    Option* HitTestOption(ff::point_float pos);
    static float GetLineHeight();

    void RenderBackMaze(ff::dxgi::draw_base& draw);
    void RenderFrontMaze(ff::dxgi::draw_base& draw);
    void RenderOptions(ff::dxgi::draw_base& draw);
    void RenderFade(ff::dxgi::draw_base& draw);

    std::shared_ptr<IMazes> _mazes;
    std::shared_ptr<IRenderMaze> _render;
    std::shared_ptr<IRenderText> _text;
    std::shared_ptr<IPlayingMaze> _backMaze;
    std::shared_ptr<ISoundEffects> _sounds;
    std::shared_ptr<ff::input_event_provider> _inputRes;
    std::vector<Option> _options;
    std::string _scores;
    size_t _curOption{};
    Stats _stats{};
    float _fade{ 1 };
    bool _fading{ true };
    bool _done{};
};
