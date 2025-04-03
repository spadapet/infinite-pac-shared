#pragma once

#include "Core/PlayingGame.h"

class IPlayingActor;

class IPacApplicationHost
{
public:
    virtual void ShowAboutDialog() = 0;
    virtual bool IsShowingPopup() const = 0;
    virtual void Quit() = 0;
};

class PacApplication : public IPlayingGameHost
{
public:
    PacApplication(IPacApplicationHost& host);
    ~PacApplication();

    static PacApplication* Get();
    IPacApplicationHost& GetHost() const;
    ff::dict& GetOptions();
    ff::rect_float GetRenderRect() const;
    ff::rect_float GetLevelRect() const;

    void PauseGame();
    bool IsPaused() const;

    static std::string_view OPTION_PAC_DIFF;
    static std::string_view OPTION_PAC_MAZES;
    static std::string_view OPTION_PAC_PLAYERS;
    static std::string_view OPTION_SOUND_ON;
    static std::string_view OPTION_VIBRATE_ON;
    static std::string_view OPTION_FULL_SCREEN;

    static const int DEFAULT_PAC_DIFF = 1;
    static const int DEFAULT_PAC_MAZES = 0;
    static const int DEFAULT_PAC_PLAYERS = 1;
    static const bool DEFAULT_SOUND_ON = true;
    static const bool DEFAULT_VIBRATE_ON = true;
    static const bool DEFAULT_FULL_SCREEN = false;

    // State
    void Update();
    void RenderOffscreen(const ff::render_params& params);
    void RenderScreen(const ff::render_params& params);
    void SaveState();
    void LoadState();

    // IPlayingGameHost
    bool IsShowingScoreBar(IPlayingGame* pGame) const;
    bool IsShowingStatusBar(IPlayingGame* pGame) const;
    void OnPlayerGameOver(IPlayingGame* pGame, std::shared_ptr<IPlayer> pPlayer);

private:
    enum EAppState
    {
        APP_LOADING,
        APP_TITLE,
        APP_PLAYING_GAME,
        APP_HIGH_SCORE,
    };

    enum class EPlayButton
    {
        HOME,
        PLAY,
        PAUSE,
    };

    void HandleInputEvents();
    void HandlePressing(ff::input_event_provider* inputMap);
    void HandleButtons(std::vector<ff::input_event>& events);
    ff::point_int HandleTouchPress(IPlayingActor* pac);
    void RenderGame(ff::dxgi::command_context_base& context, ff::dxgi::target_base& target, ff::dxgi::depth_base& depth, IPlayingGame* pGame);
    void RenderPacPressing(ff::dxgi::draw_base& draw);
    void RenderDebugGrid(ff::dxgi::draw_base& draw, ff::point_int tiles);
    void RenderButtons(ff::dxgi::draw_base& draw);
    ff::rect_float GetButtonRect(EPlayButton button);
    void SetState(EAppState state);
    std::shared_ptr<IPlayingActor> GetCurrentPac() const;

    IPacApplicationHost& _host;
    EAppState _state{};
    ff::dict _options;
    std::shared_ptr<IPlayingGame> _game;
    std::shared_ptr<IPlayingGame> _pushedGame;
    std::shared_ptr<ff::input_event_provider> _inputRes;

    // Rendering
    ff::window_size _targetSize{};
    ff::rect_float _renderRect{};
    ff::rect_float _levelRect{};
    float _fade{};
    float _destFade{};

    // Touch controls
    bool _touching{};
    bool _pressDirFromTouch{};
    double _touchLen{};
    ff::pointer_touch_info _touchInfo{};
    ff::point_double _touchStart{};
    ff::point_double _touchOffset{};
    ff::point_int _touchStartPacDir{};
    ff::auto_resource<ff::sprite_base> _touchArrowSprite;
    ff::render_targets _targets;
};
