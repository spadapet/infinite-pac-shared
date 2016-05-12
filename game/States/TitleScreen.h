#pragma once

#include "Core/Actors.h"
#include "Core/PlayingGame.h"
#include "Core/PlayingMaze.h"
#include "Resource/ResourceValue.h"

namespace ff
{
	class MetroGlobals;
	class IInputMapping;
}

class PacApplication;
class IRenderMaze;
class IRenderText;
class ISoundEffects;

class __declspec(uuid("eb214644-fafc-41d1-9d39-b27b0ff425a5"))
	TitleScreen
		: public ff::ComBase
		, public IPlayingGame
		, public IPlayingMazeHost
		, public IPlayingMaze
		, public IPlayingActor
{
public:
	DECLARE_HEADER(TitleScreen);

	bool Init();

	static size_t GetHighScoreDisplayCount();
	static ff::String GetMazesID();
	static CharType GetCharTypeOption();

	// IPlayingGame

	virtual void Advance() override;
	virtual void Render(ff::I2dRenderer *pRenderer) override;

	virtual IMazes *GetMazes() override;
	virtual ff::PointInt GetSizeInTiles() const override;
	virtual size_t GetHighScore() const override;

	virtual size_t GetPlayers() const override;
	virtual size_t GetCurrentPlayer() const override;
	virtual IPlayer *GetPlayer(size_t nPlayer) override;

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
	virtual const Stats &GetStats() const override;
	virtual IMaze *GetMaze() const override;
	virtual IRenderMaze *GetRenderMaze() override;
	virtual const Difficulty &GetDifficulty() const override;

	virtual PacState GetPacState() const override;
	virtual IPlayingActor *GetPac() override;
	virtual CharType GetCharType() const override;
	virtual bool IsPowerPac() const override;

	virtual size_t GetGhostCount() const override;
	virtual ff::PointInt GetGhostEyeDir(size_t nGhost) const override;
	virtual GhostState GetGhostState(size_t nGhost) const override;
	virtual IPlayingActor *GetGhost(size_t nGhost) override;

	virtual ff::PointInt GetGhostStartPixel() const override;
	virtual ff::PointInt GetGhostStartTile() const override;

	virtual FruitState GetFruitState() const override;
	virtual IPlayingActor *GetFruit() override;
	virtual FruitType GetFruitType() const override;
	virtual ff::PointInt GetFruitExitTile() const override;

	virtual const PointActor *GetPointDisplays(size_t &nCount) const override;
	virtual CustomActor * const *GetCustomActors(size_t &nCount) const override;

	// IPlayingActor

	virtual ff::PointInt GetTile() const override;
	virtual ff::PointInt GetPixel() const override;
	virtual void SetPixel(ff::PointInt pixel) override;

	virtual ff::PointInt GetDir() const override;
	virtual void SetDir(ff::PointInt dir) override;

	virtual ff::PointInt GetPressDir() const override;
	virtual void SetPressDir(ff::PointInt dir) override;

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
		OPT_FULL_SCREEN,
		OPT_ABOUT,
		OPT_NONE,
	};

	typedef std::function<ff::String()> GetTextFunc;

	struct Option
	{
		Option();
		Option(EOption type, ff::PointInt pos, GetTextFunc textFunc, int nMazes = -1);
		ff::String GetText() const;
		ff::RectFloat GetLevelRect() const;
		ff::RectFloat GetTargetRect() const;

		EOption _type;
		ff::PointInt _selectedPos;
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
	Option *HitTestOption(ff::PointFloat pos);
	static float GetLineHeight();

	void RenderBackMaze(ff::I2dRenderer *render);
	void RenderFrontMaze(ff::I2dRenderer *render);
	void RenderOptions(ff::I2dRenderer *render);
	void RenderFade(ff::I2dRenderer *render);

	ff::ComPtr<IMazes> _mazes;
	ff::ComPtr<IRenderMaze> _render;
	ff::ComPtr<IRenderText> _text;
	ff::ComPtr<IPlayingMaze> _backMaze;
	ff::ComPtr<ISoundEffects> _sounds;
	ff::TypedResource<ff::IInputMapping> _inputRes;
	ff::Vector<Option> _options;
	ff::String _scores;
	size_t _curOption;
	Stats _stats;
	float _fade;
	bool _fading;
	bool _done;
};
