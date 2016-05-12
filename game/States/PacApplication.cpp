#include "pch.h"
#include "App.xaml.h"
#include "Audio/AudioDevice.h"
#include "COM/ComObject.h"
#include "Core/GlobalResources.h"
#include "Core/Helpers.h"
#include "Core/Mazes.h"
#include "Core/Stats.h"
#include "Globals/AppGlobals.h"
#include "Globals/MetroGlobals.h"
#include "Graph/Anim/AnimPos.h"
#include "Graph/2D/2dRenderer.h"
#include "Graph/2D/Sprite.h"
#include "Graph/RenderTarget/RenderTarget.h"
#include "Input/InputMapping.h"
#include "Input/Joystick/JoystickDevice.h"
#include "Input/Joystick/JoystickInput.h"
#include "Input/KeyboardDevice.h"
#include "MainPage.xaml.h"
#include "Module/Module.h"
#include "States/HighScoreScreen.h"
#include "States/PacApplication.h"
#include "States/TitleScreen.h"
#include "Types/Timer.h"

// static
PacApplication *s_pacApp = nullptr;
ff::StaticString PacApplication::OPTION_PAC_DIFF(L"OPTION_PAC_DIFFICULTY");
ff::StaticString PacApplication::OPTION_PAC_MAZES(L"OPTION_PAC_MAZES");
ff::StaticString PacApplication::OPTION_PAC_PLAYERS(L"OPTION_PAC_PLAYERS");
ff::StaticString PacApplication::OPTION_SOUND_ON(L"OPTION_SOUND_ON");
ff::StaticString PacApplication::OPTION_FULL_SCREEN(L"OPTION_FULL_SCREEN");

static const double TOUCH_DEAD_ZONE = 20;

PacApplication::PacApplication()
	: _state(APP_LOADING)
	, _pendingEvent(0)
	, _fade(0)
	, _destFade(0)
	, _touching(false)
	, _pressDirFromTouch(false)
	, _touchLen(0)
	, _touchOffset(0, 0)
	, _inputRes(GetGlobalInputMapping())
	, _renderRect(ff::RectFloat::Zeros())
	, _levelRect(ff::RectFloat::Zeros())
{
	ff::ZeroObject(_touchInfo);
	_touchArrowSprite.Init(L"char-sprites.move-arrow");

	assert(!s_pacApp);
	s_pacApp = this;
}

PacApplication::~PacApplication()
{
	assert(s_pacApp == this);
	s_pacApp = nullptr;
}

PacApplication *PacApplication::Get()
{
	assert(s_pacApp);
	return s_pacApp;
}

ff::Dict &PacApplication::GetOptions()
{
	return _options;
}

ff::RectFloat PacApplication::GetRenderRect() const
{
	return _renderRect;
}

ff::RectFloat PacApplication::GetLevelRect() const
{
	return _levelRect;
}

void PacApplication::SetMainPage(Maze::MainPage ^page)
{
	_mainPage = page;
}

void PacApplication::SetInputEvent(ff::hash_t id)
{
	::InterlockedExchange(&_pendingEvent, id);
}

std::shared_ptr<ff::State> PacApplication::Advance(ff::AppGlobals *context)
{
	noAssertRetVal(!Maze::App::Current->IsShowingPopup(), nullptr);
	Maze::MainPage::State mainPageState = Maze::MainPage::State::None;

	switch (_state)
	{
	case APP_LOADING:
		SetState(APP_TITLE);
		break;

	case APP_TITLE:
	{
		ff::ComPtr<TitleScreen, IPlayingGame> pTitle;
		if (_game && _game->IsGameOver() && pTitle.QueryFrom(_game))
		{
			// Start playing the selected game
			_game = nullptr;
			SetState(APP_PLAYING_GAME);
		}
	}
	break;

	case APP_PLAYING_GAME:
		if (_game)
		{
			if (_game->IsGameOver())
			{
				// Back to the title screen
				SetState(APP_LOADING);
			}
			else if (_game->IsPaused())
			{
				mainPageState = Maze::MainPage::State::Paused;
			}
			else
			{
				mainPageState = Maze::MainPage::State::Playing;
			}
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

				// In case the game crashes, save high scores now
				ff::MetroGlobals::Get()->SaveState();
			}
			else
			{
				// Fade back to the game
				_destFade = 0;
			}
		}
		break;

	default:
		assertSz(false, L"Unknown game state");
		break;
	}

	HandleInputEvents(context);

	if (_game && !_game->IsPaused())
	{
		_game->Advance();
	}

	if (_mainPage)
	{
		_mainPage->SetState(mainPageState);
	}

	return nullptr;
}

