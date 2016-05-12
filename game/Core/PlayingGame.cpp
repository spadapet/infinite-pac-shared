#include "pch.h"
#include "App.xaml.h"
#include "COM/ComObject.h"
#include "Core/Actors.h"
#include "Core/Audio.h"
#include "Core/Helpers.h"
#include "Core/Maze.h"
#include "Core/Mazes.h"
#include "Core/PlayingGame.h"
#include "Core/PlayingMaze.h"
#include "Core/RenderMaze.h"
#include "Core/RenderText.h"
#include "Globals/MetroGlobals.h"
#include "Graph/2D/2dRenderer.h"
#include "Input/KeyboardDevice.h"

static const size_t INITIAL_LIVES = 3;

class __declspec(uuid("b2e73e6f-c27f-4d59-96c4-b39e76d8ac2e"))
	Player
		: public ff::ComBase
		, public IPlayer
		, public IPlayingMazeHost
{
public:
	DECLARE_HEADER(Player);

	bool Init(size_t nPlayer, IMazes *pMazes);
	bool Advance();
	const ff::Vector<FruitType> &GetDisplayFruits();

	// IPlayer
	virtual size_t GetLevel() const override;
	virtual void SetLevel(size_t nLevel) override;
	virtual IPlayingMaze* GetPlayingMaze() override;
	virtual size_t GetScore() const override;
	virtual size_t GetLives() const override;
	virtual bool IsGameOver() const override;
	virtual bool DidCheat() const override;
	virtual void AddStats(Stats &stats) const override;

	// IPlayingMazeHost
	virtual void OnStateChanged(GameState oldState, GameState newState) override;
	virtual size_t GetMazePlayer() override;
	virtual bool IsPlayingLevel() override;
	virtual bool IsEffectEnabled(AudioEffect effect) override;
	virtual void OnPacUsingTunnel() override;

private:
	void OnPacWon();
	void OnPacDied();
	void CheckFreeLife();

	bool _isGameOver;
	Stats _stats;
	size_t _level;
	size_t _lives;
	size_t _nextFreeLife;
	size_t _freeLifeRepeat;
	size_t _freeLivesLeft;
	size_t _player;
	ff::ComPtr<IPlayingMaze> _playMaze;
	ff::ComPtr<ISoundEffects> _sounds;
	ff::ComPtr<IMazes> _mazes;
	ff::Vector<FruitType> _displayFruits;
};

BEGIN_INTERFACES(Player)
	HAS_INTERFACE(IPlayer)
END_INTERFACES()

Player::Player()
	: _isGameOver(false)
	, _level(0)
	, _lives(INITIAL_LIVES)
	, _nextFreeLife(10000)
	, _freeLifeRepeat(0)
	, _freeLivesLeft(1)
	, _player(0)
{
}

Player::~Player()
{
}

bool Player::Init(size_t nPlayer, IMazes *pMazes)
{
	assertRetVal(pMazes, false);

	_player = nPlayer;
	_mazes = pMazes;
	_lives = _mazes->GetStartingLives();
	_nextFreeLife = _mazes->GetFreeLifeScore();
	_freeLifeRepeat = _mazes->GetFreeLifeRepeat();
	_freeLivesLeft = _mazes->GetMaxFreeLives();

	_stats._gamesStarted++;

	SetLevel(0);

	return true;
}

bool Player::Advance()
{
	bool bDied = false;

	if (_playMaze)
	{
		_playMaze->Advance();

		CheckFreeLife();

		switch (_playMaze->GetGameState())
		{
		case GS_DIED:
			bDied = true;
			OnPacDied();
			break;

		case GS_WON:
			OnPacWon();
			break;
		}
	}

	return !_isGameOver && !bDied;
}

const ff::Vector<FruitType> &Player::GetDisplayFruits()
{
	return _displayFruits;
}

size_t Player::GetLevel() const
{
	return _level;
}

