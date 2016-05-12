#pragma once

#include "Core/PlayingGame.h"
#include "Dict/Dict.h"
#include "Input/PointerDevice.h"
#include "Resource/ResourceValue.h"
#include "State\State.h"

namespace ff
{
	class IInputMapping;
	class ISprite;
}

namespace Maze
{
	ref class MainPage;
}

class IPlayingActor;

class PacApplication
	: public ff::State
	, public IPlayingGameHost
{
public:
	PacApplication();
	~PacApplication();

	static PacApplication *Get();
	ff::Dict &GetOptions();
	ff::RectFloat GetRenderRect() const;
	ff::RectFloat GetLevelRect() const;

	void SetMainPage(Maze::MainPage ^page);
	void SetInputEvent(ff::hash_t id);

	static ff::StaticString OPTION_PAC_DIFF;
	static ff::StaticString OPTION_PAC_MAZES;
	static ff::StaticString OPTION_PAC_PLAYERS;
	static ff::StaticString OPTION_SOUND_ON;
	static ff::StaticString OPTION_FULL_SCREEN;

	static const int DEFAULT_PAC_DIFF = 1;
	static const int DEFAULT_PAC_MAZES = 0;
	static const int DEFAULT_PAC_PLAYERS = 1;
	static const bool DEFAULT_SOUND_ON = true;
	static const bool DEFAULT_FULL_SCREEN = false;

	// State
	virtual std::shared_ptr<ff::State> Advance(ff::AppGlobals *context) override;
	virtual void Render(ff::AppGlobals *context, ff::IRenderTarget *target) override;
	virtual void SaveState(ff::AppGlobals *context) override;
	virtual void LoadState(ff::AppGlobals *context) override;

	// IPlayingGameHost
	bool IsShowingScoreBar(IPlayingGame *pGame) const;
	bool IsShowingStatusBar(IPlayingGame *pGame) const;
	void OnPlayerGameOver(IPlayingGame *pGame, IPlayer *pPlayer);

private:
	enum EAppState
	{
		APP_LOADING,
		APP_TITLE,
		APP_PLAYING_GAME,
		APP_HIGH_SCORE,
	};

	enum EButtons
	{
		BUTTON_NONE,
		BUTTON_PAUSE,
		BUTTON_HOME,
	};

	void HandleInputEvents(ff::AppGlobals *context);
	void HandlePressing(ff::AppGlobals *context, ff::IInputMapping *inputMap);
	ff::PointInt HandleTouchPress(ff::AppGlobals *context, IPlayingActor *pac);
	void RenderGame(ff::AppGlobals *context, ff::IRenderTarget *target, IPlayingGame *pGame);
	void RenderPacPressing(ff::AppGlobals *context, ff::I2dRenderer *render);
	void RenderDebugGrid(ff::AppGlobals *context, ff::I2dRenderer *render, ff::PointInt tiles);
	void SetState(EAppState state);
	IPlayingActor *GetCurrentPac() const;

	EAppState _state;
	ff::Dict _options;
	ff::ComPtr<IPlayingGame> _game;
	ff::ComPtr<IPlayingGame> _pushedGame;
	ff::TypedResource<ff::IInputMapping> _inputRes;
	ff::hash_t _pendingEvent;
	Maze::MainPage ^_mainPage;

	// Rendering
	ff::RectFloat _renderRect;
	ff::RectFloat _levelRect;
	float _fade;
	float _destFade;

	// Touch controls
	bool _touching;
	bool _pressDirFromTouch;
	double _touchLen;
	ff::TouchInfo _touchInfo;
	ff::PointDouble _touchStart;
	ff::PointDouble _touchOffset;
	ff::PointInt _touchStartPacDir;
	ff::TypedResource<ff::ISprite> _touchArrowSprite;
};