void PacApplication::Render(ff::AppGlobals *context, ff::IRenderTarget *target)
{
	noAssertRet(!Maze::App::Current->IsShowingPopup());

	if (_pushedGame)
	{
		RenderGame(context, target, _pushedGame);
		context->GetDepth()->Clear();

		ff::RectFloat rect(target->GetRotatedSize().ToFloat());
		ff::I2dRenderer *render = context->Get2dRender();

		if (_fade < 1 && render->BeginRender(
			target, context->GetDepth(), rect, ff::PointFloat(0, 0), ff::PointFloat(1, 1), context->Get2dEffect()))
		{
			DirectX::XMFLOAT4 colorFade(0, 0, 0, _fade);

			render->DrawFilledRectangle(
				&ff::RectFloat((float)rect.left, (float)rect.top, (float)rect.right, (float)rect.bottom),
				&colorFade, 1);

			render->EndRender();
			context->GetDepth()->Clear();
		}
	}

	if (!_pushedGame || _fade == _destFade)
	{
		RenderGame(context, target, _game);
	}
}

void PacApplication::SaveState(ff::AppGlobals *context)
{
	Stats::Save();

	if (context->GetTarget())
	{
		bool fullScreen = context->GetTarget()->IsFullScreen();
		_options.SetBool(OPTION_FULL_SCREEN, fullScreen);
	}

	ff::MetroGlobals::Get()->SetState(_options);
}

void PacApplication::LoadState(ff::AppGlobals *context)
{
	_options = ff::MetroGlobals::Get()->GetState();
	Stats::Load();
}

bool PacApplication::IsShowingScoreBar(IPlayingGame *pGame) const
{
	return true;
}

bool PacApplication::IsShowingStatusBar(IPlayingGame *pGame) const
{
	return true;
}

void PacApplication::OnPlayerGameOver(IPlayingGame *pGame, IPlayer *pPlayer)
{
	assertRet(pPlayer);

	if (_state == APP_PLAYING_GAME && pGame->GetMazes() && !pPlayer->DidCheat())
	{
		ff::String mazesID = pGame->GetMazes()->GetID();
		Stats &stats = Stats::Get(mazesID);

		pPlayer->AddStats(stats);

		if (stats.GetHighScoreSlot(pPlayer->GetScore()) != ff::INVALID_SIZE)
		{
			ff::ComPtr<HighScoreScreen, IPlayingGame> pGame = new ff::ComObject<HighScoreScreen>();

			if (pGame->Init(mazesID, pPlayer))
			{
				_pushedGame = _game;
				_game = pGame;
				_destFade = 0.875f;

				SetState(APP_HIGH_SCORE);
			}
		}
	}
}

void PacApplication::HandleInputEvents(ff::AppGlobals *context)
{
	ff::IInputMapping *inputMap = _inputRes.GetObject();
	noAssertRet(inputMap);
	inputMap->Advance(context->GetGlobalTime()._secondsPerAdvance);

	bool unpause = false;
	ff::Vector<ff::InputEvent> events = inputMap->GetEvents();

	ff::hash_t pendingEvent = ::InterlockedExchange(&_pendingEvent, 0);
	if (pendingEvent)
	{
		events.Push(ff::InputEvent{ pendingEvent, 1 });
		events.Push(ff::InputEvent{ pendingEvent, 0 });
	}

	for (const ff::InputEvent &ie : events)
	{
		if (ie.IsStart())
		{
			if (ie._eventID == GetEventPause() || ie._eventID == GetEventStart())
			{
				if (_game && !_game->IsPaused())
				{
					_game->TogglePaused();
					context->GetAudio()->PauseEffects();
				}
				else
				{
					unpause = true;
				}
			}
			else if (ie._eventID == GetEventPauseAdvance())
			{
				if (ff::GetThisModule().IsDebugBuild() && _game && _game->IsPaused())
				{
					_game->PausedAdvance();
				}
			}
			else if (ie._eventID == GetEventCancel() && _state == APP_PLAYING_GAME)
			{
				if (_game && _game->IsPaused())
				{
					unpause = true;
				}
				else
				{
					SetState(APP_LOADING);
				}
			}
			else if (ie._eventID == GetEventHome() && _state == APP_PLAYING_GAME)
			{
				SetState(APP_LOADING);
			}
		}
	}

	if (unpause && _game && _game->IsPaused())
	{
		_game->TogglePaused();
		context->GetAudio()->ResumeEffects();
	}

	HandlePressing(context, inputMap);
}