void Player::SetLevel(size_t nLevel)
{
	assertRet(_mazes);

	_isGameOver = false;
	_level = nLevel;

	_displayFruits.Clear();

	ff::ComPtr<IPlayingMaze> pPlayMaze;
	ff::ComPtr<ISoundEffects> pSounds;

	if (_mazes && _mazes->GetMazeCount())
	{
		size_t nMazeCount = _mazes->GetMazeCount();
		ff::ComPtr<IMaze> pMaze = _mazes->GetMaze(_level % nMazeCount);
		const Difficulty& diff = _mazes->GetDifficulty(_level);

		IPlayingMaze::Create(pMaze, diff, this, &pPlayMaze);
		ISoundEffects::Create(pMaze->GetCharType(), &pSounds);

		FruitType prevFruit = FRUIT_NONE;

		for (size_t i = _level;
			i != ff::INVALID_SIZE && _displayFruits.Size() < 7;
			i = ff::PreviousSize(i))
		{
			FruitType fruit = _mazes->GetDifficulty(i).GetFruit();

			// Don't show two random fruits in a row (looks dumb)
			if (fruit != FRUIT_NONE && (fruit != FRUIT_RANDOM || fruit != prevFruit))
			{
				_displayFruits.Push(fruit);
			}

			prevFruit = fruit;
		}
	}

	assert(pPlayMaze && pSounds);

	if (_playMaze)
	{
		// Save the old stats before they go away
		_stats += _playMaze->GetStats();
	}

	_playMaze = pPlayMaze;
	_sounds = pSounds;
}

IPlayingMaze *Player::GetPlayingMaze()
{
	return _playMaze;
}

size_t Player::GetScore() const
{
	return _stats._score + (_playMaze ? _playMaze->GetStats()._score : 0);
}

size_t Player::GetLives() const
{
	return _lives;
}

bool Player::IsGameOver() const
{
	return _isGameOver;
}

bool Player::DidCheat() const
{
	return _stats._cheated || (_playMaze && _playMaze->GetStats()._cheated);
}

void Player::AddStats(Stats &stats) const
{
	stats += _stats;

	if (_playMaze)
	{
		stats += _playMaze->GetStats();
	}
}

void Player::OnPacWon()
{
	_stats._levelsBeaten++;

	SetLevel(_level + 1);
}

void Player::OnPacDied()
{
	if (_lives > 1)
	{
		_lives--;

		if (_playMaze)
		{
			_playMaze->Reset();
		}
	}
	else
	{
		_isGameOver = true;
	}
}

void Player::CheckFreeLife()
{
	if (_nextFreeLife && _freeLivesLeft && GetScore() >= _nextFreeLife)
	{
		if (_sounds && IsEffectEnabled(EFFECT_FREE_LIFE))
		{
			_sounds->Play(EFFECT_FREE_LIFE);
		}

		if (_lives <= INITIAL_LIVES)
		{
			_lives++;
			_freeLivesLeft--;
		}

		_nextFreeLife = _freeLifeRepeat ? _nextFreeLife + _freeLifeRepeat : 0;
	}

#ifdef _DEBUG
	static bool s_bCheating = false;

	if (ff::MetroGlobals::Get()->GetKeys()->GetKey('0'))
	{
		_stats._cheated = true;

		if (!s_bCheating)
		{
			s_bCheating = true;

			_lives = std::min<size_t>(5, _lives + 1);
		}
	}
	else
	{
		s_bCheating = false;
	}

	if (ff::MetroGlobals::Get()->GetKeys()->GetKey('9'))
	{
		_stats._cheated = true;
		_stats._score += 100;
	}

	if (ff::MetroGlobals::Get()->GetKeys()->GetKey('8'))
	{
		// Doesn't count as cheating
		_lives = 1;
	}
#endif
}

// IPlayingMazeHost
size_t Player::GetMazePlayer()
{
	return _player;
}

// IPlayingMazeHost
bool Player::IsPlayingLevel()
{
	return true;
}

// IPlayingMazeHost
bool Player::IsEffectEnabled(AudioEffect effect)
{
	if (effect == EFFECT_INTRO)
	{
		return !_level && !_player;
	}

	return true;
}

// IPlayingMazeHost
void Player::OnStateChanged(GameState oldState, GameState newState)
{
}

// IPlayingMazeHost
void Player::OnPacUsingTunnel()
{
}

