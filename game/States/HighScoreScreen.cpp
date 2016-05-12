#include "pch.h"
#include "Core/GlobalResources.h"
#include "Core/Helpers.h"
#include "Core/Maze.h"
#include "Core/Mazes.h"
#include "Core/RenderMaze.h"
#include "Core/RenderText.h"
#include "Core/Tiles.h"
#include "Globals/MetroGlobals.h"
#include "Graph/2D/2dRenderer.h"
#include "States/PacApplication.h"
#include "States/TitleScreen.h"
#include "HighScoreScreen.h"
#include "Input/InputMapping.h"
#include "Input/Joystick/JoystickDevice.h"
#include "Input/Joystick/JoystickInput.h"
#include "Input/KeyboardDevice.h"
#include "Input/PointerDevice.h"

// STATIC_DATA(pod)
static const size_t s_nSpaceLetter = 26;
static const size_t s_nPeriodLetter = 27;
static const size_t s_nBackLetter = 28;
static const size_t s_nEndLetter = 29;

BEGIN_INTERFACES(HighScoreScreen)
	HAS_INTERFACE(IPlayingGame)
	HAS_INTERFACE(IPlayingMaze)
END_INTERFACES()

HighScoreScreen::HighScoreScreen()
	: _done(false)
	, _letter(0)
	, _counter(0)
	, _showNewLetter(true)
	, _inputRes(GetGlobalInputMapping())
{
}

HighScoreScreen::~HighScoreScreen()
{
}

HighScoreScreen::Letter *HighScoreScreen::GetLetter(wchar_t ch)
{
	ch = tolower(ch);

	if (ch >= 'a' && ch <= 'z')
	{
		size_t i = ch - 'a';

		if (i >= 0 && i < _letters.Size())
		{
			return &_letters[i];
		}
	}

	return nullptr;
}

ff::PointFloat HighScoreScreen::GetLetterPos(wchar_t ch)
{
	ch = tolower(ch);

	if (ch >= 'a' && ch <= 'z')
	{
		size_t i = ch - 'a';

		return TileTopLeftToPixelF(ff::PointInt(
			(int)(i % 7) * 2 + 7,
			(int)(i / 7) * 2 + 20)) + ff::PointFloat(PixelsPerTileF().x / 2.0f, 0);
	}

	assertRetVal(false, ff::PointFloat::Zeros());
}

HighScoreScreen::Letter *HighScoreScreen::HitTestLetter(ff::PointFloat pos)
{
	ff::RectFloat renderRect = PacApplication::Get()->GetRenderRect();
	ff::RectFloat levelRect = PacApplication::Get()->GetLevelRect();
	noAssertRetVal(!renderRect.IsEmpty() && !levelRect.IsEmpty(), nullptr);

	ff::PointFloat scale(renderRect.Width() / levelRect.Width(), renderRect.Height() / levelRect.Height());
	ff::PointFloat slop = PixelsPerTileF() * scale;

	// The maze is shifted down by 3
	pos.y -= PixelsPerTileF().y * scale.y * 3.0f;

	for (Letter &letter : _letters)
	{
		ff::RectFloat rect(letter._pos * scale, ff::PointFloat(0, 0));
		rect.right = rect.left + (::wcslen(letter._letter) - 1) * PixelsPerTileF().x * scale.x;
		rect.bottom = rect.top;

		rect.Offset(renderRect.TopLeft() + slop / 2.0f);
		rect.Deflate(-slop);

		if (rect.PointInRect(pos))
		{
			return &letter;
		}
	}

	return nullptr;
}

ff::String HighScoreScreen::GetName() const
{
	return _name;
}