void PacApplication::HandlePressing(ff::AppGlobals *context, ff::IInputMapping *inputMap)
{
	ff::PointInt pressDir(0, 0);

	if (inputMap->GetDigitalValue(GetEventUp()))
	{
		pressDir.y = -1;
	}
	else if (inputMap->GetDigitalValue(GetEventDown()))
	{
		pressDir.y = 1;
	}

	if (inputMap->GetDigitalValue(GetEventLeft()))
	{
		pressDir.x = -1;
	}
	else if (inputMap->GetDigitalValue(GetEventRight()))
	{
		pressDir.x = 1;
	}

	if (_game && _game->GetPlayers())
	{
		// Tell the game about what directions the user is pressing

		IPlayingActor *pac = GetCurrentPac();
		if (pac && pressDir.IsNull())
		{
			pressDir = HandleTouchPress(context, pac);

			if (_touching)
			{
				pac->SetPressDir(pressDir);
				_pressDirFromTouch = true;
			}
			else if (_pressDirFromTouch)
			{
				ff::PointInt dir = pac->GetDir();
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

ff::PointInt PacApplication::HandleTouchPress(ff::AppGlobals *context, IPlayingActor *pac)
{
	ff::PointInt pressDir(0, 0);
	ff::IPointerDevice *pointer = context->GetPointer();

	if (pointer->GetTouchCount())
	{
		const double scale = ff::MetroGlobals::Get()->PixelToDip(1);

		_touching = true;
		_touchInfo = pointer->GetTouchInfo(0);
		_touchOffset = _touchInfo.pos - _touchInfo.startPos;
		_touchLen = _touchOffset.Length() * scale;

		if (_touchLen >= TOUCH_DEAD_ZONE)
		{
			double angle = std::atan2(_touchOffset.y, _touchOffset.x);
			int slice = (int)(angle * 8.0 / ff::PI_D);

			switch (slice)
			{
			default: assertSz(false, ff::String::format_new(L"Bad slice:%d, angle:%.4f, len:%.4f", slice, angle, _touchLen).c_str()); break;
			case  0:          pressDir.SetPoint( 1,  0); break;
			case  1: case  2: pressDir.SetPoint( 1,  1); break;
			case  3: case  4: pressDir.SetPoint( 0,  1); break;
			case  5: case  6: pressDir.SetPoint(-1,  1); break;
			case  7: case -7: pressDir.SetPoint(-1,  0); break;
			case  8: case -8: pressDir.SetPoint(-1,  0); break;
			case -6: case -5: pressDir.SetPoint(-1, -1); break;
			case -4: case -3: pressDir.SetPoint( 0, -1); break;
			case -2: case -1: pressDir.SetPoint( 1, -1); break;
			}
		}
	}
	else
	{
		_touching = false;
	}

	return pressDir;
}

void PacApplication::RenderGame(ff::AppGlobals *context, ff::IRenderTarget *target, IPlayingGame *pGame)
{
	noAssertRet(pGame);

	ff::RectInt renderRect(target->GetRotatedSize());
	ff::RectInt clientRect(target->GetRotatedSize());
	ff::RectInt visibleRect = clientRect;
	int padding1 = (int)ff::MetroGlobals::Get()->DipToPixel(PixelsPerTileF().y);
	ff::RectInt padding(padding1, padding1, padding1, padding1);

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

	ff::PointInt renderSize = renderRect.Size();
	ff::PointInt clientSize = clientRect.Size();
	ff::PointInt visibleSize = visibleRect.Size();

	if (pGame &&
		renderSize.x >= 64 &&
		renderSize.y >= 64 &&
		visibleSize.x >= 64 &&
		visibleSize.y >= 64)
	{
		ff::PointInt tiles = pGame->GetSizeInTiles();
		ff::PointInt tileSize = PixelsPerTile();

		ff::PointInt playPixelSize(tiles.x * tileSize.x, tiles.y * tileSize.y);
		ff::RectInt playRenderRect(0, 0, playPixelSize.x, playPixelSize.y);

		_levelRect = playRenderRect.ToFloat();

		playRenderRect.ScaleToFit(visibleRect);
		playRenderRect.CenterWithin(visibleRect);

		playRenderRect.left = playRenderRect.left * renderSize.x / clientSize.x;
		playRenderRect.right = playRenderRect.right * renderSize.x / clientSize.x;
		playRenderRect.top = playRenderRect.top * renderSize.y / clientSize.y;
		playRenderRect.bottom = playRenderRect.bottom * renderSize.y / clientSize.y;

		ff::PointFloat viewScale(
			(float)playRenderRect.Width() / playPixelSize.x,
			(float)playRenderRect.Height() / playPixelSize.y);

		_renderRect = playRenderRect.ToFloat();

		ff::I2dRenderer *render = context->Get2dRender();
		if (render->BeginRender(
			target,
			context->GetDepth(),
			_renderRect,
			ff::PointFloat::Zeros(),
			viewScale,
			context->Get2dEffect()))
		{
			pGame->Render(render);
			RenderPacPressing(context, render);
			RenderDebugGrid(context, render, tiles);

			render->EndRender();
		}
	}
}

void PacApplication::RenderPacPressing(ff::AppGlobals *context, ff::I2dRenderer *render)
{
	noAssertRet(_touchArrowSprite.GetObject());

	IPlayingActor *pac = GetCurrentPac();
	noAssertRet(pac);
	
	ff::PointInt pressDir = pac->GetPressDir();
	noAssertRet(!pressDir.IsNull());

	float rotation = 0;
	float scale = 0;
	float opacity = 0;

	if (_touching)
	{
		if (!_touchOffset.IsNull())
		{
			rotation = (float)std::atan2(-_touchOffset.y, _touchOffset.x);
			scale = (float)std::min(4.0, (_touchLen - TOUCH_DEAD_ZONE) / 80.0 + 0.75);
			opacity = (float)ff::Clamp(1.25 / scale, 0.25, 1.0);
		}
	}
	else
	{
		rotation = (float)std::atan2(-pressDir.y, pressDir.x);
		scale = 1.25;
		opacity = 0.5;
	}

	if (opacity != 0 && scale != 0)
	{
		ff::PointFloat arrowPos = pac->GetPixel().ToFloat() + PixelsPerTileF() * ff::PointFloat(0, 3);

		render->DrawSprite(
			_touchArrowSprite.GetObject(),
			&arrowPos,
			&ff::PointFloat(scale, scale),
			rotation,
			&DirectX::XMFLOAT4(1, 1, 1, opacity));
	}
}

void PacApplication::RenderDebugGrid(ff::AppGlobals *context, ff::I2dRenderer *render, ff::PointInt tiles)
{
#ifdef _DEBUG
	if (context->GetKeys()->GetKey('G'))
	{
		ff::PointFloat tileSizeF = PixelsPerTileF();

		for (int i = 0; i < tiles.x; i++)
		{
			render->DrawLine(
				&ff::PointFloat(i * tileSizeF.x, 0),
				&ff::PointFloat(i * tileSizeF.x, tiles.y * tileSizeF.y),
				&DirectX::XMFLOAT4(1, 1, 1, 0.25f));
		}

		for (int i = 0; i < tiles.y; i++)
		{
			render->DrawLine(
				&ff::PointFloat(0, i * tileSizeF.y),
				&ff::PointFloat(tiles.x * tileSizeF.x, i * tileSizeF.y),
				&DirectX::XMFLOAT4(1, 1, 1, 0.25f));
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
			ff::ComPtr<TitleScreen, IPlayingGame> pTitle = new ff::ComObject<TitleScreen>;
			verify(pTitle->Init());
			_game = pTitle;
			_state = APP_TITLE;
		}
		break;

	case APP_PLAYING_GAME:
		{
			if (!_game)
			{
				ff::ComPtr<IMazes> pMazes;

				if (CreateMazesFromId(TitleScreen::GetMazesID(), &pMazes))
				{
					ff::ComPtr<IPlayingGame> pGame;
					int players =_options.GetInt(OPTION_PAC_PLAYERS, DEFAULT_PAC_PLAYERS);
					if (IPlayingGame::Create(pMazes, players, this, &pGame))
					{
						_game = pGame;
					}
				}
			}

			if (_game)
			{
				_state = APP_PLAYING_GAME;
			}
		}
		break;

	case APP_LOADING:
		{
			_game = nullptr;
			_state = APP_LOADING;
		}
		break;

	case APP_HIGH_SCORE:
		{
			assert(_game && _pushedGame);
			_state = APP_HIGH_SCORE;
		}
		break;
	}

	assert(_state == state);
}

IPlayingActor *PacApplication::GetCurrentPac() const
{
	IPlayer *player = _game ? _game->GetPlayer(_game->GetCurrentPlayer()) : nullptr;
	IPlayingMaze *playMaze = player ? player->GetPlayingMaze() : nullptr;
	IPlayingActor *pac = playMaze ? playMaze->GetPac() : nullptr;

	return pac;
}