class __declspec(uuid("53360486-e457-476f-9453-38894c41b317"))
	PlayingGame : public ff::ComBase, public IPlayingGame
{
public:
	DECLARE_HEADER(PlayingGame);

	bool Init(size_t nPlayers, IPlayingGameHost *pHost, IMazes *pMazes);

	// IPlayingGame

	virtual void Advance() override;
	virtual void Render(ff::I2dRenderer *pRenderer) override;

	virtual IMazes* GetMazes() override;
	virtual ff::PointInt GetSizeInTiles() const override;
	virtual size_t GetHighScore() const override;

	virtual size_t GetPlayers() const override;
	virtual size_t GetCurrentPlayer() const override;
	virtual IPlayer* GetPlayer(size_t nPlayer) override;

	virtual bool IsGameOver() const override;
	virtual bool IsPaused() const override;
	virtual void TogglePaused() override;
	virtual void PausedAdvance() override;

private:
	void InternalAdvance(bool bForce);
	void InternalAdvanceOne();

	IPlayingGameHost* _host;

	ff::ComPtr<IMazes> _mazes;
	ff::ComPtr<IRenderText> _renderText;
	ff::ComPtr<Player> _players[2];

	bool _isGameOver;
	bool _paused;
	bool _singleAdvance;
	bool _switchPlayer;
	size_t _player;
	size_t _counter;
	size_t _gameOverCounter;

	static const int _nScoreTiles = 3;
	static const int _nStatusTiles = 2;
};

BEGIN_INTERFACES(PlayingGame)
	HAS_INTERFACE(IPlayingGame)
END_INTERFACES()

// static
bool IPlayingGame::Create(
	IMazes* pMazes,
	size_t nPlayers,
	IPlayingGameHost* pHost,
	IPlayingGame** ppGame)
{
	assertRetVal(ppGame, false);
	*ppGame = nullptr;

	ff::ComPtr<PlayingGame> pGame = new ff::ComObject<PlayingGame>;
	assertRetVal(pGame->Init(nPlayers, pHost, pMazes), false);

	*ppGame = ff::GetAddRef<IPlayingGame>(pGame);

	return true;
}

PlayingGame::PlayingGame()
	: _host(nullptr)
	, _isGameOver(false)
	, _paused(false)
	, _singleAdvance(false)
	, _switchPlayer(false)
	, _player(0)
	, _counter(0)
	, _gameOverCounter(0)
{
}

PlayingGame::~PlayingGame()
{
}

bool PlayingGame::Init(size_t nPlayers, IPlayingGameHost *pHost, IMazes *pMazes)
{
	assertRetVal(pMazes, false);
	assertRetVal(nPlayers >= 1 && nPlayers <= _countof(_players), false);

	_host = pHost;
	_mazes = pMazes;

	for (size_t i = 0; i < nPlayers; i++)
	{
		_players[i] = new ff::ComObject<Player>;
		assertRetVal(_players[i]->Init(i, pMazes), false);
	}

	assertRetVal(IRenderText::Create(&_renderText), false);

	return true;
}

void PlayingGame::InternalAdvanceOne()
{
	Player *pPlayer = _players[_player];

	if (_switchPlayer)
	{
		_switchPlayer = false;

		size_t nNewPlayer = !_player ? 1 : 0;
		Player* pOtherPlayer = _players[nNewPlayer];

		if (pOtherPlayer && !pOtherPlayer->IsGameOver())
		{
			_player = nNewPlayer;
			pPlayer = _players[_player];
		}
		else if (pPlayer->IsGameOver())
		{
			_isGameOver = true;
			_gameOverCounter = 1;
		}
	}

	if (_gameOverCounter)
	{
		_gameOverCounter++;

		if (!_isGameOver && _gameOverCounter > 120)
		{
			_gameOverCounter = 0;
			_switchPlayer = true; // switch during the NEXT advance

			if (_host)
			{
				_host->OnPlayerGameOver(this, pPlayer);
			}
		}
	}
	else if (pPlayer)
	{
		if (!pPlayer->Advance())
		{
			if (pPlayer->IsGameOver())
			{
				_gameOverCounter++;
			}
			else
			{
				_switchPlayer = true;
			}
		}
	}

	_counter++;
}

