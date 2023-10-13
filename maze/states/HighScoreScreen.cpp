#include "pch.h"
#include "Core/GlobalResources.h"
#include "Core/Helpers.h"
#include "Core/Maze.h"
#include "Core/Mazes.h"
#include "Core/RenderMaze.h"
#include "Core/RenderText.h"
#include "Core/Tiles.h"
#include "States/PacApplication.h"
#include "States/TitleScreen.h"
#include "HighScoreScreen.h"

// STATIC_DATA(pod)
static const size_t s_nSpaceLetter = 26;
static const size_t s_nPeriodLetter = 27;
static const size_t s_nBackLetter = 28;
static const size_t s_nEndLetter = 29;

HighScoreScreen::HighScoreScreen(std::string_view mazesId, std::shared_ptr<IPlayer> pPlayer)
    : _inputRes(GetGlobalInputMapping())
{
    Stats& stats = Stats::Get(mazesId);

    size_t nSlot = stats.GetHighScoreSlot(pPlayer->GetScore());
    assert(nSlot >= 0 && nSlot < TitleScreen::GetHighScoreDisplayCount());

    _mazesID = mazesId;
    _player = pPlayer;

    char szIntro[512];

    _snprintf_s(szIntro,
        _TRUNCATE,
        "      %s\n\n\n\n"
        " New high score %02Iu!\n\n"
        "    Enter name:",
        FormatScoreAsString(_player->GetScore()).c_str(),
        nSlot + 1);

    _intro = szIntro;

    for (char ch = 'a'; ch <= 'z'; ch++)
    {
        Letter letter{};
        letter._letter[0] = ch;
        letter._pos = GetLetterPos(ch);
        letter._vk = ch - 'a' + 'A';

        _letters.push_back(letter);
    }

    // Space
    {
        assert(_letters.size() == s_nSpaceLetter);

        Letter letter{};
        letter._letter[0] = ' ';
        letter._pos = GetLetterPos('z') + ff::point_float(2, 0) * PixelsPerTileF();
        letter._vk = VK_SPACE;

        _letters.push_back(letter);
    }

    // Period
    {
        assert(_letters.size() == s_nPeriodLetter);

        Letter letter{};
        letter._letter[0] = '.';
        letter._pos = GetLetterPos('z') + ff::point_float(4, 0) * PixelsPerTileF();
        letter._vk = VK_OEM_PERIOD;

        _letters.push_back(letter);
    }

    // Back
    {
        assert(_letters.size() == s_nBackLetter);

        Letter letter{};
        strcpy_s(letter._letter, "Back");
        letter._pos = TileTopLeftToPixelF(ff::point_int(7, 28)) + ff::point_float(PixelsPerTileF().x / 2, 0);
        letter._vk = VK_BACK;

        _letters.push_back(letter);
    }

    // End
    {
        assert(_letters.size() == s_nEndLetter);

        Letter letter{};
        strcpy_s(letter._letter, "End");
        letter._pos = TileTopLeftToPixelF(ff::point_int(17, 28)) + ff::point_float(PixelsPerTileF().x / 2, 0);
        letter._vk = VK_RETURN;

        _letters.push_back(letter);
    }

    std::shared_ptr<IMaze> pMaze;
    pMaze = CreateMazeFromResource("high-score-maze");
    _render = IRenderMaze::Create(pMaze);
    _text = IRenderText::Create();

    _mazes = IMazes::Create();
    _mazes->AddMaze(0, pMaze);
}

HighScoreScreen::Letter* HighScoreScreen::GetLetter(char ch)
{
    ch = tolower(ch);

    if (ch >= 'a' && ch <= 'z')
    {
        size_t i = ch - 'a';

        if (i >= 0 && i < _letters.size())
        {
            return &_letters[i];
        }
    }

    return nullptr;
}

ff::point_float HighScoreScreen::GetLetterPos(char ch)
{
    ch = tolower(ch);

    if (ch >= 'a' && ch <= 'z')
    {
        size_t i = ch - 'a';

        return TileTopLeftToPixelF(ff::point_int(
            (int)(i % 7) * 2 + 7,
            (int)(i / 7) * 2 + 20)) + ff::point_float(PixelsPerTileF().x / 2.0f, 0);
    }

    assert_ret_val(false, ff::point_float{});
}

