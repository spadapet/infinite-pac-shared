#include "pch.h"
#include "Core/Audio.h"
#include "Core/GlobalResources.h"
#include "Core/Helpers.h"
#include "Core/Maze.h"
#include "Core/Mazes.h"
#include "Core/PlayingGame.h"
#include "Core/RenderMaze.h"
#include "Core/RenderText.h"
#include "Core/Tiles.h"
#include "States/PacApplication.h"
#include "TitleScreen.h"

TitleScreen::Option::Option()
{
}

TitleScreen::Option::Option(EOption type, ff::point_int pos, GetTextFunc textFunc, int nMazes)
    : _type(type)
    , _selectedPos(pos)
    , _textFunc(textFunc)
    , _hoverOpacity(0)
    , _mazes(nMazes)
{
}

std::string TitleScreen::Option::GetText() const
{
    return (_textFunc != nullptr) ? _textFunc() : std::string("---");
}

ff::rect_float TitleScreen::Option::GetLevelRect() const
{
    std::string text = GetText();

    ff::point_float pos = _selectedPos.cast<float>();
    pos += PixelsPerTileF() * ff::point_float(1.5f, -0.5f);

    return ff::rect_float(pos, pos + PixelsPerTileF() * ff::point_float((float)text.size(), 1));
}

ff::rect_float TitleScreen::Option::GetTargetRect() const
{
    ff::rect_float renderRect = PacApplication::Get()->GetRenderRect();
    ff::rect_float levelRect = PacApplication::Get()->GetLevelRect();

    if (renderRect.empty() || levelRect.empty())
    {
        return ff::rect_float{};
    }

    ff::point_float scale(renderRect.width() / levelRect.width(), renderRect.height() / levelRect.height());
    ff::rect_float targetRect = GetLevelRect() * scale + renderRect.top_left();
    return targetRect;
}

TitleScreen::TitleScreen()
    : _inputRes(GetGlobalInputMapping())
    , _sounds(ISoundEffects::Create(CHAR_DEFAULT))
{
    verify(CreateFrontMaze());
    verify(CreateBackMaze());

    CreateOptions();
    SelectDefaultOption();
    UpdateHighScores();
}

void TitleScreen::SelectDefaultOption()
{
    const ff::dict& options = PacApplication::Get()->GetOptions();
    int defaultMazes = options.get<int>(PacApplication::OPTION_PAC_MAZES, PacApplication::DEFAULT_PAC_MAZES);
    check_ret(defaultMazes != -1);

    for (size_t i = 0; i < _options.size(); i++)
    {
        if (_options[i]._mazes == defaultMazes)
        {
            _curOption = i;
            break;
        }
    }
}

void TitleScreen::UpdateHighScores()
{
    _scores.clear();

    ff::dict& options = PacApplication::Get()->GetOptions();
    options.set<int>(PacApplication::OPTION_PAC_MAZES, _options[_curOption]._mazes);

    if (GetMazesID().size())
    {
        const Stats& stats = Stats::Get(GetMazesID());

        for (size_t i = 0; i < GetHighScoreDisplayCount() && i < _countof(stats._highScores); i++)
        {
            _scores += (i > 0) ? "\n" : "";
            _scores += FormatScoreAsString(stats._highScores[i]._score);
            _scores += "  ";
            _scores += FormatHighScoreName(stats._highScores[i]._name);
        }
    }
}