bool HighScoreScreen::Init(ff::StringRef mazesId, IPlayer *pPlayer)
{
	assertRetVal(pPlayer, false);

	// Create text to display

	Stats &stats = Stats::Get(mazesId);

	size_t nSlot = stats.GetHighScoreSlot(pPlayer->GetScore());
	assertRetVal(nSlot >= 0 && nSlot < TitleScreen::GetHighScoreDisplayCount(), false);

	_mazesID = mazesId;
	_player = pPlayer;

	wchar_t szIntro[512];

	_sntprintf_s(szIntro,
		_TRUNCATE,
		L"      %s\n\n\n\n"
		L" New high score %02Iu!\n\n"
		L"    Enter name:",
		FormatScoreAsString(_player->GetScore()).c_str(),
		nSlot + 1);

	_intro = szIntro;

	for (wchar_t ch = 'a'; ch <= 'z'; ch++)
	{
		Letter letter;
		ff::ZeroObject(letter);

		letter._letter[0] = ch;
		letter._pos = GetLetterPos(ch);
		letter._vk = ch - 'a' + 'A';

		_letters.Push(letter);
	}

	// Space
	{
		assert(_letters.Size() == s_nSpaceLetter);

		Letter letter;
		ff::ZeroObject(letter);

		letter._letter[0] = ' ';
		letter._pos = GetLetterPos('z') + ff::PointFloat(2, 0) * PixelsPerTileF();
		letter._vk = VK_SPACE;

		_letters.Push(letter);
	}

	// Period
	{
		assert(_letters.Size() == s_nPeriodLetter);

		Letter letter;
		ff::ZeroObject(letter);

		letter._letter[0] = '.';
		letter._pos = GetLetterPos('z') + ff::PointFloat(4, 0) * PixelsPerTileF();
		letter._vk = VK_OEM_PERIOD;

		_letters.Push(letter);
	}

	// Back
	{
		assert(_letters.Size() == s_nBackLetter);

		Letter letter;
		ff::ZeroObject(letter);

		_tcscpy_s(letter._letter, L"Back");
		letter._pos = TileTopLeftToPixelF(ff::PointInt(7, 28)) + ff::PointFloat(PixelsPerTileF().x / 2, 0);
		letter._vk = VK_BACK;

		_letters.Push(letter);
	}

	// End
	{
		assert(_letters.Size() == s_nEndLetter);

		Letter letter;
		ff::ZeroObject(letter);

		_tcscpy_s(letter._letter, L"End");
		letter._pos = TileTopLeftToPixelF(ff::PointInt(17, 28)) + ff::PointFloat(PixelsPerTileF().x / 2, 0);
		letter._vk = VK_RETURN;

		_letters.Push(letter);
	}

	assertRetVal(IMazes::Create(&_mazes), false);

	ff::ComPtr<IMaze> pMaze;
	assertRetVal(CreateMazeFromResource(ff::String(L"high-score-maze"), &pMaze), false);
	assertRetVal(IRenderMaze::Create(pMaze, &_render), false);
	assertRetVal(IRenderText::Create(&_text), false);

	_mazes->AddMaze(0, pMaze);

	return true;
}