void PlayingGame::InternalAdvance(bool bForce)
{
	if (bForce || !_paused)
	{
		size_t speed = 1;

#ifdef _DEBUG
		if (ff::MetroGlobals::Get()->GetKeys()->GetKey(VK_INSERT))
		{
			// Is this cheating? If so, set _bCheated in the stats
			speed = 4;
		}
#endif

		for (; speed > 0; speed--)
		{
			InternalAdvanceOne();
		}
	}
}

void PlayingGame::Advance()
{
	InternalAdvance(false);
}

static const DirectX::XMFLOAT4 s_colorText (0.8706f, 0.8706f, 1, 1);
static const DirectX::XMFLOAT4 s_colorPaused (1, 0, 0, 0.5f);
static const DirectX::XMFLOAT4 s_colorGameOver (1, 0, 0, 1);
static const DirectX::XMFLOAT4 s_colorWhite (1, 1, 1, 1);
static const DirectX::XMFLOAT4 s_colorBlack (0, 0, 0, 1);
static const DirectX::XMFLOAT4 s_pausedFade (0, 0, 0, 0.5f);
static const DirectX::XMFLOAT4 s_colorPlayerText(0, 1, 1, 1);
static const DirectX::XMFLOAT4 s_colorReadyText (1, 1, 0, 1);