void TitleScreen::CreateOptions()
{
    int lineHeight = (int)GetLineHeight();
    ff::point_int optionPos = TileBottomRightToPixel(ff::point_int(5, 3)) + ff::point_int(PixelsPerTile().x / -2, 0);
    const ff::dict* options = &PacApplication::Get()->GetOptions();

    auto mrPacText = []() { return std::string("MR.PAC"); };
    auto msPacText = []() { return std::string("MS.LILA"); };
    auto aboutText = []() { return std::string("ABOUT"); };

    auto playersText = [options]()
        {
            return (options->get<int>(PacApplication::OPTION_PAC_PLAYERS, PacApplication::DEFAULT_PAC_PLAYERS) == 1)
                ? std::string("PLAYERS:ONE")
                : std::string("PLAYERS:TWO");
        };

    auto diffText = [options]()
        {
            switch (options->get<int>(PacApplication::OPTION_PAC_DIFF, PacApplication::DEFAULT_PAC_DIFF))
            {
                case 0: return std::string("DIFFICULTY:EASY");
                case 1: default: return std::string("DIFFICULTY:NORMAL");
                case 2: return std::string("DIFFICULTY:HARD");
            }
        };

    auto soundText = [options]()
        {
            return options->get<bool>(PacApplication::OPTION_SOUND_ON, PacApplication::DEFAULT_SOUND_ON)
                ? std::string("SOUND:ON")
                : std::string("SOUND:OFF");
        };

    auto vibrateText = [options]()
        {
            return options->get<bool>(PacApplication::OPTION_VIBRATE_ON, PacApplication::DEFAULT_VIBRATE_ON)
                ? std::string("VIBRATE:ON")
                : std::string("VIBRATE:OFF");
        };

    auto fullScreenText = []()
        {
            return ff::app_window().full_screen()
                ? std::string("FULL SCREEN:ON")
                : std::string("FULL SCREEN:OFF");
        };

    _options.push_back(Option(OPT_PAC, optionPos, mrPacText, 0));
    optionPos.y += lineHeight;

    _options.push_back(Option(OPT_MSPAC, optionPos, msPacText, 1));
    optionPos.y += lineHeight * 2;

    _options.push_back(Option(OPT_PLAYERS, optionPos, playersText));
    optionPos.y += lineHeight;

    _options.push_back(Option(OPT_DIFF, optionPos, diffText));
    optionPos.y += lineHeight;

    _options.push_back(Option(OPT_SOUND, optionPos, soundText));
    optionPos.y += lineHeight;

    _options.push_back(Option(OPT_VIBRATE, optionPos, vibrateText));
    optionPos.y += lineHeight;

    _options.push_back(Option(OPT_FULL_SCREEN, optionPos, fullScreenText));
    optionPos.y += lineHeight;

    _options.push_back(Option(OPT_ABOUT, optionPos, aboutText));
    optionPos.y += lineHeight;
}

bool TitleScreen::CreateFrontMaze()
{
    std::shared_ptr<IMaze> pMaze = CreateMazeFromResource("title-maze-front");
    _render = IRenderMaze::Create(pMaze);
    _text = IRenderText::Create();

    _mazes = IMazes::Create();
    _mazes->AddMaze(0, pMaze);

    return true;
}

bool TitleScreen::CreateBackMaze()
{
    std::shared_ptr<IMaze> pBackMaze = CreateMazeFromResource("title-maze-back");
    pBackMaze->SetCharType(CHAR_MS); // to get random ghost scattering

    _backMaze = IPlayingMaze::Create(pBackMaze, GetEmptyDifficulty(), this);
    pBackMaze = _backMaze->GetMaze();

    // Remove all dots from the maze
    ff::point_int mazeSize = pBackMaze->GetSizeInTiles();

    for (ff::point_int tile(0, 0); tile.y < mazeSize.y; tile.y++, tile.x = 0)
    {
        for (; tile.x < mazeSize.x; tile.x++)
        {
            switch (pBackMaze->GetTileContent(tile))
            {
                case CONTENT_DOT:
                case CONTENT_POWER:
                    _backMaze->GetMaze()->SetTileContent(tile, CONTENT_NOTHING);
                    break;
            }
        }
    }

    return true;
}

void TitleScreen::Execute(EOption option, bool pressedLeft)
{
    bool playEffect = false;
    ff::dict& appOptions = PacApplication::Get()->GetOptions();

    switch (option)
    {
        case OPT_PAC:
        case OPT_MSPAC:
            _curOption = (option == OPT_MSPAC) ? 1 : 0;
            appOptions.set<int>(PacApplication::OPTION_PAC_MAZES, _options[_curOption]._mazes);
            _done = !GetMazesID().empty();
            break;

        case OPT_PLAYERS:
            {
                playEffect = true;
                int value = appOptions.get<int>(PacApplication::OPTION_PAC_PLAYERS, PacApplication::DEFAULT_PAC_PLAYERS);
                appOptions.set<int>(PacApplication::OPTION_PAC_PLAYERS, (value == 1) ? 2 : 1);
            }
            break;

        case OPT_DIFF:
            {
                playEffect = true;
                int value = appOptions.get<int>(PacApplication::OPTION_PAC_DIFF, PacApplication::DEFAULT_PAC_DIFF);
                appOptions.set<int>(PacApplication::OPTION_PAC_DIFF, (value + (pressedLeft ? 2 : 1)) % 3);
            }
            break;

        case OPT_SOUND:
            {
                playEffect = true;
                bool value = appOptions.get<bool>(PacApplication::OPTION_SOUND_ON, PacApplication::DEFAULT_SOUND_ON);
                appOptions.set<bool>(PacApplication::OPTION_SOUND_ON, !value);
            }
            break;

        case OPT_VIBRATE:
            {
                playEffect = true;
                bool value = appOptions.get<bool>(PacApplication::OPTION_VIBRATE_ON, PacApplication::DEFAULT_VIBRATE_ON);
                appOptions.set<bool>(PacApplication::OPTION_VIBRATE_ON, !value);
            }
            break;

        case OPT_FULL_SCREEN:
            {
                playEffect = true;
                bool value = !appOptions.get<bool>(PacApplication::OPTION_FULL_SCREEN, PacApplication::DEFAULT_FULL_SCREEN);
                appOptions.set<bool>(PacApplication::OPTION_FULL_SCREEN, value);
                ff::app_window().full_screen(value);
            }
            break;

        case OPT_ABOUT:
            playEffect = true;
            PacApplication::Get()->GetHost().ShowAboutDialog();
            break;
    }

    if (playEffect)
    {
        _sounds->Play(EFFECT_FRUIT_BOUNCE);
    }
}

