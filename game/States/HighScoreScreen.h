#pragma once

#include "Core/Actors.h"
#include "Core/PlayingGame.h"
#include "Core/PlayingMaze.h"
#include "Resource/ResourceValue.h"

namespace ff
{
	class IInputMapping;
}

class PacApplication;
class IRenderText;

class __declspec(uuid("eb214644-fafc-41d1-9d39-b27b0ff425a5"))
	HighScoreScreen
		: public ff::ComBase
		, public IPlayingGame
		, public IPlayingMazeHost
		, public IPlayingMaze
		, public IPlayingActor
{
public:
	DECLARE_HEADER(HighScoreScreen);

	bool Init(ff::StringRef mazesId, IPlayer *pPlayer);

	ff::String GetName() const;

	// IPlayingGame

	virtual void Advance() override;
	virtual void Render(ff::I2dRenderer *pRenderer) override;

	virtual IMazes *GetMazes() override;
	virtual ff::PointInt GetSizeInTiles() const override;
	virtual size_t GetHighScore() const override;

	virtual size_t GetPlayers() const override;
	virtual size_t GetCurrentPlayer() const override;
	virtual IPlayer* GetPlayer(size_t nPlayer) override;

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

	//virtual void Advance() override;
	//virtual void Render(ff::I2dRenderer *pRenderer) override;
	virtual void Reset() override;

	virtual GameState GetGameState() const override;
	virtual const Stats &GetStats() const override;
	virtual IMaze *GetMaze() const override;
	virtual IRenderMaze* GetRenderMaze() override;
	virtual const Difficulty &GetDifficulty() const override;

	virtual PacState GetPacState() const override;
	virtual IPlayingActor* GetPac() override;
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
	ff::TypedResource<ff::IInputMapping> _inputRes;
	ff::ComPtr<IMazes> _mazes;
	ff::ComPtr<IRenderMaze> _render;
	ff::ComPtr<IRenderText> _text;
	ff::ComPtr<IPlayer> _player;
	ff::String _intro;
	ff::String _name;
	size_t _counter;
	ff::String _mazesID;
	bool _done;
	bool _showNewLetter;

	struct Letter
	{
		wchar_t _letter[5];
		ff::PointFloat _pos;
		int _vk;
	};

	Letter *GetLetter(wchar_t ch);
	Letter *HitTestLetter(ff::PointFloat pos);
	ff::PointFloat GetLetterPos(wchar_t ch);

	ff::Vector<Letter> _letters;
	size_t _letter;
};