void PlayingGame::Render(ff::I2dRenderer *pRenderer)
{
	assertRet(pRenderer);

	ff::PointInt totalTiles = GetSizeInTiles();
	bool bShowScores = (!_host || _host->IsShowingScoreBar(this));
	bool bShowStatus = (!_host || _host->IsShowingStatusBar(this));
	IPlayingMaze* pPlayMaze = _players[_player]->GetPlayingMaze();

	if (bShowScores)
	{
		ff::String szScore0 = _players[0] ? FormatScoreAsString(_players[0]->GetScore()) : ff::String();
		ff::String szScore1 = _players[1] ? FormatScoreAsString(_players[1]->GetScore()) : ff::String();
		ff::String szHighScore = FormatScoreAsString(GetHighScore());
		ff::PointFloat tileSize = PixelsPerTileF();
		bool bNameVisible = _isGameOver || _switchPlayer || (_counter % 30) < 15;

		if ((bNameVisible || _player != 0) && _players[0])
		{
			_renderText->DrawText(pRenderer, L"1UP", ff::PointFloat(3 * tileSize.x, 0), 0, &s_colorText, nullptr, nullptr);
		}

		if ((bNameVisible || _player != 1) && _players[1])
		{
			_renderText->DrawText(pRenderer, L"2UP", ff::PointFloat(tileSize.x * (totalTiles.x - 6), 0), 0, &s_colorText, nullptr, nullptr);
		}

		_renderText->DrawText(pRenderer, szScore0.c_str(), ff::PointFloat(0, tileSize.y), 0, &s_colorText, nullptr, nullptr);
		_renderText->DrawText(pRenderer, szScore1.c_str(), ff::PointFloat(tileSize.x * (totalTiles.x - 8), tileSize.y), 0, &s_colorText, nullptr, nullptr);

		_renderText->DrawText(pRenderer, L"HIGH SCORE", ff::PointFloat(9 * tileSize.x, 0), 0, &s_colorText, nullptr, nullptr);
		_renderText->DrawText(pRenderer, szHighScore.c_str(), ff::PointFloat(10 * tileSize.x, tileSize.y), 0, &s_colorText, nullptr, nullptr);

		if (_singleAdvance)
		{
			ff::String szFrame = FormatScoreAsString(_counter);
			_renderText->DrawText(pRenderer, szFrame.c_str(), ff::PointFloat(0, 2 * tileSize.y), 0, &s_colorPaused, nullptr, nullptr);

			if (pPlayMaze)
			{
				ff::PointInt pix = pPlayMaze->GetPac()->GetPixel();

				if (pix.x >= 0 && pix.y >= 0)
				{
					ff::String szX = FormatScoreAsString((size_t)pix.x);
					ff::String szY = FormatScoreAsString((size_t)pix.y);

					_renderText->DrawText(pRenderer, szX.c_str(), ff::PointFloat(17 * tileSize.x, 2 * tileSize.y), 0, &s_colorPaused, nullptr, nullptr);
					_renderText->DrawText(pRenderer, szY.c_str(), ff::PointFloat(21 * tileSize.x, 2 * tileSize.y), 0, &s_colorPaused, nullptr, nullptr);
				}
			}
		}
	}

	size_t nLives = _players[_player]->GetLives();

	if (bShowStatus && pPlayMaze && nLives > 0)
	{
		size_t nRenderLives = (pPlayMaze->GetGameState() >= GS_READY)
			? ff::PreviousSize(nLives)
			: nLives;

		if (nRenderLives > 0)
		{
			nRenderLives = std::min<size_t>(5, nRenderLives);

			pPlayMaze->GetRenderMaze()->RenderFreeLives(
				pRenderer,
				pPlayMaze,
				nRenderLives,
				TileTopLeftToPixelF(ff::PointInt(3, totalTiles.y - 1)));
		}
	}

	if (bShowStatus && pPlayMaze && _players[_player]->GetDisplayFruits().Size())
	{
		pPlayMaze->GetRenderMaze()->RenderStatusFruits(
			pRenderer,
			_players[_player]->GetDisplayFruits().Data(),
			_players[_player]->GetDisplayFruits().Size(),
			TileTopLeftToPixelF(ff::PointInt(totalTiles.x - 3, totalTiles.y - 1)));
	}

	if (pPlayMaze)
	{
		if (bShowScores)
		{
			pRenderer->PushMatrix(ff::MATRIX_WORLD);
			DirectX::XMFLOAT4X4 matrix;
			DirectX::XMStoreFloat4x4(&matrix, DirectX::XMMatrixTranslation(0, -_nScoreTiles * PixelsPerTileF().y, 0));
			pRenderer->TransformMatrix(ff::MATRIX_WORLD, &matrix);
		}

		pPlayMaze->Render(pRenderer);

		if (bShowScores)
		{
			pRenderer->PopMatrix(ff::MATRIX_WORLD);
		}
	}

	if ((_isGameOver || _gameOverCounter) && pPlayMaze)
	{
		ff::PointInt doorTile = pPlayMaze->GetGhostStartTile();
		ff::PointFloat textPos;

		if (!_isGameOver)
		{
			textPos = TileTopLeftToPixelF(doorTile + ff::PointInt(-4, 0 + (bShowScores ? _nScoreTiles : 0)));

			_renderText->DrawText(
				pRenderer,
				!_player ? L"PLAYER ONE" : L"PLAYER TWO",
				textPos, 0,
				&s_colorGameOver,
				nullptr, nullptr);
		}

		textPos = TileTopLeftToPixelF(doorTile + ff::PointInt(-4, 6 + (bShowScores ? _nScoreTiles : 0)));
	
		_renderText->DrawText(
			pRenderer,
			L"GAME OVER",
			textPos, 0,
			&s_colorGameOver,
			nullptr, nullptr);
	}

	if (pPlayMaze->GetGameState() <= GS_READY)
	{
		// Render intro text

		if (pPlayMaze->GetGameState() == GS_PLAYER_READY)
		{
			ff::PointInt tile = pPlayMaze->GetGhostStartTile() + ff::PointInt(-4, bShowScores ? _nScoreTiles : 0);

			_renderText->DrawText(
				pRenderer,
				(_player == 1) ? L"PLAYER TWO" : L"PLAYER ONE",
				TileTopLeftToPixelF(tile), 0,
				&s_colorPlayerText,
				nullptr, nullptr);

			tile = pPlayMaze->GetGhostStartTile() + ff::PointInt(-3, 6 + (bShowScores ? _nScoreTiles : 0));

			wchar_t szLevelText[12];
			_sntprintf_s(szLevelText, _TRUNCATE,
				L"LEVEL %02Iu",
				_players[_player]->GetLevel() + 1);

			_renderText->DrawText(
				pRenderer,
				szLevelText,
				TileTopLeftToPixelF(tile), 0,
				&s_colorReadyText,
				nullptr, nullptr);
		}
		else if (pPlayMaze->GetGameState() == GS_READY)
		{
			ff::PointInt tile = pPlayMaze->GetGhostStartTile() + ff::PointInt(-2, 6 + (bShowScores ? _nScoreTiles : 0));

			_renderText->DrawText(
				pRenderer,
				L"READY!",
				TileTopLeftToPixelF(tile), 0,
				&s_colorReadyText,
				nullptr, nullptr);
		}
	}

	if (_paused)
	{
		if (!_singleAdvance)
		{
			// Fade the screen

			pRenderer->DrawFilledRectangle(
				&ff::RectFloat(ff::PointFloat(0, 0), TileTopLeftToPixelF(totalTiles)),
				&s_pausedFade, 1);
		}

		if (pPlayMaze)
		{
			ff::PointInt doorTile = pPlayMaze->GetGhostStartTile();
			ff::PointFloat pausedPos = TileTopLeftToPixelF(doorTile + ff::PointInt(-5, 4 + (bShowScores ? _nScoreTiles : 0)));
			const wchar_t *szPaused = L"PAUSED";
			ff::PointFloat scale(2, 2);

			_renderText->DrawText(
				pRenderer,
				szPaused,
				pausedPos + ff::PointFloat(-0.5f, -0.5f), 0,
				&s_colorBlack,
				nullptr, &scale);

			_renderText->DrawText(
				pRenderer,
				szPaused,
				pausedPos + ff::PointFloat(0.5f, 0.5f), 0,
				&s_colorWhite,
				nullptr, &scale);
	
			_renderText->DrawText(
				pRenderer,
				szPaused,
				pausedPos, 0,
				&s_colorPaused,
				nullptr, &scale);
		}
	}
}