TitleScreen::Option* TitleScreen::HitTestOption(ff::point_float pos)
{
    ff::rect_float renderRect = PacApplication::Get()->GetRenderRect();
    ff::rect_float levelRect = PacApplication::Get()->GetLevelRect();
    check_ret_val(!renderRect.empty() && !levelRect.empty(), nullptr);

    ff::point_float scale(renderRect.width() / levelRect.width(), renderRect.height() / levelRect.height());
    ff::point_float slop = PixelsPerTileF() * scale / -4.0f;

    for (Option& option : _options)
    {
        ff::rect_float rect = option.GetTargetRect();
        rect.deflate(slop);

        if (rect.contains(pos))
        {
            return &option;
        }
    }

    return nullptr;
}

// static
float TitleScreen::GetLineHeight()
{
    return ::PixelsPerTileF().y * 3 / 2;
}

// static
size_t TitleScreen::GetHighScoreDisplayCount()
{
    return 10;
}

// static
std::string TitleScreen::GetMazesID()
{
    const ff::dict& options = PacApplication::Get()->GetOptions();
    int mazes = options.get<int>(PacApplication::OPTION_PAC_MAZES, PacApplication::DEFAULT_PAC_MAZES);
    int diff = options.get<int>(PacApplication::OPTION_PAC_DIFF, PacApplication::DEFAULT_PAC_DIFF);

    std::string idString;
    switch (mazes)
    {
        case 0: idString = "mr-mazes"; break;
        case 1: idString = "ms-mazes"; break;
        default: return idString;
    }

    std::string diffString;
    switch (diff)
    {
        case 0: diffString = "-easy"; break;
        case 1: diffString = "-normal"; break;
        case 2: diffString = "-hard"; break;
        default: return diffString;
    }

    return idString + diffString;
}

void TitleScreen::Advance()
{
    ff::pointer_device& pointer = ff::input::pointer();
    ff::point_float pointerPos = pointer.pos().cast<float>();
    Option* optionUnderPointer = HitTestOption(pointerPos);

    if (_backMaze)
    {
        _backMaze->Advance();
    }

    if (_render)
    {
        _render->Advance(true, false, false, this);
    }

    int nChangeOption = 0;
    bool bExecute = false;
    bool bLeft = false;

    if (!_done && _inputRes->advance())
    {
        for (const ff::input_event& ie : _inputRes->events())
        {
            if (ie.started())
            {
                if (ie.event_id == GetEventUp())
                {
                    nChangeOption = -1;
                }
                else if (ie.event_id == GetEventDown())
                {
                    nChangeOption = 1;
                }
                else if (ie.event_id == GetEventLeft() || ie.event_id == GetEventRight())
                {
                    switch (_options[_curOption]._type)
                    {
                        case OPT_PLAYERS:
                        case OPT_DIFF:
                        case OPT_SOUND:
                        case OPT_VIBRATE:
                        case OPT_FULL_SCREEN:
                            bExecute = true;
                            bLeft = (ie.event_id == GetEventLeft());
                            break;
                    }
                    break;
                }
                else if (ie.event_id == GetEventAction() || ie.event_id == GetEventStart())
                {
                    bExecute = true;
                }
            }
        }
    }

    if (pointer.release_count(VK_LBUTTON) && optionUnderPointer)
    {
        Execute(optionUnderPointer->_type, false);
    }

    if (nChangeOption)
    {
        bool playEffect = false;

        if (nChangeOption > 0 && _curOption + 1 < _options.size())
        {
            playEffect = true;
            _curOption++;
            UpdateHighScores();
        }
        else if (nChangeOption < 0 && _curOption > 0)
        {
            playEffect = true;
            _curOption--;
            UpdateHighScores();
        }

        if (playEffect)
        {
            _sounds->Play((_curOption % 2) ? EFFECT_EAT_DOT2 : EFFECT_EAT_DOT1);
        }
    }
    else if (bExecute)
    {
        Execute(_options[_curOption]._type, bLeft);
    }

    if (pointer.relative_pos() != ff::point_double{} && optionUnderPointer)
    {
        optionUnderPointer->_hoverOpacity = 1;
    }

    for (Option& option : _options)
    {
        if (option._hoverOpacity > 0)
        {
            if (nChangeOption)
            {
                option._hoverOpacity = 0;
            }
            else if (optionUnderPointer != &option)
            {
                option._hoverOpacity = std::max(0.0f, option._hoverOpacity - 1.0f / 24.0f);
            }
        }
    }
}

