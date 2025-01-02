#include "pch.h"
#include "Core/GlobalResources.h"
#include "Core/Helpers.h"
#include "Core/Mazes.h"
#include "Core/Stats.h"
#include "States/HighScoreScreen.h"
#include "States/PacApplication.h"
#include "States/TitleScreen.h"

// static
PacApplication* s_pacApp = nullptr;
std::string_view PacApplication::OPTION_PAC_DIFF("OPTION_PAC_DIFFICULTY");
std::string_view PacApplication::OPTION_PAC_MAZES("OPTION_PAC_MAZES");
std::string_view PacApplication::OPTION_PAC_PLAYERS("OPTION_PAC_PLAYERS");
std::string_view PacApplication::OPTION_SOUND_ON("OPTION_SOUND_ON");
std::string_view PacApplication::OPTION_VIBRATE_ON("OPTION_VIBRATE_ON");
std::string_view PacApplication::OPTION_FULL_SCREEN("OPTION_FULL_SCREEN");

static const double TOUCH_DEAD_ZONE = 20;

PacApplication::PacApplication(IPacApplicationHost& host)
    : _inputRes(GetGlobalInputMapping())
    , _host(host)
{
    _touchArrowSprite = "char-sprites.move-arrow";

    assert(!s_pacApp);
    s_pacApp = this;
}

PacApplication::~PacApplication()
{
    assert(s_pacApp == this);
    s_pacApp = nullptr;
}

PacApplication* PacApplication::Get()
{
    return s_pacApp;
}

IPacApplicationHost& PacApplication::GetHost() const
{
    return _host;
}

ff::dict& PacApplication::GetOptions()
{
    return _options;
}

ff::rect_float PacApplication::GetRenderRect() const
{
    return _renderRect;
}

ff::rect_float PacApplication::GetLevelRect() const
{
    return _levelRect;
}

void PacApplication::SetInputEvent(size_t id)
{
    ::InterlockedExchange(&_pendingEvent, id);
}

void PacApplication::Advance()
{
    check_ret(!_host.IsShowingPopup());
    bool gamePaused = false;

    switch (_state)
    {
        case APP_LOADING:
            SetState(APP_TITLE);
            break;

        case APP_TITLE:
            {
                std::shared_ptr<TitleScreen> pTitle = std::dynamic_pointer_cast<TitleScreen>(_game);
                if (_game && _game->IsGameOver() && pTitle)
                {
                    // Start playing the selected game
                    auto keepAliveGame = _game;
                    _game = nullptr;
                    SetState(APP_PLAYING_GAME);
                }
            }
            break;

        case APP_PLAYING_GAME:
            if (_game && _game->IsGameOver())
            {
                SetState(APP_TITLE);
            }
            break;

        case APP_HIGH_SCORE:
            if (_fade != _destFade)
            {
                if (_fade < _destFade)
                {
                    _fade = std::min(_destFade, _fade + 0.015625f);
                }
                else if (_fade > _destFade)
                {
                    _fade = std::max(_destFade, _fade - 0.015625f);
                }
            }

            if (_game && _game->IsGameOver())
            {
                if (_fade == 0)
                {
                    // Back to the game
                    _game = _pushedGame;
                    _pushedGame = nullptr;
                    SetState(APP_PLAYING_GAME);

                    ff::save_settings();
                }
                else
                {
                    // Fade back to the game
                    _destFade = 0;
                }
            }
            break;

        default:
            assert_msg(false, "Unknown game state");
            break;
    }

    HandleInputEvents();

    if (_game && !_game->IsPaused())
    {
        _game->Advance();
    }
}

void PacApplication::Render(ff::dxgi::command_context_base& context, ff::render_targets& targets)
{
    check_ret(!_host.IsShowingPopup());

    ff::dxgi::depth_base& depth = targets.depth(context);
    ff::dxgi::target_base& target = targets.target(context);

    if (_pushedGame)
    {
        RenderGame(context, target, depth, _pushedGame.get());
        depth.clear(context, 0, 0);

        ff::rect_float rect = target.size().logical_pixel_rect<float>();
        ff::dxgi::draw_ptr draw = ff::dxgi::global_draw_device().begin_draw(context, target, &depth);
        if (_fade < 1 && draw)
        {
            DirectX::XMFLOAT4 colorFade(0, 0, 0, _fade);

            draw->draw_filled_rectangle(ff::rect_float((float)rect.left, (float)rect.top, (float)rect.right, (float)rect.bottom), colorFade);
            draw.reset();
            depth.clear(context, 0, 0);
        }
    }

    if (!_pushedGame || _fade == _destFade)
    {
        RenderGame(context, target, depth, _game.get());
    }
}