HighScoreScreen::Letter* HighScoreScreen::HitTestLetter(ff::point_float pos)
{
    ff::rect_float renderRect = PacApplication::Get()->GetRenderRect();
    ff::rect_float levelRect = PacApplication::Get()->GetLevelRect();
    check_ret_val(!renderRect.empty() && !levelRect.empty(), nullptr);

    ff::point_float scale(renderRect.width() / levelRect.width(), renderRect.height() / levelRect.height());
    ff::point_float slop = PixelsPerTileF() * scale;

    // The maze is shifted down by 3
    pos.y -= PixelsPerTileF().y * scale.y * 3.0f;

    for (Letter& letter : _letters)
    {
        ff::rect_float rect(letter._pos * scale, ff::point_float(0, 0));
        rect.right = rect.left + (strlen(letter._letter) - 1) * PixelsPerTileF().x * scale.x;
        rect.bottom = rect.top;

        rect.offset(renderRect.top_left() + slop / 2.0f);
        rect.deflate(-slop);

        if (rect.contains(pos))
        {
            return &letter;
        }
    }

    return nullptr;
}

std::string HighScoreScreen::GetName() const
{
    return _name;
}

void HighScoreScreen::Advance()
{
    ff::keyboard_device& keys = ff::input::keyboard();
    ff::pointer_device& pointer = ff::input::pointer();
    ff::point_float pointerPos = pointer.pos().cast<float>();
    Letter* letterUnderPointer = HitTestLetter(pointerPos);

    if (_render)
    {
        _render->Advance(true, false, false, this);
    }

    _showNewLetter = (_counter % 60) < 30;
    _counter++;

    ff::point_float change(0, 0);
    bool bExecute = false;
    bool bTyped = false;

    if (!_done && _inputRes->advance())
    {
        for (const ff::input_event& ie : _inputRes->events())
        {
            if (ie.started())
            {
                if (ie.event_id == GetEventUp())
                {
                    change.y = -1;
                }
                else if (ie.event_id == GetEventDown())
                {
                    change.y = 1;
                }
                else if (ie.event_id == GetEventLeft())
                {
                    change.x = -1;
                }
                else if (ie.event_id == GetEventRight())
                {
                    change.x = 1;
                }
                else if (ie.event_id == GetEventAction())
                {
                    bExecute = true;
                }
                else if (ie.event_id == GetEventCancel())
                {
                    _done = true;
                }
            }
        }
    }

    if (_letter >= 0 && _letter < _letters.size())
    {
        Letter* pNewLetter = nullptr;
        ff::point_float pos = _letters[_letter]._pos;
        ff::point_float newPos = _letters[_letter]._pos + change * 2.0f * PixelsPerTileF();

        if (newPos != pos)
        {
            // Find the closest letter

            float dist = std::numeric_limits<float>::max();

            for (size_t i = 0; i < _letters.size(); i++)
            {
                ff::point_float letterPos = _letters[i]._pos;

                if ((!change.y && letterPos.y != pos.y) ||
                    (!change.x && letterPos.x != pos.x) ||
                    (change.x > 0 && letterPos.x <= pos.x) ||
                    (change.x < 0 && letterPos.x >= pos.x) ||
                    (change.y > 0 && letterPos.y <= pos.y) ||
                    (change.y < 0 && letterPos.y >= pos.y))
                {
                    continue;
                }

                float newDist =
                    (letterPos.x - newPos.x) * (letterPos.x - newPos.x) +
                    (letterPos.y - newPos.y) * (letterPos.y - newPos.y);

                if (newDist < dist)
                {
                    dist = newDist;
                    _letter = i;
                }
            }

            if (_letters[_letter]._pos == pos)
            {
                // Didn't move, maybe force a move

                if (change.y > 0 && _letter <= 'w' - 'a')
                {
                    _letter = s_nBackLetter;
                }
                else if (change.y > 0 && _letter > 'w' - 'a')
                {
                    _letter = s_nEndLetter;
                }
            }
        }

        if (letterUnderPointer)
        {
            bool clicked = pointer.release_count(VK_LBUTTON) > 0;
            if (clicked || pointer.relative_pos() != ff::point_double{})
            {
                _letter = letterUnderPointer - &_letters[0];
            }

            if (clicked)
            {
                bExecute = true;
            }
        }

        for (Letter& letter : _letters)
        {
            if (letter._vk != VK_RETURN && keys.press_count(letter._vk))
            {
                _letter = &letter - &_letters[0];
                bExecute = true;
                bTyped = true;
                break;
            }
        }

        if (bExecute)
        {
            if (_letter < s_nBackLetter)
            {
                if (_name.size() < 9)
                {
                    _name += _letters[_letter]._letter;
                }
            }
            else if (_letter == s_nBackLetter)
            {
                if (_name.size())
                {
                    _name.erase(_name.size() - 1);
                }
            }
            else if (_letter == s_nEndLetter)
            {
                Stats& stats = Stats::Get(_mazesID);
                stats.InsertHighScore(_player->GetScore(), _player->GetLevel(), _name.c_str());

                _done = true;
            }

            if (bTyped)
            {
                _letter = s_nEndLetter;
            }
        }
    }
}