void TitleScreen::Render(ff::dxgi::draw_base& draw)
{
    RenderBackMaze(draw);
    RenderFrontMaze(draw);
    RenderOptions(draw);
    RenderFade(draw);
}

void TitleScreen::RenderBackMaze(ff::dxgi::draw_base& draw)
{
    check_ret(_backMaze);

    _backMaze->Render(draw);

    static const DirectX::XMFLOAT4 s_color(0, 0, 0, 0.75f);
    ff::point_int tiles = GetSizeInTiles();
    draw.draw_rectangle(ff::rect_float(0, 0, tiles.x * PixelsPerTileF().x, tiles.y * PixelsPerTileF().y), s_color);

}

void TitleScreen::RenderFrontMaze(ff::dxgi::draw_base& draw)
{
    _render->RenderTheMaze(draw);
    _render->RenderActors(draw, true, false, false, this);
}

void TitleScreen::RenderOptions(ff::dxgi::draw_base& draw)
{
    check_ret(_text);

    static DirectX::XMFLOAT4 s_textColor(1, 1, 1, 1);
    static DirectX::XMFLOAT4 s_titleColor(0.75, 0, 0, 1);
    float lineHeight = GetLineHeight();

    for (const Option& option : _options)
    {
        std::string text = option.GetText();
        ff::point_float pos = option.GetLevelRect().top_left();

        _text->DrawText(draw, text.c_str(), pos, lineHeight, &s_textColor, nullptr, nullptr);

        if (option._hoverOpacity > 0)
        {
            DirectX::XMFLOAT4 hoverColor(0.349f, 0.486f, 0.812f, option._hoverOpacity);
            _text->DrawText(draw, text.c_str(), pos, lineHeight, &hoverColor, nullptr, nullptr);
        }
    }

    _text->DrawText(
        draw,
        " START GAME:",
        TileTopLeftToPixelF(ff::point_int(6, 2)),
        lineHeight,
        &s_titleColor,
        nullptr, nullptr);

    if (_scores.size())
    {
        _text->DrawText(
            draw,
            "HIGH SCORES:",
            TileTopLeftToPixelF(ff::point_int(6, 17)) + ff::point_float(0, PixelsPerTileF().y / 2),
            lineHeight,
            &s_titleColor,

        nullptr, nullptr);
        _text->DrawText(
            draw,
            _scores.c_str(),
            TileTopLeftToPixelF(ff::point_int(6, 19)),
            0, &s_textColor,
            nullptr, nullptr);
    }
}

void TitleScreen::RenderFade(ff::dxgi::draw_base& draw)
{
    check_ret(_fading);

    _fade += _done ? 0.015625f : -0.015625f;

    _fade = std::max<float>(0, _fade);
    _fade = std::min<float>(1, _fade);

    if (_fade == 0 || _fade == 1)
    {
        _fading = false;
    }

    if (_fade > 0)
    {
        DirectX::XMFLOAT4 color(0, 0, 0, _fade);
        ff::point_int tiles = GetSizeInTiles();
        draw.draw_rectangle(ff::rect_float(0, 0, tiles.x * PixelsPerTileF().x, tiles.y * PixelsPerTileF().y), color);
    }
}