static std::string_view s_state = "PacApplication";

void PacApplication::SaveState()
{
    Stats::Save();
    ff::settings(s_state, _options);
}

void PacApplication::LoadState()
{
    _options = ff::settings(s_state);
    Stats::Load();
}

bool PacApplication::IsShowingScoreBar(IPlayingGame* pGame) const
{
    return true;
}

bool PacApplication::IsShowingStatusBar(IPlayingGame* pGame) const
{
    return true;
}

void PacApplication::OnPlayerGameOver(IPlayingGame* pGame, std::shared_ptr<IPlayer> pPlayer)
{
    assert_ret(pPlayer);

    if (_state == APP_PLAYING_GAME && pGame->GetMazes() && !pPlayer->DidCheat())
    {
        std::string_view mazesID = pGame->GetMazes()->GetID();
        Stats& stats = Stats::Get(mazesID);

        pPlayer->AddStats(stats);

        if (stats.GetHighScoreSlot(pPlayer->GetScore()) != ff::constants::invalid_unsigned<size_t>())
        {
            std::shared_ptr<HighScoreScreen> pGame = std::make_shared<HighScoreScreen>(mazesID, pPlayer);

            _pushedGame = _game;
            _game = pGame;
            _destFade = 0.875f;

            SetState(APP_HIGH_SCORE);
        }
    }
}

void PacApplication::HandleInputEvents()
{
    _inputRes->advance();

    bool unpause = false;
    std::vector<ff::input_event> events = _inputRes->events();

    size_t pendingEvent = ::InterlockedExchange(&_pendingEvent, 0);
    if (pendingEvent)
    {
        events.push_back(ff::input_event{ pendingEvent, 1 });
        events.push_back(ff::input_event{ pendingEvent, 0 });
    }

    for (const ff::input_event& ie : events)
    {
        if (ie.started())
        {
            if (ie.event_id == GetEventPause() || ie.event_id == GetEventStart())
            {
                if (_game && !_game->IsPaused())
                {
                    PauseGame();
                }
                else
                {
                    unpause = true;
                }
            }
            else if (ie.event_id == GetEventPauseAdvance())
            {
                if (ff::constants::debug_build && _game && _game->IsPaused())
                {
                    _game->PausedAdvance();
                }
            }
            else if (ie.event_id == GetEventCancel() && _state == APP_PLAYING_GAME)
            {
                if (_game && _game->IsPaused())
                {
                    unpause = true;
                }
                else
                {
                    SetState(APP_TITLE);
                }
            }
            else if (ie.event_id == GetEventHome() && _state == APP_PLAYING_GAME)
            {
                SetState(APP_TITLE);
            }
        }
    }

    if (unpause && _game && _game->IsPaused())
    {
        _game->TogglePaused();

        if (!_game->IsPaused())
        {
            ff::audio::resume_effects();
        }
    }

    HandlePressing(_inputRes.get());
}

void PacApplication::HandlePressing(ff::input_event_provider* inputMap)
{
    ff::point_int pressDir(0, 0);

    if (inputMap->digital_value(GetEventUp()))
    {
        pressDir.y = -1;
    }
    else if (inputMap->digital_value(GetEventDown()))
    {
        pressDir.y = 1;
    }

    if (inputMap->digital_value(GetEventLeft()))
    {
        pressDir.x = -1;
    }
    else if (inputMap->digital_value(GetEventRight()))
    {
        pressDir.x = 1;
    }

    if (_game && _game->GetPlayers())
    {
        // Tell the game about what directions the user is pressing

        std::shared_ptr<IPlayingActor> pac = GetCurrentPac();
        if (pac && !pressDir)
        {
            pressDir = HandleTouchPress(pac.get());

            if (_touching)
            {
                pac->SetPressDir(pressDir);
                _pressDirFromTouch = true;
            }
            else if (_pressDirFromTouch)
            {
                ff::point_int dir = pac->GetDir();
                pressDir = pac->GetPressDir();

                if (dir.x == pressDir.x)
                {
                    pressDir.x = 0;
                }

                if (dir.y == pressDir.y)
                {
                    pressDir.y = 0;
                }

                pac->SetPressDir(pressDir);
            }
            else
            {
                pac->SetPressDir(pressDir);
                _pressDirFromTouch = false;
            }
        }
        else
        {
            pac->SetPressDir(pressDir);
            _pressDirFromTouch = false;
        }
    }
}