void HighScoreScreen::Render(ff::dxgi::draw_base& draw)
{
    DirectX::XMFLOAT4X4 matrix;
    DirectX::XMStoreFloat4x4(&matrix, DirectX::XMMatrixTranslation(0, 3 * PixelsPerTileF().y, 0));
    draw.world_matrix_stack().push();
    draw.world_matrix_stack().transform(matrix);

    if (_render)
    {
        _render->RenderTheMaze(draw);
        _render->RenderActors(draw, true, false, false, this);
    }

    if (_text)
    {
        static DirectX::XMFLOAT4 s_colorWhite(1, 1, 1, 1);
        static DirectX::XMFLOAT4 s_colorPurple(1, 0, 1, 1);

        for (size_t i = 0; i < _letters.size(); i++)
        {
            Letter& letter = _letters[i];

            _text->DrawText(
                draw,
                letter._letter,
                letter._pos,
                0, &s_colorWhite, nullptr, nullptr);
        }

        if (_letter != ff::constants::invalid_unsigned<size_t>())
        {
            Letter& letter = _letters[_letter];

            _text->DrawText(
                draw,
                letter._letter,
                ff::point_float((float)(letter._pos.x - 1), (float)(letter._pos.y - 1)),
                0, &s_colorPurple, nullptr, nullptr);
        }

        _text->DrawText(
            draw,
            _intro.c_str(),
            TileTopLeftToPixelF(ff::point_int(4, 4)),
            0, &s_colorWhite, nullptr, nullptr);

        if (!_name.empty())
        {
            _text->DrawText(
                draw,
                _name.c_str(),
                TileTopLeftToPixelF(ff::point_int(9, 15)),
                0, &s_colorWhite, nullptr, nullptr);
        }

        size_t nMaxDashes = (_letter < s_nBackLetter && _showNewLetter) ? 8 : 9;

        if (_name.size() < nMaxDashes)
        {
            std::string szDashes(nMaxDashes - _name.size(), '-');

            _text->DrawText(
                draw,
                szDashes.c_str(),
                TileTopLeftToPixelF(ff::point_int(9 + (9 - (int)nMaxDashes) + (int)_name.size(), 15)),
                0, &s_colorWhite, nullptr, nullptr);
        }

        if (_name.size() < 9 && _letter < s_nBackLetter && _showNewLetter)
        {
            _text->DrawText(
                draw,
                _letters[_letter]._letter,
                TileTopLeftToPixelF(ff::point_int(9 + (int)_name.size(), 15)),
                0, &s_colorPurple, nullptr, nullptr);
        }
    }

    draw.world_matrix_stack().pop();
}

std::shared_ptr<IMazes> HighScoreScreen::GetMazes()
{
    return _mazes;
}

ff::point_int HighScoreScreen::GetSizeInTiles() const
{
    if (_render && _render->GetMaze())
    {
        return _render->GetMaze()->GetSizeInTiles() + ff::point_int(0, 5);
    }

    assert_ret_val(false, ff::point_int(16, 16));
}

size_t HighScoreScreen::GetHighScore() const
{
    return 0;
}

size_t HighScoreScreen::GetPlayers() const
{
    return 0;
}

size_t HighScoreScreen::GetCurrentPlayer() const
{
    return 0;
}