std::shared_ptr<IMazes> TitleScreen::GetMazes()
{
    return _mazes;
}

ff::point_int TitleScreen::GetSizeInTiles() const
{
    if (_render && _render->GetMaze())
    {
        return _render->GetMaze()->GetSizeInTiles();
    }

    assert_ret_val(false, ff::point_int(16, 16));
}

size_t TitleScreen::GetHighScore() const
{
    return 0;
}

size_t TitleScreen::GetPlayers() const
{
    return 0;
}

size_t TitleScreen::GetCurrentPlayer() const
{
    return 0;
}

std::shared_ptr<IPlayer> TitleScreen::GetPlayer(size_t nPlayer)
{
    return nullptr;
}

bool TitleScreen::IsGameOver() const
{
    return _done;
}

bool TitleScreen::IsPaused() const
{
    return false;
}

void TitleScreen::TogglePaused()
{
}

void TitleScreen::PausedAdvance()
{
}

size_t TitleScreen::GetMazePlayer()
{
    return ff::constants::invalid_unsigned<size_t>();
}

bool TitleScreen::IsPlayingLevel()
{
    return false;
}

bool TitleScreen::IsEffectEnabled(AudioEffect effect)
{
    return false;
}

void TitleScreen::OnStateChanged(GameState oldState, GameState newState)
{
}

void TitleScreen::OnPacUsingTunnel()
{
}

void TitleScreen::Reset()
{
}

GameState TitleScreen::GetGameState() const
{
    return GS_PLAYING;
}

const Stats& TitleScreen::GetStats() const
{
    return _stats;
}

std::shared_ptr<IMaze> TitleScreen::GetMaze() const
{
    return _render ? _render->GetMaze() : nullptr;
}

std::shared_ptr<IRenderMaze> TitleScreen::GetRenderMaze()
{
    return _render;
}

const Difficulty& TitleScreen::GetDifficulty() const
{
    return GetEmptyDifficulty();
}

PacState TitleScreen::GetPacState() const
{
    return PAC_NORMAL;
}

std::shared_ptr<IPlayingActor> TitleScreen::GetPac()
{
    return shared_from_this();
}

CharType TitleScreen::GetCharType() const
{
    return (_curOption == 1) ? CHAR_MS : CHAR_MR;
}

bool TitleScreen::IsPowerPac() const
{
    return false;
}

size_t TitleScreen::GetGhostCount() const
{
    return 0;
}

ff::point_int TitleScreen::GetGhostEyeDir(size_t nGhost) const
{
    return ff::point_int(0, 0);
}

GhostState TitleScreen::GetGhostState(size_t nGhost) const
{
    return GHOST_INVALID;
}

std::shared_ptr<IPlayingActor> TitleScreen::GetGhost(size_t nGhost)
{
    return nullptr;
}

ff::point_int TitleScreen::GetGhostStartPixel() const
{
    return ff::point_int(0, 0);
}

ff::point_int TitleScreen::GetGhostStartTile() const
{
    return ff::point_int(0, 0);
}

FruitState TitleScreen::GetFruitState() const
{
    return FRUIT_INVALID;
}

std::shared_ptr<IPlayingActor> TitleScreen::GetFruit()
{
    return nullptr;
}

FruitType TitleScreen::GetFruitType() const
{
    return FRUIT_NONE;
}

ff::point_int TitleScreen::GetFruitExitTile() const
{
    return ff::point_int(0, 0);
}

std::shared_ptr<PointActor> const* TitleScreen::GetPointDisplays(size_t& nCount) const
{
    nCount = 0;
    return nullptr;
}

std::shared_ptr<CustomActor> const* TitleScreen::GetCustomActors(size_t& nCount) const
{
    nCount = 0;
    return nullptr;
}

ff::point_int TitleScreen::GetTile() const
{
    return PixelToTile(GetPixel());
}

ff::point_int TitleScreen::GetPixel() const
{
    return _options[_curOption]._selectedPos;
}

void TitleScreen::SetPixel(ff::point_int pixel)
{
}

ff::point_int TitleScreen::GetDir() const
{
    return ff::point_int(1, 0);
}

void TitleScreen::SetDir(ff::point_int dir)
{
}

ff::point_int TitleScreen::GetPressDir() const
{
    return ff::point_int(0, 0);
}

void TitleScreen::SetPressDir(ff::point_int dir)
{
}

bool TitleScreen::IsActive() const
{
    return true;
}

void TitleScreen::SetActive(bool bActive)
{
}