ff::point_int PacApplication::HandleTouchPress(IPlayingActor* pac)
{
    ff::point_int pressDir(0, 0);
    ff::pointer_device& pointer = ff::input::pointer();

    if (pointer.touch_info_count())
    {
        const double scale = ff::app_window().dpi_scale();

        _touching = true;
        _touchInfo = pointer.touch_info(0);
        _touchOffset = _touchInfo.pos - _touchInfo.start_pos;
        _touchLen = std::sqrt(_touchOffset.length_squared()) * scale;

        if (_touchLen >= TOUCH_DEAD_ZONE)
        {
            double angle = std::atan2(_touchOffset.y, _touchOffset.x);
            int slice = (int)(angle * 8.0 / std::numbers::pi_v<double>);

            switch (slice)
            {
                default: debug_fail_msg(ff::string::concat("Bad slice:", slice, ", angle:", angle, ", len:", _touchLen).c_str()); break;
                case  0:          pressDir = ff::point_int(1, 0); break;
                case  1: case  2: pressDir = ff::point_int(1, 1); break;
                case  3: case  4: pressDir = ff::point_int(0, 1); break;
                case  5: case  6: pressDir = ff::point_int(-1, 1); break;
                case  7: case -7: pressDir = ff::point_int(-1, 0); break;
                case  8: case -8: pressDir = ff::point_int(-1, 0); break;
                case -6: case -5: pressDir = ff::point_int(-1, -1); break;
                case -4: case -3: pressDir = ff::point_int(0, -1); break;
                case -2: case -1: pressDir = ff::point_int(1, -1); break;
            }
        }
    }
    else
    {
        _touching = false;
    }

    return pressDir;
}

void PacApplication::PauseGame()
{
    if (_game && !_game->IsPaused())
    {
        _game->TogglePaused();

        if (_game->IsPaused())
        {
            ff::audio::pause_effects();
        }
    }
}

bool PacApplication::IsPaused() const
{
    return _game && _game->IsPaused();
}

void PacApplication::RenderGame(ff::dxgi::command_context_base& context, ff::dxgi::target_base& target, ff::dxgi::depth_base& depth, IPlayingGame* pGame)
{
    check_ret(pGame);

    ff::window_size target_size = target.size();
    ff::rect_int renderRect(target_size.logical_pixel_rect<int>());
    ff::rect_int clientRect(renderRect);
    ff::rect_int visibleRect(renderRect);
    int padding1 = (int)(PixelsPerTileF().y * target_size.dpi_scale);
    ff::rect_int padding(padding1, padding1, padding1, padding1);

    if (visibleRect.right > padding.left + padding.right)
    {
        visibleRect.left += padding.left;
        visibleRect.right -= padding.right;
    }

    if (visibleRect.bottom > padding.top + padding.bottom)
    {
        visibleRect.top += padding.top;
        visibleRect.bottom -= padding.bottom;
    }

    ff::point_int renderSize = renderRect.size();
    ff::point_int clientSize = clientRect.size();
    ff::point_int visibleSize = visibleRect.size();

    if (pGame &&
        renderSize.x >= 64 &&
        renderSize.y >= 64 &&
        visibleSize.x >= 64 &&
        visibleSize.y >= 64)
    {
        ff::point_int tiles = pGame->GetSizeInTiles();
        ff::point_int tileSize = PixelsPerTile();
        ff::point_int playPixelSize(tiles.x * tileSize.x, tiles.y * tileSize.y);
        ff::rect_int playRenderRect(0, 0, playPixelSize.x, playPixelSize.y);

        _levelRect = playRenderRect.cast<float>();

        playRenderRect = playRenderRect.scale_to_fit(visibleRect).center(visibleRect);
        playRenderRect.left = playRenderRect.left * renderSize.x / clientSize.x;
        playRenderRect.right = playRenderRect.right * renderSize.x / clientSize.x;
        playRenderRect.top = playRenderRect.top * renderSize.y / clientSize.y;
        playRenderRect.bottom = playRenderRect.bottom * renderSize.y / clientSize.y;

        _renderRect = playRenderRect.cast<float>();

        ff::dxgi::draw_ptr draw = ff::dxgi::global_draw_device().begin_draw(context, target, &depth, _renderRect,
            ff::rect_float(ff::point_float{}, playPixelSize.cast<float>()));
        if (draw)
        {
            pGame->Render(*draw);
            RenderPacPressing(*draw);
            RenderDebugGrid(*draw, tiles);
        }
    }
}

