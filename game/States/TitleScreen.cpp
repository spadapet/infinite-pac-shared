#include "pch.h"
#include "App.xaml.h"
#include "Core/Audio.h"
#include "Core/GlobalResources.h"
#include "Core/Helpers.h"
#include "Core/Maze.h"
#include "Core/Mazes.h"
#include "Core/PlayingGame.h"
#include "Core/RenderMaze.h"
#include "Core/RenderText.h"
#include "Core/Tiles.h"
#include "Globals/MetroGlobals.h"
#include "Graph/2D/2dRenderer.h"
#include "Graph/RenderTarget/RenderTarget.h"
#include "Input/InputDevice.h"
#include "Input/InputMapping.h"
#include "Input/Joystick/JoystickDevice.h"
#include "Input/Joystick/JoystickInput.h"
#include "Input/KeyboardDevice.h"
#include "Input/PointerDevice.h"
#include "States/PacApplication.h"
#include "TitleScreen.h"

TitleScreen::Option::Option()
{
}

TitleScreen::Option::Option(EOption type, ff::PointInt pos, GetTextFunc textFunc, int nMazes)
	: _type(type)
	, _selectedPos(pos)
	, _textFunc(textFunc)
	, _hoverOpacity(0)
	, _mazes(nMazes)
{
}

ff::String TitleScreen::Option::GetText() const
{
	return (_textFunc != nullptr) ? _textFunc() : ff::String(L"---");
}

ff::RectFloat TitleScreen::Option::GetLevelRect() const
{
	ff::String text = GetText();

	ff::PointFloat pos = _selectedPos.ToFloat();
	pos += PixelsPerTileF() * ff::PointFloat(1.5f, -0.5f);

	return ff::RectFloat(pos, pos + PixelsPerTileF() * ff::PointFloat((float)text.size(), 1));
}

ff::RectFloat TitleScreen::Option::GetTargetRect() const
{
	ff::RectFloat renderRect = PacApplication::Get()->GetRenderRect();
	ff::RectFloat levelRect = PacApplication::Get()->GetLevelRect();

	if (renderRect.IsEmpty() || levelRect.IsEmpty())
	{
		return ff::RectFloat::Zeros();
	}

	ff::PointFloat scale(renderRect.Width() / levelRect.Width(), renderRect.Height() / levelRect.Height());
	ff::RectFloat targetRect = GetLevelRect() * scale + renderRect.TopLeft();
	return targetRect;
}

BEGIN_INTERFACES(TitleScreen)
	HAS_INTERFACE(IPlayingGame)
	HAS_INTERFACE(IPlayingMaze)
END_INTERFACES()

TitleScreen::TitleScreen()
	: _curOption(0)
	, _done(false)
	, _fading(true)
	, _fade(1)
	, _inputRes(GetGlobalInputMapping())
{
}

TitleScreen::~TitleScreen()
{
}

bool TitleScreen::Init()
{
	assertRetVal(CreateFrontMaze(), false);
	assertRetVal(CreateBackMaze(), false);
	assertRetVal(ISoundEffects::Create(CHAR_DEFAULT, &_sounds), false);

	CreateOptions();
	SelectDefaultOption();
	UpdateHighScores();

	return true;
}

void TitleScreen::SelectDefaultOption()
{
	const ff::Dict &options = PacApplication::Get()->GetOptions();
	int defaultMazes = options.GetInt(PacApplication::OPTION_PAC_MAZES, PacApplication::DEFAULT_PAC_MAZES);
	noAssertRet(defaultMazes != -1);

	for (size_t i = 0; i < _options.Size(); i++)
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

	ff::Dict &options = PacApplication::Get()->GetOptions();
	options.SetInt(PacApplication::OPTION_PAC_MAZES, _options[_curOption]._mazes);

	if (GetMazesID().size())
	{
		const Stats &stats = Stats::Get(GetMazesID());

		for (size_t i = 0; i < GetHighScoreDisplayCount() && i < _countof(stats._highScores); i++)
		{
			_scores += (i > 0) ? L"\n" : L"";
			_scores += FormatScoreAsString(stats._highScores[i]._score);
			_scores += L"  ";
			_scores += FormatHighScoreName(stats._highScores[i]._name);
		}
	}
}