std::shared_ptr<IPlayer> HighScoreScreen::GetPlayer(size_t nPlayer)
{
    return nullptr;
}

bool HighScoreScreen::IsGameOver() const
{
    return _done;
}

bool HighScoreScreen::IsPaused() const
{
    return false;
}

void HighScoreScreen::TogglePaused()
{
}

void HighScoreScreen::PausedAdvance()
{
}

size_t HighScoreScreen::GetMazePlayer()
{
    return ff::constants::invalid_unsigned<size_t>();
}

bool HighScoreScreen::IsPlayingLevel()
{
    return false;
}

bool HighScoreScreen::IsEffectEnabled(AudioEffect effect)
{
    return false;
}

void HighScoreScreen::OnStateChanged(GameState oldState, GameState newState)
{
}

void HighScoreScreen::OnPacUsingTunnel()
{
}

void HighScoreScreen::Reset()
{
}

GameState HighScoreScreen::GetGameState() const
{
    return GS_PLAYING;
}

const Stats& HighScoreScreen::GetStats() const
{
    static const Stats s_stats;
    return s_stats;
}

std::shared_ptr<IMaze> HighScoreScreen::GetMaze() const
{
    return _render ? _render->GetMaze() : nullptr;
}

std::shared_ptr<IRenderMaze> HighScoreScreen::GetRenderMaze()
{
    return _render;
}

const Difficulty& HighScoreScreen::GetDifficulty() const
{
    return GetEmptyDifficulty();
}

PacState HighScoreScreen::GetPacState() const
{
    return PAC_NORMAL;
}

std::shared_ptr<IPlayingActor> HighScoreScreen::GetPac()
{
    return shared_from_this();
}

CharType HighScoreScreen::GetCharType() const
{
    return CHAR_DEFAULT;
}

bool HighScoreScreen::IsPowerPac() const
{
    return false;
}

size_t HighScoreScreen::GetGhostCount() const
{
    return 0;
}

ff::point_int HighScoreScreen::GetGhostEyeDir(size_t nGhost) const
{
    return ff::point_int(0, 0);
}

GhostState HighScoreScreen::GetGhostState(size_t nGhost) const
{
    return GHOST_INVALID;
}

std::shared_ptr<IPlayingActor> HighScoreScreen::GetGhost(size_t nGhost)
{
    return nullptr;
}

ff::point_int HighScoreScreen::GetGhostStartPixel() const
{
    return ff::point_int(0, 0);
}

ff::point_int HighScoreScreen::GetGhostStartTile() const
{
    return ff::point_int(0, 0);
}

FruitState HighScoreScreen::GetFruitState() const
{
    return FRUIT_INVALID;
}

std::shared_ptr<IPlayingActor> HighScoreScreen::GetFruit()
{
    return nullptr;
}

FruitType HighScoreScreen::GetFruitType() const
{
    return FRUIT_NONE;
}

ff::point_int HighScoreScreen::GetFruitExitTile() const
{
    return ff::point_int(0, 0);
}

std::shared_ptr<PointActor> const* HighScoreScreen::GetPointDisplays(size_t& nCount) const
{
    nCount = 0;
    return nullptr;
}

std::shared_ptr<CustomActor> const* HighScoreScreen::GetCustomActors(size_t& nCount) const
{
    nCount = 0;
    return nullptr;
}

ff::point_int HighScoreScreen::GetTile() const
{
    return PixelToTile(GetPixel());
}

ff::point_int HighScoreScreen::GetPixel() const
{
    return (_letter >= 0 && _letter < _letters.size())
        ? _letters[_letter]._pos.cast<int>() + PixelsPerTile() / 2
        : ff::point_int{};
}

void HighScoreScreen::SetPixel(ff::point_int pixel)
{
}

ff::point_int HighScoreScreen::GetDir() const
{
    return ff::point_int(1, 0);
}

void HighScoreScreen::SetDir(ff::point_int dir)
{
}

ff::point_int HighScoreScreen::GetPressDir() const
{
    return ff::point_int(0, 0);
}

void HighScoreScreen::SetPressDir(ff::point_int dir)
{
}

bool HighScoreScreen::IsActive() const
{
    return true;
}

void HighScoreScreen::SetActive(bool bActive)
{
}