void HighScoreScreen::Advance()
{
	ff::IInputMapping *inputMap = _inputRes.GetObject();
	ff::IKeyboardDevice *keys = ff::MetroGlobals::Get()->GetKeys();
	ff::IPointerDevice *pointer = ff::MetroGlobals::Get()->GetPointer();
	ff::PointFloat pointerPos = pointer->GetPos().ToFloat();
	Letter *letterUnderPointer = HitTestLetter(pointerPos);

	if (_render)
	{
		_render->Advance(true, false, false, this);
	}

	_showNewLetter = (_counter % 60) < 30;
	_counter++;

	ff::PointFloat change(0, 0);
	bool bExecute = false;
	bool bTyped = false;

	if (inputMap)
	{
		if (!_done)
		{
			inputMap->Advance(ff::MetroGlobals::Get()->GetGlobalTime()._secondsPerAdvance);
		}
		else
		{
			inputMap->ClearEvents();
		}

		for (const ff::InputEvent &ie : inputMap->GetEvents())
		{
			if (ie.IsStart())
			{
				if (ie._eventID == GetEventUp())
				{
					change.y = -1;
				}
				else if (ie._eventID == GetEventDown())
				{
					change.y = 1;
				}
				else if (ie._eventID == GetEventLeft())
				{
					change.x = -1;
				}
				else if (ie._eventID == GetEventRight())
				{
					change.x = 1;
				}
				else if (ie._eventID == GetEventAction())
				{
					bExecute = true;
				}
				else if (ie._eventID == GetEventCancel())
				{
					_done = true;
				}
			}
		}
	}

	if (_letter >= 0 && _letter < _letters.Size())
	{
		Letter *pNewLetter = nullptr;
		ff::PointFloat pos = _letters[_letter]._pos;
		ff::PointFloat newPos = _letters[_letter]._pos + change * 2.0f * PixelsPerTileF();

		if (newPos != pos)
		{
			// Find the closest letter

			float dist = std::numeric_limits<float>::max();

			for (size_t i = 0; i < _letters.Size(); i++)
			{
				ff::PointFloat letterPos = _letters[i]._pos;

				if ((!change.y && letterPos.y != pos.y) ||
					(!change.x && letterPos.x != pos.x) ||
					( change.x > 0 && letterPos.x <= pos.x) ||
					( change.x < 0 && letterPos.x >= pos.x) ||
					( change.y > 0 && letterPos.y <= pos.y) ||
					( change.y < 0 && letterPos.y >= pos.y))
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
			bool clicked = pointer->GetButtonReleaseCount(VK_LBUTTON) > 0;
			if (clicked || pointer->GetRelativePos() != ff::PointDouble::Zeros())
			{
				_letter = letterUnderPointer - &_letters[0];
			}

			if (clicked)
			{
				bExecute = true;
			}
		}

		for (Letter &letter : _letters)
		{
			if (letter._vk != VK_RETURN && keys->GetKeyPressCount(letter._vk))
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
				Stats &stats = Stats::Get(_mazesID);
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

void HighScoreScreen::Render(ff::I2dRenderer *pRenderer)
{
	DirectX::XMFLOAT4X4 matrix;
	DirectX::XMStoreFloat4x4(&matrix, DirectX::XMMatrixTranslation(0, -3 * PixelsPerTileF().y, 0));
	pRenderer->PushMatrix(ff::MATRIX_WORLD);
	pRenderer->TransformMatrix(ff::MATRIX_WORLD, &matrix);

	if (_render)
	{
		_render->RenderTheMaze(pRenderer);
		_render->RenderActors(pRenderer, true, false, false, this);
	}

	if (_text)
	{
		static DirectX::XMFLOAT4 s_colorWhite (1, 1, 1, 1);
		static DirectX::XMFLOAT4 s_colorPurple(1, 0, 1, 1);

		for (size_t i = 0; i < _letters.Size(); i++)
		{
			Letter &letter = _letters[i];

			_text->DrawText(
				pRenderer,
				letter._letter,
				letter._pos,
				0, &s_colorWhite, nullptr, nullptr);
		}

		if (_letter != ff::INVALID_SIZE)
		{
			Letter &letter = _letters[_letter];

			_text->DrawText(
				pRenderer,
				letter._letter,
				ff::PointFloat((float)(letter._pos.x - 1), (float)(letter._pos.y - 1)),
				0, &s_colorPurple, nullptr, nullptr);
		}

		_text->DrawText(
			pRenderer,
			_intro.c_str(),
			TileTopLeftToPixelF(ff::PointInt(4, 4)),
			0, &s_colorWhite, nullptr, nullptr);

		if (!_name.empty())
		{
			_text->DrawText(
				pRenderer,
				_name.c_str(),
				TileTopLeftToPixelF(ff::PointInt(9, 15)),
				0, &s_colorWhite, nullptr, nullptr);
		}

		size_t nMaxDashes = (_letter < s_nBackLetter && _showNewLetter) ? 8 : 9;

		if (_name.size() < nMaxDashes)
		{
			ff::String szDashes(nMaxDashes - _name.size(), '-');

			_text->DrawText(
				pRenderer,
				szDashes.c_str(),
				TileTopLeftToPixelF(ff::PointInt(9 + (9 - (int)nMaxDashes) + (int)_name.size(), 15)),
				0, &s_colorWhite, nullptr, nullptr);
		}

		if (_name.size() < 9 && _letter < s_nBackLetter && _showNewLetter)
		{
			_text->DrawText(
				pRenderer,
				_letters[_letter]._letter,
				TileTopLeftToPixelF(ff::PointInt(9 + (int)_name.size(), 15)),
				0, &s_colorPurple, nullptr, nullptr);
		}
	}

	pRenderer->PopMatrix(ff::MATRIX_WORLD);
}

IMazes *HighScoreScreen::GetMazes()
{
	return _mazes;
}

ff::PointInt HighScoreScreen::GetSizeInTiles() const
{
	if (_render && _render->GetMaze())
	{
		return _render->GetMaze()->GetSizeInTiles() + ff::PointInt(0, 5);
	}

	assertRetVal(false, ff::PointInt(16, 16));
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

IPlayer *HighScoreScreen::GetPlayer(size_t nPlayer)
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
	return ff::INVALID_SIZE;
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

const Stats &HighScoreScreen::GetStats() const
{
	static const Stats s_stats;
	return s_stats;
}

IMaze *HighScoreScreen::GetMaze() const
{
	return _render ? _render->GetMaze() : nullptr;
}

IRenderMaze *HighScoreScreen::GetRenderMaze()
{
	return _render;
}

const Difficulty &HighScoreScreen::GetDifficulty() const
{
	return GetEmptyDifficulty();
}

PacState HighScoreScreen::GetPacState() const
{
	return PAC_NORMAL;
}

IPlayingActor *HighScoreScreen::GetPac()
{
	return this;
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

ff::PointInt HighScoreScreen::GetGhostEyeDir(size_t nGhost) const
{
	return ff::PointInt(0, 0);
}

GhostState HighScoreScreen::GetGhostState(size_t nGhost) const
{
	return GHOST_INVALID;
}

IPlayingActor *HighScoreScreen::GetGhost(size_t nGhost)
{
	return nullptr;
}

ff::PointInt HighScoreScreen::GetGhostStartPixel() const
{
	return ff::PointInt(0, 0);
}

ff::PointInt HighScoreScreen::GetGhostStartTile() const
{
	return ff::PointInt(0, 0);
}

FruitState HighScoreScreen::GetFruitState() const
{
	return FRUIT_INVALID;
}

IPlayingActor *HighScoreScreen::GetFruit()
{
	return nullptr;
}

FruitType HighScoreScreen::GetFruitType() const
{
	return FRUIT_NONE;
}

ff::PointInt HighScoreScreen::GetFruitExitTile() const
{
	return ff::PointInt(0, 0);
}

const PointActor *HighScoreScreen::GetPointDisplays(size_t &nCount) const
{
	nCount = 0;
	return nullptr;
}

CustomActor * const *HighScoreScreen::GetCustomActors(size_t &nCount) const
{
	nCount = 0;
	return nullptr;
}

ff::PointInt HighScoreScreen::GetTile() const
{
	return PixelToTile(GetPixel());
}

ff::PointInt HighScoreScreen::GetPixel() const
{
	return (_letter >= 0 && _letter < _letters.Size())
		? _letters[_letter]._pos.ToInt() + PixelsPerTile() / 2
		: ff::PointInt::Zeros();
}

void HighScoreScreen::SetPixel(ff::PointInt pixel)
{
}

ff::PointInt HighScoreScreen::GetDir() const
{
	return ff::PointInt(1, 0);
}

void HighScoreScreen::SetDir(ff::PointInt dir)
{
}

ff::PointInt HighScoreScreen::GetPressDir() const
{
	return ff::PointInt(0, 0);
}

void HighScoreScreen::SetPressDir(ff::PointInt dir)
{
}

bool HighScoreScreen::IsActive() const
{
	return true;
}

void HighScoreScreen::SetActive(bool bActive)
{
}