void TitleScreen::CreateOptions()
{
	int lineHeight = (int)GetLineHeight();
	ff::PointInt optionPos = TileBottomRightToPixel(ff::PointInt(5, 3)) + ff::PointInt(PixelsPerTile().x / -2, 0);
	const ff::Dict *options = &PacApplication::Get()->GetOptions();

	auto mrPacText = []() { return ff::String(L"MR.PAC"); };
	auto msPacText = []() { return ff::String(L"MS.LILA"); };
	auto aboutText = []() { return ff::String(L"ABOUT"); };

	auto playersText = [options]()
	{
		return (options->GetInt(PacApplication::OPTION_PAC_PLAYERS, PacApplication::DEFAULT_PAC_PLAYERS) == 1)
			? ff::String(L"PLAYERS:ONE")
			: ff::String(L"PLAYERS:TWO");
	};

	auto diffText = [options]()
	{
		switch (options->GetInt(PacApplication::OPTION_PAC_DIFF, PacApplication::DEFAULT_PAC_DIFF))
		{
		case 0: return ff::String(L"DIFFICULTY:EASY");
		case 1: default: return ff::String(L"DIFFICULTY:NORMAL");
		case 2: return ff::String(L"DIFFICULTY:HARD");
		}
	};

	auto soundText = [options]()
	{
		return options->GetBool(PacApplication::OPTION_SOUND_ON, PacApplication::DEFAULT_SOUND_ON)
			? ff::String(L"SOUND:ON")
			: ff::String(L"SOUND:OFF");
	};

	auto fullScreenText = []()
	{
		ff::IRenderTargetWindow *target = ff::MetroGlobals::Get()->GetTarget();
		return target && target->IsFullScreen()
			? ff::String(L"FULL SCREEN:ON")
			: ff::String(L"FULL SCREEN:OFF");
	};

	_options.Push(Option(OPT_PAC, optionPos, mrPacText, 0));
	optionPos.y += lineHeight;

	_options.Push(Option(OPT_MSPAC, optionPos, msPacText, 1));
	optionPos.y += lineHeight * 2;

	_options.Push(Option(OPT_PLAYERS, optionPos, playersText));
	optionPos.y += lineHeight;

	_options.Push(Option(OPT_DIFF, optionPos, diffText));
	optionPos.y += lineHeight;

	_options.Push(Option(OPT_SOUND, optionPos, soundText));
	optionPos.y += lineHeight;

	_options.Push(Option(OPT_FULL_SCREEN, optionPos, fullScreenText));
	optionPos.y += lineHeight;

	_options.Push(Option(OPT_ABOUT, optionPos, aboutText));
	optionPos.y += lineHeight;
}

bool TitleScreen::CreateFrontMaze()
{
	_mazes = nullptr;
	_render = nullptr;
	_text = nullptr;

	assertRetVal(IMazes::Create(&_mazes), false);

	ff::ComPtr<IMaze> pMaze;
	assertRetVal(CreateMazeFromResource(ff::String(L"title-maze-front"), &pMaze), false);
	assertRetVal(IRenderMaze::Create(pMaze, &_render), false);
	assertRetVal(IRenderText::Create(&_text), false);

	_mazes->AddMaze(0, pMaze);

	return true;
}