void PacApplication::RenderPacPressing(ff::dxgi::draw_base& draw)
{
    std::shared_ptr<IPlayingActor> pac = GetCurrentPac();
    check_ret(pac);

    ff::point_int pressDir = pac->GetPressDir();
    check_ret(pressDir);

    float rotation = 0;
    float scale = 0;
    float opacity = 0;

    if (_touching)
    {
        if (_touchOffset)
        {
            rotation = ff::math::radians_to_degrees<float>((float)std::atan2(-_touchOffset.y, _touchOffset.x));
            scale = (float)std::min(2.0, (_touchLen - TOUCH_DEAD_ZONE) / 80.0 + 0.75);
            opacity = (float)std::clamp(1.25 / scale, 0.25, 1.0);
        }
    }
    // else // render for non-touch controls
    // {
    //     rotation = ff::math::radians_to_degrees<float>((float)std::atan2(-pressDir.y, pressDir.x));
    //     scale = 1.25;
    //     opacity = 0.5;
    // }

    if (opacity != 0 && scale != 0)
    {
        ff::point_float arrowPos = pac->GetPixel().cast<float>() + PixelsPerTileF() * ff::point_float(0, 3);

        draw.draw_sprite(_touchArrowSprite->sprite_data(),
            ff::transform(arrowPos, ff::point_float(scale, scale), rotation, DirectX::XMFLOAT4(1, 1, 1, opacity)));
    }
}

void PacApplication::RenderDebugGrid(ff::dxgi::draw_base& draw, ff::point_int tiles)
{
#ifdef _DEBUG
    if (ff::input::keyboard().pressing('G'))
    {
        ff::point_float tileSizeF = PixelsPerTileF();

        for (int i = 0; i < tiles.x; i++)
        {
            draw.draw_line(
                ff::point_float(i * tileSizeF.x, 0),
                ff::point_float(i * tileSizeF.x, tiles.y * tileSizeF.y),
                DirectX::XMFLOAT4(1, 1, 1, 0.25f), 1, true);
        }

        for (int i = 0; i < tiles.y; i++)
        {
            draw.draw_line(
                ff::point_float(0, i * tileSizeF.y),
                ff::point_float(tiles.x * tileSizeF.x, i * tileSizeF.y),
                DirectX::XMFLOAT4(1, 1, 1, 0.25f), 1, true);
        }
    }
#endif
}

void PacApplication::SetState(EAppState state)
{
    switch (state)
    {
        case APP_TITLE:
            {
                std::shared_ptr<IPlayingGame> pTitle = std::make_shared<TitleScreen>();
                std::swap(_game, pTitle);
                _state = APP_TITLE;
            }
            break;

        case APP_PLAYING_GAME:
            if (!_game)
            {
                int players = _options.get<int>(OPTION_PAC_PLAYERS, DEFAULT_PAC_PLAYERS);
                std::shared_ptr<IMazes> pMazes = CreateMazesFromId(TitleScreen::GetMazesID());
                std::shared_ptr<IPlayingGame> pGame = IPlayingGame::Create(pMazes, players, this);
                std::swap(_game, pGame);
            }

            _state = APP_PLAYING_GAME;
            break;

        case APP_HIGH_SCORE:
            assert(_game && _pushedGame);
            _state = APP_HIGH_SCORE;
            break;
    }

    assert(_state == state);
}

std::shared_ptr<IPlayingActor> PacApplication::GetCurrentPac() const
{
    std::shared_ptr<IPlayer> player = _game ? _game->GetPlayer(_game->GetCurrentPlayer()) : nullptr;
    std::shared_ptr<IPlayingMaze> playMaze = player ? player->GetPlayingMaze() : nullptr;
    std::shared_ptr<IPlayingActor> pac = playMaze ? playMaze->GetPac() : nullptr;

    return pac;
}