IMazes *PlayingGame::GetMazes()
{
	return _mazes;
}

ff::PointInt PlayingGame::GetSizeInTiles() const
{
	ff::PointInt size(16, 16);
	IPlayingMaze* pPlayMaze = _players[_player]->GetPlayingMaze();

	if (pPlayMaze)
	{
		ff::PointInt mazeSize = pPlayMaze->GetMaze()->GetSizeInTiles();

		int extraHeight = 0;

		if (!_host || _host->IsShowingScoreBar(const_cast<PlayingGame*>(this)))
		{
			extraHeight += _nScoreTiles;
		}

		if (!_host || _host->IsShowingStatusBar(const_cast<PlayingGame*>(this)))
		{
			extraHeight += _nStatusTiles;
		}

		size.x = std::max(size.x, mazeSize.x);
		size.y = std::max(size.y, mazeSize.y + extraHeight);
	}

	return size;
}

size_t PlayingGame::GetHighScore() const
{
	size_t nScore = 0;

	for (size_t i = 0; i < _countof(_players); i++)
	{
		if (_players[i])
		{
			const Stats &stats = Stats::Get(_mazes->GetID());

			nScore = std::max<size_t>(nScore, _players[i]->GetScore());
			nScore = std::max<size_t>(nScore, stats._highScores[0]._score);
		}
	}

	return nScore;
}

size_t PlayingGame::GetPlayers() const
{
	if (_players[1])
	{
		return 2;
	}

	if (_players[0])
	{
		return 1;
	}

	assertRetVal(false, 0);
}

size_t PlayingGame::GetCurrentPlayer() const
{
	return _player;
}

IPlayer *PlayingGame::GetPlayer(size_t nPlayer)
{
	return (nPlayer >= 0 && nPlayer < _countof(_players)) ? _players[nPlayer] : nullptr;
}

bool PlayingGame::IsGameOver() const
{
	return _isGameOver && _gameOverCounter > 120;
}

bool PlayingGame::IsPaused() const
{
	return _paused;
}

void PlayingGame::TogglePaused()
{
	_paused = !_paused;

	_singleAdvance = false;
}

void PlayingGame::PausedAdvance()
{
#ifdef _DEBUG
	if (_paused)
	{
		_singleAdvance = true;

		InternalAdvance(true);
	}
#endif
}