bool TitleScreen::CreateBackMaze()
{
	ff::ComPtr<IMaze> pBackMaze;
	assertRetVal(CreateMazeFromResource(ff::String(L"title-maze-back"), &pBackMaze), false);
	pBackMaze->SetCharType(CHAR_MS); // to get random ghost scattering

	_backMaze = nullptr;
	assertRetVal(IPlayingMaze::Create(pBackMaze, GetEmptyDifficulty(), this, &_backMaze), false);
	pBackMaze = _backMaze->GetMaze();

	// Remove all dots from the maze
	ff::PointInt mazeSize = pBackMaze->GetSizeInTiles();

	for (ff::PointInt tile(0, 0); tile.y < mazeSize.y; tile.y++, tile.x = 0)
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
	ff::Dict &appOptions = PacApplication::Get()->GetOptions();

	switch (option)
	{
	case OPT_PAC:
	case OPT_MSPAC:
		_curOption = (option == OPT_MSPAC) ? 1 : 0;
		appOptions.SetInt(PacApplication::OPTION_PAC_MAZES, _options[_curOption]._mazes);
		_done = !GetMazesID().empty();
		break;

	case OPT_PLAYERS:
		{
			playEffect = true;
			int value = appOptions.GetInt(PacApplication::OPTION_PAC_PLAYERS, PacApplication::DEFAULT_PAC_PLAYERS);
			appOptions.SetInt(PacApplication::OPTION_PAC_PLAYERS, (value == 1) ? 2 : 1);
		}
		break;

	case OPT_DIFF:
		{
			playEffect = true;
			int value = appOptions.GetInt(PacApplication::OPTION_PAC_DIFF, PacApplication::DEFAULT_PAC_DIFF);
			appOptions.SetInt(PacApplication::OPTION_PAC_DIFF, (value + (pressedLeft ? 2 : 1)) % 3);
		}
		break;

	case OPT_SOUND:
		{
			playEffect = true;
			bool value = appOptions.GetBool(PacApplication::OPTION_SOUND_ON, PacApplication::DEFAULT_SOUND_ON);
			appOptions.SetBool(PacApplication::OPTION_SOUND_ON, !value);
		}
		break;

	case OPT_FULL_SCREEN:
		{
			playEffect = true;
			bool value = appOptions.GetBool(PacApplication::OPTION_FULL_SCREEN, PacApplication::DEFAULT_FULL_SCREEN);
			appOptions.SetBool(PacApplication::OPTION_FULL_SCREEN, !value);

			ff::IRenderTargetWindow *target = ff::MetroGlobals::Get()->GetTarget();
			if (target && target->CanSetFullScreen())
			{
				bool fullScreen = appOptions.GetBool(PacApplication::OPTION_FULL_SCREEN, PacApplication::DEFAULT_FULL_SCREEN);
				ff::MetroGlobals::Get()->GetTarget()->SetFullScreen(fullScreen);
			}
		}
		break;

	case OPT_ABOUT:
		playEffect = true;
		Maze::App::Current->ShowAboutPage();
		break;
	}

	if (playEffect)
	{
		_sounds->Play(EFFECT_FRUIT_BOUNCE);
	}
}

TitleScreen::Option *TitleScreen::HitTestOption(ff::PointFloat pos)
{
	ff::RectFloat renderRect = PacApplication::Get()->GetRenderRect();
	ff::RectFloat levelRect = PacApplication::Get()->GetLevelRect();
	noAssertRetVal(!renderRect.IsEmpty() && !levelRect.IsEmpty(), nullptr);

	ff::PointFloat scale(renderRect.Width() / levelRect.Width(), renderRect.Height() / levelRect.Height());
	ff::PointFloat slop = PixelsPerTileF() * scale / -4.0f;

	for (Option &option : _options)
	{
		ff::RectFloat rect = option.GetTargetRect();
		rect.Deflate(slop);

		if (rect.PointInRect(pos))
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
ff::String TitleScreen::GetMazesID()
{
	const ff::Dict &options = PacApplication::Get()->GetOptions();
	int mazes = options.GetInt(PacApplication::OPTION_PAC_MAZES, PacApplication::DEFAULT_PAC_MAZES);
	int diff = options.GetInt(PacApplication::OPTION_PAC_DIFF, PacApplication::DEFAULT_PAC_DIFF);

	ff::String idString;
	switch (mazes)
	{
	case 0: idString = L"mr-mazes"; break;
	case 1: idString = L"ms-mazes"; break;
	default: return idString;
	}

	ff::String diffString;
	switch (diff)
	{
	case 0: diffString = L"-easy"; break;
	case 1: diffString = L"-normal"; break;
	case 2: diffString = L"-hard"; break;
	default: return diffString;
	}

	return idString + diffString;
}

void TitleScreen::Advance()
{
	ff::IInputMapping *inputMap = _inputRes.GetObject();
	ff::IPointerDevice *pointer = ff::MetroGlobals::Get()->GetPointer();
	ff::PointFloat pointerPos = pointer->GetPos().ToFloat();
	Option *optionUnderPointer = HitTestOption(pointerPos);

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
					nChangeOption = -1;
				}
				else if (ie._eventID == GetEventDown())
				{
					nChangeOption = 1;
				}
				else if (ie._eventID == GetEventLeft() || ie._eventID == GetEventRight())
				{
					switch (_options[_curOption]._type)
					{
					case OPT_PLAYERS:
					case OPT_DIFF:
					case OPT_SOUND:
					case OPT_FULL_SCREEN:
						bExecute = true;
						bLeft = (ie._eventID == GetEventLeft());
						break;
					}
					break;
				}
				else if (ie._eventID == GetEventAction() || ie._eventID == GetEventStart())
				{
					bExecute = true;
				}
			}
		}
	}

	if (pointer->GetButtonReleaseCount(VK_LBUTTON) && optionUnderPointer)
	{
		Execute(optionUnderPointer->_type, false);
	}

	if (nChangeOption)
	{
		bool playEffect = false;

		if (nChangeOption > 0 && _curOption + 1 < _options.Size())
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

	if (pointer->GetRelativePos() != ff::PointDouble::Zeros() && optionUnderPointer)
	{
		optionUnderPointer->_hoverOpacity = 1;
	}

	for (Option &option : _options)
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

void TitleScreen::Render(ff::I2dRenderer *pRenderer)
{
	RenderBackMaze(pRenderer);
	RenderFrontMaze(pRenderer);
	RenderOptions(pRenderer);
	RenderFade(pRenderer);
}

void TitleScreen::RenderBackMaze(ff::I2dRenderer *render)
{
	noAssertRet(_backMaze && render);

	_backMaze->Render(render);

	static const DirectX::XMFLOAT4 s_color(0, 0, 0, 0.75f);
	ff::PointInt tiles = GetSizeInTiles();

	render->DrawFilledRectangle(
		&ff::RectFloat(0, 0, tiles.x * PixelsPerTileF().x, tiles.y * PixelsPerTileF().y),
		&s_color, 1);

}

void TitleScreen::RenderFrontMaze(ff::I2dRenderer *render)
{
	noAssertRet(render);

	_render->RenderTheMaze(render);
	_render->RenderActors(render, true, false, false, this);
}

void TitleScreen::RenderOptions(ff::I2dRenderer *render)
{
	noAssertRet(_text && render);

	static DirectX::XMFLOAT4 s_textColor(1, 1, 1, 1);
	static DirectX::XMFLOAT4 s_titleColor(0.75, 0, 0, 1);
	float lineHeight = GetLineHeight();

	for (const Option &option : _options)
	{
		ff::String text = option.GetText();
		ff::PointFloat pos = option.GetLevelRect().TopLeft();

		_text->DrawText(render, text.c_str(), pos, lineHeight, &s_textColor, nullptr, nullptr);

		if (option._hoverOpacity > 0)
		{
			DirectX::XMFLOAT4 hoverColor(0.349f, 0.486f, 0.812f, option._hoverOpacity);
			_text->DrawText(render, text.c_str(), pos, lineHeight, &hoverColor, nullptr, nullptr);
		}
	}

	ff::String szTitles(L" START GAME:\n\n\n\n\n\n\n\n\n\n");
	if (_scores.size())
	{
		szTitles += L"HIGH SCORES:";
	}

	_text->DrawText(
		render,
		szTitles.c_str(),
		TileTopLeftToPixelF(ff::PointInt(6, 2)),
		lineHeight,
		&s_titleColor,
		nullptr, nullptr);

	_text->DrawText(
		render,
		_scores.c_str(),
		TileTopLeftToPixelF(ff::PointInt(6, 19)),
		0, &s_textColor,
		nullptr, nullptr);
}

void TitleScreen::RenderFade(ff::I2dRenderer *render)
{
	noAssertRet(_fading);

	_fade += _done ? 0.015625f : -0.015625f;

	_fade = std::max<float>(0, _fade);
	_fade = std::min<float>(1, _fade);

	if (_fade == 0 || _fade == 1)
	{
		_fading = false;
	}

	if (render && _fade > 0)
	{
		DirectX::XMFLOAT4 color(0, 0, 0, _fade);
		ff::PointInt tiles = GetSizeInTiles();

		render->DrawFilledRectangle(
			&ff::RectFloat(0, 0, tiles.x * PixelsPerTileF().x, tiles.y * PixelsPerTileF().y),
			&color, 1);
	}
}

IMazes *TitleScreen::GetMazes()
{
	return _mazes;
}

ff::PointInt TitleScreen::GetSizeInTiles() const
{
	if (_render && _render->GetMaze())
	{
		return _render->GetMaze()->GetSizeInTiles();
	}

	assertRetVal(false, ff::PointInt(16, 16));
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

IPlayer *TitleScreen::GetPlayer(size_t nPlayer)
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
	return ff::INVALID_SIZE;
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

const Stats &TitleScreen::GetStats() const
{
	return _stats;
}

IMaze *TitleScreen::GetMaze() const
{
	return _render ? _render->GetMaze() : nullptr;
}

IRenderMaze *TitleScreen::GetRenderMaze()
{
	return _render;
}

const Difficulty &TitleScreen::GetDifficulty() const
{
	return GetEmptyDifficulty();
}

PacState TitleScreen::GetPacState() const
{
	return PAC_NORMAL;
}

IPlayingActor *TitleScreen::GetPac()
{
	return this;
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

ff::PointInt TitleScreen::GetGhostEyeDir(size_t nGhost) const
{
	return ff::PointInt(0, 0);
}

GhostState TitleScreen::GetGhostState(size_t nGhost) const
{
	return GHOST_INVALID;
}

IPlayingActor *TitleScreen::GetGhost(size_t nGhost)
{
	return nullptr;
}

ff::PointInt TitleScreen::GetGhostStartPixel() const
{
	return ff::PointInt(0, 0);
}

ff::PointInt TitleScreen::GetGhostStartTile() const
{
	return ff::PointInt(0, 0);
}

FruitState TitleScreen::GetFruitState() const
{
	return FRUIT_INVALID;
}

IPlayingActor *TitleScreen::GetFruit()
{
	return nullptr;
}

FruitType TitleScreen::GetFruitType() const
{
	return FRUIT_NONE;
}

ff::PointInt TitleScreen::GetFruitExitTile() const
{
	return ff::PointInt(0, 0);
}

const PointActor *TitleScreen::GetPointDisplays(size_t &nCount) const
{
	nCount = 0;
	return nullptr;
}

CustomActor * const *TitleScreen::GetCustomActors(size_t &nCount) const
{
	nCount = 0;
	return nullptr;
}

ff::PointInt TitleScreen::GetTile() const
{
	return PixelToTile(GetPixel());
}

ff::PointInt TitleScreen::GetPixel() const
{
	return _options[_curOption]._selectedPos;
}

void TitleScreen::SetPixel(ff::PointInt pixel)
{
}

ff::PointInt TitleScreen::GetDir() const
{
	return ff::PointInt(1, 0);
}

void TitleScreen::SetDir(ff::PointInt dir)
{
}

ff::PointInt TitleScreen::GetPressDir() const
{
	return ff::PointInt(0, 0);
}

void TitleScreen::SetPressDir(ff::PointInt dir)
{
}

bool TitleScreen::IsActive() const
{
	return true;
}

void TitleScreen::SetActive(bool bActive)
{
}
