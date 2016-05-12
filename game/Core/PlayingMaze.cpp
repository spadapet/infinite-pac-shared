#include "pch.h"
#include "COM/ComObject.h"
#include "Core/Actors.h"
#include "Core/Audio.h"
#include "Core/GhostBrains.h"
#include "Core/Helpers.h"
#include "Core/Maze.h"
#include "Core/PlayingMaze.h"
#include "Core/RenderMaze.h"
#include "Core/RenderText.h"
#include "Core/Tiles.h"
#include "Globals/Log.h"
#include "Globals/MetroGlobals.h"
#include "Graph/2D/2dRenderer.h"
#include "Graph/Anim/AnimPos.h"
#include "Graph/Anim/AnimKeys.h"
#include "Input/KeyboardDevice.h"
#include "Types/Timer.h"

static const int DOT_BUBBLE_COUNT = 2;
static const int FRUIT_BUBBLE_COUNT = 20;
static const int POWER_BUBBLE_COUNT = 20;
static const int GHOST_BUBBLE_COUNT = 30;

static const int DOT_BUBBLE_SPREAD = 7;
static const int FRUIT_BUBBLE_SPREAD = 13;
static const int POWER_BUBBLE_SPREAD = 13;
static const int GHOST_BUBBLE_SPREAD = 13;

static bool operator<(
	const std::pair<ff::PointInt, ff::PointInt> &lhs,
	const std::pair<ff::PointInt, ff::PointInt> &rhs)
{
	return ::memcmp(&lhs, &rhs, sizeof(lhs)) < 0;
}

class __declspec(uuid("a334c2a0-e23f-4033-8abb-2737de5cbb3a"))
	PlayingMaze : public ff::ComBase, public IPlayingMaze
{
public:
	DECLARE_HEADER(PlayingMaze);

	bool Init(IMaze *pMaze, const Difficulty &difficulty, IPlayingMazeHost *pHost);

	// IPlayingMaze

	virtual void Advance() override;
	virtual void Render(ff::I2dRenderer *pRenderer) override;
	virtual void Reset() override;

	virtual GameState GetGameState() const override;
	virtual const Stats& GetStats() const override;
	virtual IMaze* GetMaze() const override;
	virtual IRenderMaze* GetRenderMaze() override;
	virtual const Difficulty &GetDifficulty() const override;

	virtual PacState GetPacState() const override;
	virtual IPlayingActor* GetPac() override;
	virtual CharType GetCharType() const override;
	virtual bool IsPowerPac() const override;

	virtual size_t GetGhostCount() const override;
	virtual ff::PointInt GetGhostEyeDir(size_t nGhost) const override;
	virtual GhostState GetGhostState (size_t nGhost) const override;
	virtual IPlayingActor* GetGhost (size_t nGhost) override;

	virtual ff::PointInt GetGhostStartPixel() const override;
	virtual ff::PointInt GetGhostStartTile() const override;

	virtual FruitState GetFruitState() const override;
	virtual IPlayingActor* GetFruit() override;
	virtual FruitType GetFruitType() const override;
	virtual ff::PointInt GetFruitExitTile() const override;

	virtual const PointActor *GetPointDisplays(size_t &nCount) const override;
	virtual CustomActor * const *GetCustomActors(size_t &nCount) const override;

private:
	void SetGameState(GameState state);
	void AddPoints(size_t nPoints);
	void PlayEffect(AudioEffect effect);
	void StopEffect(AudioEffect effect);
	void StopEffectBG();

	void AddPointDisplay(
		ff::PointInt pos,
		ff::PointFloat scale,
		size_t nPoints,
		size_t nTimer,
		const DirectX::XMFLOAT4& color,
		bool bFades);
	void AddCustomActor(CustomActor *actor);
	void AddBubble(IPlayingActor &pac, int maxCount, int spread);

	void InitActorPositions();
	void InitDotCount();

	void RenderDebugGhostPaths(ff::I2dRenderer *pRenderer);

	void AdvanceSounds();
	void AdvanceRenderer();
	void AdvanceActors();
	void AdvancePac(PacActor &pac);
	void AdvanceGhost(GhostActor &ghost);
	void AdvanceFruit(FruitActor &fruit);
	void AdvanceCustomActors();

	void UpdatePointDisplays();
	void UpdateFruitMode();
	void UpdateGhostMode();
	void UpdateGhostHouse();
	void UpdateActorSpeeds();
	void UpdateScatterChaseTimes(size_t nIndex);

	void CheckPacCollisions(PacActor &pac);
	bool CheckTunnel(PlayingActor &actor);
	bool CheckGhostsScared(bool &scared, bool &eyes) const;

	void OnPacEatDot(PacActor &pac, bool bPower);
	void OnPacEatFruit(PacActor &pac, FruitActor &fruit);
	void OnPacEatGhost(PacActor &pac, GhostActor &ghost);
	void OnGhostEatPac(PacActor &pac, GhostActor &ghost);

	size_t GhostIndex(GhostActor &ghost);
	ff::PointInt GhostDecidePress(GhostActor &ghost);
	ff::PointInt GhostDecidePress(IGhostBrains *brains, MoveState move, ff::PointInt tile, ff::PointInt dir);
	void FlipGhosts();
	void ScareGhosts(bool bScared);
	bool ReleaseGhost();
	size_t CountGhostsInHouse();

	ff::PointInt FruitDecideDir(FruitActor &fruit);

	bool IsWall(ff::PointInt tile);
	bool HitWall(ff::PointInt tile, ff::PointInt dir);

	ff::ComPtr<IMaze> _maze;
	ff::ComPtr<IRenderMaze> _renderMaze;
	ff::ComPtr<IRenderText> _renderText;
	ff::ComPtr<ISoundEffects> _sound;
	IPlayingMazeHost* _host;

	// Actors
	PacActor _pac;
	FruitActor _fruit;
	GhostActor _ghosts[4];
	ff::Vector<PointActor> _points;
	ff::Vector<CustomActor*> _customs;

	// Game state stuff
	Stats _stats;
	GameState _state;
	Difficulty _difficulty;
	size_t _stateCounter;

	// Dot stuff
	size_t _dotCount;
	size_t _dotCountTotal;
	size_t _lastDotCounter;
	size_t _globalDotIndex;
	size_t _globalDotCounter;
	size_t _dotEffect;

	// Ghost stuff
	size_t _ghostCount;
	size_t _ghostScatterCountdown;
	size_t _ghostScaredCountdown;
	size_t _ghostChaseCountdown;
	size_t _ghostEatenCountdown;
	size_t _ghostEatenIndex;
	size_t _ghostScatterChaseIndex;
	ff::PointInt _ghostStartPixel;
	ff::PointInt _ghostStartTile;
	ff::RectInt _ghostHouseRect;

	// Fruit stuff
	size_t _nFruitCounter;
	size_t _nCurrentFruit;
	size_t _nFruitDots[2];
	ff::PointInt _fruitPixel;
	ff::Vector<std::pair<ff::PointInt, ff::PointInt>> _fruitStartTiles;
};

BEGIN_INTERFACES(PlayingMaze)
	HAS_INTERFACE(IPlayingMaze)
END_INTERFACES()

static const DirectX::XMFLOAT4 s_ghostPointsTextColor(0, 1, 1, 1);
static const DirectX::XMFLOAT4 s_fruitPointsTextColor(1, 0.7216f, 1, 1);

// static
bool IPlayingMaze::Create(
		IMaze* pMaze,
		const Difficulty& difficulty,
		IPlayingMazeHost* pHost,
		IPlayingMaze** ppPlayMaze)
{
	assertRetVal(ppPlayMaze, false);
	*ppPlayMaze = nullptr;

	ff::ComPtr<PlayingMaze> pPlayMaze = new ff::ComObject<PlayingMaze>;
	assertRetVal(pPlayMaze->Init(pMaze, difficulty, pHost), false);

	*ppPlayMaze = ff::GetAddRef<IPlayingMaze>(pPlayMaze);

	return true;
}

PlayingMaze::PlayingMaze()
	: _host(nullptr)
	, _ghostStartTile(0, 0)
	, _ghostStartPixel(0, 0)
	, _ghostHouseRect(0, 0, 0, 0)
	, _state(GS_BEFORE_TIME)
	, _stateCounter(0)
	, _dotCount(0)
	, _dotCountTotal(0)
	, _lastDotCounter(0)
	, _globalDotCounter(0)
	, _dotEffect(0)
	, _globalDotIndex(0)
	, _ghostScaredCountdown(0)
	, _ghostEatenCountdown(0)
	, _ghostEatenIndex(0)
	, _ghostCount(0)
	, _ghostScatterCountdown(0)
	, _ghostChaseCountdown(0)
	, _ghostScatterChaseIndex(0)
	, _nFruitCounter(0)
	, _nCurrentFruit(0)
	, _fruitPixel(0, 0)
{
	ff::ZeroObject(_difficulty);
	ff::ZeroObject(_nFruitDots);
}

PlayingMaze::~PlayingMaze()
{
}

bool PlayingMaze::Init(IMaze *pMaze, const Difficulty &difficulty, IPlayingMazeHost *pHost)
{
	assertRetVal(pMaze, false);

	_host = pHost;

	// Clone the maze so that it can be modified
	{
		ff::ComPtr<IMaze> pClone;
		assertRetVal(pMaze->Clone(false, &pClone), false);
		assertRetVal(_maze.QueryFrom(pClone), false);
	}

	assertRetVal(IRenderMaze::Create(_maze, &_renderMaze), false);
	assertRetVal(IRenderText::Create(&_renderText), false);
	
	if (!pHost || pHost->IsPlayingLevel())
	{
		// Don't care if audio creation fails
		ISoundEffects::Create(_maze->GetCharType(), &_sound);
	}

	for (size_t i = 0; i < _countof(_ghosts); i++)
	{
		ff::ComPtr<IGhostBrains> brains;
		assertRetVal(IGhostBrains::Create(i, &brains), false);

		_ghosts[i].SetBrains(brains);
	}

	_difficulty = difficulty;

	UpdateScatterChaseTimes(_ghostScatterChaseIndex);
	InitActorPositions();
	InitDotCount();

	return true;
}

void PlayingMaze::InitActorPositions()
{
	bool bFoundFruit = false;

	_fruitStartTiles.Clear();
	_fruit.SetActive(false);
	_pac.SetActive(false);

	for (size_t i = 0; i < _countof(_ghosts); i++)
	{
		_ghosts[i].SetActive(false);
	}

	_ghostCount = 0;

	for (ff::PointInt tile(0, 0), size = _maze->GetSizeInTiles();
		tile.y < size.y; tile.x = 0, tile.y++)
	{
		for (; tile.x < size.x; tile.x++)
		{
			TileContent content = _maze->GetTileContent(tile);
			TileZone zone = _maze->GetTileZone(tile);

			if (content == CONTENT_GHOST_DOOR && !_ghostCount)
			{
				_ghostCount = std::min(_difficulty._ghostCount, _countof(_ghosts));
				_ghostStartTile = tile + ff::PointInt(0, -1);
				_ghostStartPixel = TileMiddleRightToPixel(_ghostStartTile);

				if (!bFoundFruit)
				{
					_fruitPixel = TileMiddleRightToPixel(tile + ff::PointInt(0, 5));
				}

				_ghostHouseRect.SetRect(
					TileTopLeftToPixel(tile + ff::PointInt(-1, 2)),
					TileBottomRightToPixel(tile + ff::PointInt(2, 2)));

				_ghosts[0].SetActive(_ghostCount >= 1);
				_ghosts[0].SetPixel(TileMiddleRightToPixel(tile + ff::PointInt(0, -1)));
				_ghosts[0].SetDir(ff::PointInt(-1, 0));

				_ghosts[1].SetActive(_ghostCount >= 2);
				_ghosts[1].SetHouseState(HOUSE_INSIDE);
				_ghosts[1].SetPixel(TileMiddleRightToPixel(tile + ff::PointInt(0, 2)));
				_ghosts[1].SetDir(ff::PointInt(0, 1));

				_ghosts[2].SetActive(_ghostCount >= 3);
				_ghosts[2].SetHouseState(HOUSE_INSIDE);
				_ghosts[2].SetPixel(TileMiddleRightToPixel(tile + ff::PointInt(-2, 2)));
				_ghosts[2].SetDir(ff::PointInt(0, -1));

				_ghosts[3].SetActive(_ghostCount >= 4);
				_ghosts[3].SetHouseState(HOUSE_INSIDE);
				_ghosts[3].SetPixel(TileMiddleRightToPixel(tile + ff::PointInt(2, 2)));
				_ghosts[3].SetDir(ff::PointInt(0, -1));
			}
			else if (content == CONTENT_PAC_START && !_pac.IsActive() &&
				(!_host || _host->GetMazePlayer() != ff::INVALID_SIZE))
			{
				_pac.SetActive(true);
				_pac.SetPixel(TileMiddleRightToPixel(tile));
				_pac.SetDir(ff::PointInt(-1, 0));
			}
			else if (content == CONTENT_FRUIT_START && !bFoundFruit)
			{
				_fruitPixel = TileMiddleRightToPixel(tile);

				bFoundFruit = true;
			}
			else if (content == CONTENT_NOTHING && zone == ZONE_GHOST_SLOW)
			{
				if (tile.x == 0)
				{
					std::pair<ff::PointInt, ff::PointInt> pair(ff::PointInt(tile.x - 1, tile.y), ff::PointInt(1, 0));
					_fruitStartTiles.Push(pair);
				}
				else if (tile.x == size.x - 1)
				{
					std::pair<ff::PointInt, ff::PointInt> pair(ff::PointInt(tile.x + 1, tile.y), ff::PointInt(-1, 0));
					_fruitStartTiles.Push(pair);
				}
				else if (tile.y == 0)
				{
					std::pair<ff::PointInt, ff::PointInt> pair(ff::PointInt(tile.x, tile.y - 1), ff::PointInt(0, 1));
					_fruitStartTiles.Push(pair);
				}
				else if (tile.y == size.y - 1)
				{
					std::pair<ff::PointInt, ff::PointInt> pair(ff::PointInt(tile.x, tile.y + 1), ff::PointInt(0, -1));
					_fruitStartTiles.Push(pair);
				}
			}
		}
	}
}

void PlayingMaze::InitDotCount()
{
	// Set the dot count for each ghost

	for (size_t i = 0; i < _countof(_ghosts); i++)
	{
		_ghosts[i].SetDotCounter(_difficulty.GetGhostDotCounter(i));
	}

	// Count dots

	ff::PointInt size = _maze->GetSizeInTiles();

	for (ff::PointInt tile(0, 0); tile.y < size.y; tile.x = 0, tile.y++)
	{
		for (; tile.x < size.x; tile.x++)
		{
			TileContent content = _maze->GetTileContent(tile);

			if (content == CONTENT_DOT || content == CONTENT_POWER)
			{
				_dotCount++;
				_dotCountTotal++;
			}
		}
	}

	// Update fruit dot count

	_difficulty.GetFruitDotCount(_dotCount, _nFruitDots[0], _nFruitDots[1]);
}

IRenderMaze *PlayingMaze::GetRenderMaze()
{
	return _renderMaze;
}

const Difficulty &PlayingMaze::GetDifficulty() const
{
	return _difficulty;
}

void PlayingMaze::SetGameState(GameState state)
{
	bool bLevel = !_host || _host->IsPlayingLevel();

	if (bLevel)
	{
		_state = state;
		_stateCounter = 0;
	}
	else if (_state != GS_PLAYING)
	{
		// Can't leave the playing state when not playing a level
		// (yes, that sounds weird)

		_state = GS_PLAYING;
		_stateCounter = 0;
	}
}

void PlayingMaze::AddPoints(size_t nPoints)
{
	_stats._score += (DWORD)nPoints;
}

void PlayingMaze::PlayEffect(AudioEffect effect)
{
	if (_sound && (!_host || _host->IsEffectEnabled(effect)))
	{
		_sound->Play(effect);
	}
}

void PlayingMaze::StopEffect(AudioEffect effect)
{
	if (_sound)
	{
		_sound->Stop(effect);
	}
}

void PlayingMaze::StopEffectBG()
{
	if (_sound)
	{
		_sound->StopBG();
	}
}

void PlayingMaze::AddPointDisplay(
	ff::PointInt pos,
	ff::PointFloat scale,
	size_t nPoints,
	size_t nTimer,
	const DirectX::XMFLOAT4& color,
	bool bFades)
{
	PointActor point;

	point.SetActive(true);
	point.SetPixel(pos);

	point.SetCountdown(nTimer);
	point.SetPoints(nPoints);
	point.SetColor(color, bFades);
	point.SetScale(scale);

	_points.Push(point);
}

void PlayingMaze::AddCustomActor(CustomActor *actor)
{
	assertRet(actor);
	_customs.Push(actor);
}

void PlayingMaze::AddBubble(IPlayingActor &pac, int maxCount, int spread)
{
	static std::array<const wchar_t *, 3> s_names =
	{
		L"bubble-1-anim",
		L"bubble-2-anim",
		L"bubble-3-anim",
	};

	int count = maxCount - rand() % (maxCount / 2);

	for (int i = 0; i < count; i++)
	{
		ff::PointFloat pos = (pac.GetPixel() + ff::PointInt(
			rand() % spread - spread / 2,
			rand() % spread - spread / 2)).ToFloat();
		float scale = 1.0f + (rand() % 9 - 4) / 12.0f;
		float timeScale = 1.0f + (rand() % 9 - 4) / 12.0f;
		float velocity = (rand() % 10) / -100.0f;
		const wchar_t *animName = s_names[rand() % s_names.size()];

		AddCustomActor(new SpriteAnimActor(
			animName,
			pos,
			ff::PointFloat(scale, scale),
			ff::GetColorWhite(),
			ff::POSE_TWEEN_SPLINE_CLAMP,
			ff::PointFloat(0, velocity),
			ff::PointFloat::Zeros(),
			timeScale,
			false)); // force opaque
	}
}

void PlayingMaze::Advance()
{
	switch (_state)
	{
	case GS_BEFORE_TIME:
		if (!_host || _host->IsPlayingLevel())
		{
			if (!_host || !_host->GetMazePlayer())
			{
				SetGameState(GS_PLAYER_READY);
				PlayEffect(EFFECT_INTRO);
			}
			else
			{
				SetGameState(GS_READY);
			}
		}
		else
		{
			SetGameState(GS_PLAYING);
		}
		break;

	case GS_PLAYER_READY:
		if (_stateCounter > 150)
		{
			SetGameState(GS_READY);
		}
		break;

	case GS_READY:
		if (_stateCounter > 120)
		{
			SetGameState(GS_PLAYING);
			StopEffect(EFFECT_INTRO);
		}
		break;

	case GS_PLAYING:
		AdvanceActors();
		break;

	case GS_CAUGHT:
		if (_stateCounter > 60)
		{
			SetGameState(GS_DYING);
		}
		break;

	case GS_DYING:
		if (_stateCounter == 30)
		{
			PlayEffect(EFFECT_DYING);
		}
		else if (_stateCounter > 120)
		{
			SetGameState(GS_DEAD);
		}
		break;

	case GS_DEAD:
		if (_stateCounter > 30)
		{
			SetGameState(GS_DIED);

			// This object must now be Reset() by the owner
		}
		break;

	case GS_WINNING:
		if (_stateCounter > 240)
		{
			SetGameState(GS_WON);

			// This object must now be Reset() by the owner
		}
	}

	AdvanceSounds();

	AdvanceRenderer();

	_stateCounter++;
}

void PlayingMaze::AdvanceSounds()
{
	switch (_state)
	{
	case GS_PLAYING:
		{
			bool bScared = false;
			bool bEyes = false;
			CheckGhostsScared(bScared, bEyes);

			if (bEyes)
			{
				PlayEffect(EFFECT_BACKGROUND_EYES);
			}
			else if (bScared)
			{
				PlayEffect(EFFECT_BACKGROUND_SCARED);
			}
			else if (_dotCount < (_dotCountTotal >> 3))
			{
				PlayEffect(EFFECT_BACKGROUND4);
			}
			else if (_dotCount < (_dotCountTotal >> 2))
			{
				PlayEffect(EFFECT_BACKGROUND3);
			}
			else if (_dotCount < (_dotCountTotal >> 1))
			{
				PlayEffect(EFFECT_BACKGROUND2);
			}
			else
			{
				PlayEffect(EFFECT_BACKGROUND1);
			}
		}
		break;

	default:
		StopEffectBG();
		break;
	}
}

void PlayingMaze::AdvanceRenderer()
{
	bool bAdvancePac = (_state >= GS_PLAYING);
	bool bAdvanceGhosts = bAdvancePac;
	bool bAdvanceDots = bAdvancePac;

	if (bAdvancePac && _state == GS_PLAYING && _pac.IsStuck())
	{
		bAdvancePac = false;
	}

	_renderMaze->Advance(bAdvancePac, bAdvanceGhosts, bAdvanceDots, this);
}

void PlayingMaze::AdvanceActors()
{
	UpdateGhostMode();
	UpdatePointDisplays();

	if (!_ghostEatenCountdown)
	{
		UpdateGhostHouse();
		UpdateFruitMode();
		UpdateActorSpeeds();
	}

	// Move ghosts

	for (size_t nGhost = 0; nGhost < _countof(_ghosts); nGhost++)
	{
		GhostActor &ghost = _ghosts[nGhost];

		if (ghost.IsActive())
		{
			for (size_t i = ghost.GetAdvanceCount(); i > 0; i--)
			{
				// Ghost eyes can still move when another ghost was just eaten

				if (!_ghostEatenCountdown || ghost.GetMoveState() == MOVE_EYES)
				{
					AdvanceGhost(ghost);
				}
			}
		}
	}

	if (!_ghostEatenCountdown)
	{
		// Move fruit

		for (size_t i = _fruit.GetAdvanceCount(); i > 0; i--)
		{
			AdvanceFruit(_fruit);
		}

		// Move Pac

		for (size_t i = _pac.GetAdvanceCount(); i > 0; i--)
		{
			AdvancePac(_pac);

			if (i > 1)
			{
				CheckPacCollisions(_pac);
			}
		}

		AdvanceCustomActors();
		CheckPacCollisions(_pac);

#ifdef _DEBUG
		if (_pac.IsActive() && ff::MetroGlobals::Get()->GetKeys()->GetKey('4'))
		{
			_stats._cheated = true;

			// Cheat to get Pac to move much faster
			AdvancePac(_pac);
			CheckPacCollisions(_pac);
		}
#endif
	}
}

void PlayingMaze::AdvancePac(PacActor &pac)
{
	// Cache info about pac

	ff::PointInt dir = pac.GetDir();
	ff::PointInt tile = pac.GetTile();
	ff::PointInt pixel = pac.GetPixel();
	ff::PointInt press = pac.GetPressDir();
	ff::PointInt center = TileCenterToPixel(tile);

	// Check if pac's direction should change

	if (dir.x && press.x == -dir.x)
	{
		dir.x = -dir.x;
	}
	else if (dir.y && press.y == -dir.y)
	{
		dir.y = -dir.y;
	}
	else if (pac.CanTurn() && _maze->GetTileZone(tile) != ZONE_OUT_OF_BOUNDS)
	{
		if (dir.x && press.y && pixel.y == center.y && !HitWall(tile, ff::PointInt(0, press.y)))
		{
			dir.SetPoint(0, press.y);
		}
		else if (dir.y && press.x && pixel.x == center.x && !HitWall(tile, ff::PointInt(press.x, 0)))
		{
			dir.SetPoint(press.x, 0);
		}

		if (dir != pac.GetDir())
		{
			// Once a turn starts, another turn can't start until the
			// user moves the joystick or until the turn is complete.

			pac.SetCanTurn(false);
		}
	}

	// Gravitate towards the center line (when cornering)

	if (dir.x && pixel.y != center.y)
	{
		pixel.y += Sign(center.y - pixel.y);
	}
	else if (dir.y && pixel.x != center.x)
	{
		pixel.x += Sign(center.x - pixel.x);
	}

	// Move pac if he hasn't hit a wall

	if (((dir.x && pixel.x == center.x) || (dir.y && pixel.y == center.y)) &&
		HitWall(tile, dir))
	{
		pac.SetStuck(true);
	}
	else
	{
		pac.SetStuck(false);

		pixel += dir;
	}

	pac.SetDir(dir);
	pac.SetPixel(pixel);

	if (CheckTunnel(pac))
	{
		_stats._tunnelsUsed++;
	}
}

void PlayingMaze::AdvanceGhost(GhostActor &ghost)
{
	// Cache info about the ghost

	ff::PointInt dir = ghost.GetDir();
	ff::PointInt tile = ghost.GetTile();
	ff::PointInt pixel = ghost.GetPixel();
	ff::PointInt press = ghost.GetPressDir();
	ff::PointInt center = TileCenterToPixel(tile);

	if (ghost.GetHouseState() == HOUSE_INSIDE)
	{
		if (ghost.GetMoveState() == MOVE_EYES)
		{
			// Move towards the bottom of the house before leaving

			ff::PointInt target(_ghostStartPixel.x, _ghostHouseRect.bottom);

			if (GhostIndex(ghost) == 2)
			{
				target.x -= PixelsPerTile().x * 2;
			}
			else if (GhostIndex(ghost) == 3)
			{
				target.x += PixelsPerTile().x * 2;
			}

			if (pixel.y < target.y)
			{
				dir.SetPoint(0, 1);
			}
			else if (pixel.x != target.x)
			{
				dir.SetPoint(Sign(target.x - pixel.x), 0);
			}
			else
			{
				dir.SetPoint(0, -1);
				ghost.SetMoveState(MOVE_NORMAL);
				ghost.SetHouseState(HOUSE_INSIDE);
			}
		}
		else if (pixel.y == _ghostHouseRect.top || pixel.y == _ghostHouseRect.bottom)
		{
			// Move up and down while in the house

			dir.y = -dir.y;
		}
	}
	else if (ghost.GetHouseState() == HOUSE_LEAVING)
	{
		// Move towards the position above the ghost door

		if (pixel.x != _ghostStartPixel.x)
		{
			dir.SetPoint(Sign(_ghostStartPixel.x - pixel.x), 0);
		}
		else if (pixel.y > _ghostStartPixel.y)
		{
			dir.SetPoint(0, -1);
		}
		else
		{
			ghost.SetHouseState(HOUSE_OUTSIDE);
			dir.SetPoint(-1, 0);
		}
	}
	else if (ghost.GetMoveState() == MOVE_EYES && pixel == _ghostStartPixel)
	{
		// Enter the ghost house

		ghost.SetHouseState(HOUSE_INSIDE);
	}
	else if (pixel == center)
	{
		// Check if the direction should change

		if (press.x || press.y)
		{
			ghost.SetDir(dir = press);
			press.SetPoint(0, 0);
		}

		press = GhostDecidePress(ghost);
	}

	pixel += dir;

	ghost.SetDir(dir);
	ghost.SetPressDir(press);
	ghost.SetPixel(pixel);

	// Check for warp tunnel usage

	CheckTunnel(ghost);
}

void PlayingMaze::AdvanceFruit(FruitActor &fruit)
{
	ff::PointInt dir = fruit.GetDir();

	if (dir.x || dir.y)
	{
		ff::PointInt tile = fruit.GetTile();
		ff::PointInt pixel = fruit.GetPixel();
		ff::PointInt center = TileCenterToPixel(tile);

		if (pixel == center)
		{
			dir = FruitDecideDir(fruit);
			PlayEffect(EFFECT_FRUIT_BOUNCE);
		}

		pixel += dir;

		fruit.SetDir(dir);
		fruit.SetPixel(pixel);

		if (CheckTunnel(fruit))
		{
			fruit.Reset();
		}
	}
}

void PlayingMaze::AdvanceCustomActors()
{
	for (size_t i = ff::PreviousSize(_customs.Size()); i != ff::INVALID_SIZE; i = ff::PreviousSize(i))
	{
		CustomActor *actor = _customs[i];

		if (!actor->Advance())
		{
			actor->DeleteThis();
			_customs.Delete(i);
		}
	}
}

void PlayingMaze::UpdatePointDisplays()
{
	for (size_t i = ff::PreviousSize(_points.Size()); i != ff::INVALID_SIZE; i = ff::PreviousSize(i))
	{
		PointActor &point = _points[i];

		if (!point.AdvanceCountdown())
		{
			_points.Delete(i);
		}
	}
}

void PlayingMaze::UpdateFruitMode()
{
	if (_nFruitCounter)
	{
		_nFruitCounter--;
	}
	else if (!_fruit.IsActive() &&
		_nCurrentFruit < _countof(_nFruitDots) &&
		_dotCount && _dotCount == _nFruitDots[_nCurrentFruit] &&
		_difficulty.GetFruit() != FRUIT_NONE)
	{
		_nCurrentFruit++;
		_nFruitCounter = _difficulty.GetFruitFrames(_maze->GetCharType());

		_fruit.SetActive(true);

		if (_difficulty.IsFruitMoving(_maze->GetCharType()) && _fruitStartTiles.Size())
		{
			// Set the start
			size_t nFruitStart = (rand() % _fruitStartTiles.Size());

			_fruit.SetPixel(TileCenterToPixel(_fruitStartTiles[nFruitStart].first));
			_fruit.SetDir(_fruitStartTiles[nFruitStart].second);

			// Set the end
			_fruit.SetExitTile(_fruitStartTiles[rand() % _fruitStartTiles.Size()].first);
		}
		else
		{
			_fruit.SetPixel(_fruitPixel);
			_fruit.SetDir(ff::PointInt(0, 0));
		}

		FruitType type = _difficulty.GetFruit();

		if (type == FRUIT_RANDOM)
		{
			type = (FruitType)(rand() % (FRUIT_RANDOM - FRUIT_0) + FRUIT_0);
		}

		_fruit.SetType(type);
	}
	
	if (!_nFruitCounter && _fruit.IsActive() && _fruit.GetDir() == ff::PointInt(0, 0))
	{
		_fruit.Reset();
	}
}

void PlayingMaze::UpdateGhostMode()
{
	if (_ghostEatenCountdown)
	{
		if (!--_ghostEatenCountdown)
		{
			// Reactivate the ghost that was just eaten

			for (size_t i = 0; i < _countof(_ghosts); i++)
			{
				GhostActor &ghost = _ghosts[i];

				if (ghost.IsActive() && ghost.GetMoveState() == MOVE_EATEN)
				{
					ghost.SetMoveState(MOVE_EYES);
				}
			}
		}
	}
	else if (_ghostScaredCountdown)
	{
		if (!--_ghostScaredCountdown)
		{
			ScareGhosts(false);
		}
	}
	else if (_ghostScatterCountdown)
	{
		if (!--_ghostScatterCountdown)
		{
			FlipGhosts();
		}
	}
	else if (_ghostChaseCountdown)
	{
		if (!--_ghostChaseCountdown)
		{
			FlipGhosts();

			UpdateScatterChaseTimes(_ghostScatterChaseIndex + 1);
		}
	}

	assert(_ghostScatterCountdown || _ghostChaseCountdown);
}

void PlayingMaze::UpdateGhostHouse()
{
	// The red ghost can never stay inside the house

	if (_ghosts[0].IsActive() &&
		_ghosts[0].GetMoveState() == MOVE_NORMAL &&
		_ghosts[0].GetHouseState() == HOUSE_INSIDE)
	{
		_ghosts[0].SetHouseState(HOUSE_LEAVING);
	}

	// check if a new ghost should be released

	_lastDotCounter++;

	if (_lastDotCounter > _difficulty.GetLastDotFrames())
	{
		ReleaseGhost();
	}
	else if (_globalDotIndex)
	{
		// This logic here might seem dumb, but it matches the real game

		if (_globalDotCounter >= _difficulty.GetGlobalDotCounter(_globalDotIndex) &&
			CountGhostsInHouse()) // this is the dumb part
		{
			ReleaseGhost();

			_globalDotIndex++;

			if (!CountGhostsInHouse())
			{
				_globalDotIndex = 0;
			}
		}
	}
	else
	{
		for (size_t i = 0; i < _countof(_ghosts); i++)
		{
			GhostActor &ghost = _ghosts[i];

			if (ghost.IsActive() &&
				ghost.GetMoveState() == MOVE_NORMAL &&
				ghost.GetHouseState() == HOUSE_INSIDE &&
				!ghost.GetDotCounter())
			{
				ReleaseGhost();
				break;
			}
		}
	}
}

void PlayingMaze::UpdateActorSpeeds()
{
	_pac.SetSpeed(_difficulty.GetPacSpeed(_ghostScaredCountdown != 0));

	_fruit.SetSpeed(_difficulty.GetFruitSpeed());

	for (size_t i = 0; i < _countof(_ghosts); i++)
	{
		GhostActor &ghost = _ghosts[i];

		if (ghost.IsActive())
		{
			TileZone zone = _maze->GetTileZone(ghost.GetTile());
			bool bTunnel = (zone == ZONE_GHOST_SLOW || zone == ZONE_OUT_OF_BOUNDS);

			size_t speed = _difficulty.GetGhostSpeed(
				ghost.GetMoveState(),
				ghost.GetHouseState(),
				bTunnel,
				!i, _dotCount);

			ghost.SetSpeed(speed);
		}
	}
}

void PlayingMaze::UpdateScatterChaseTimes(size_t nIndex)
{
	_ghostScatterChaseIndex = nIndex;

	_difficulty.GetGhostModeFrames(
		_ghostScatterChaseIndex,
		_ghostScatterCountdown,
		_ghostChaseCountdown);
}

static const size_t s_nMaxCollideDist =
	((PixelsPerTile().x / 2) * (PixelsPerTile().x / 2)) +
	((PixelsPerTile().y / 2) * (PixelsPerTile().y / 2));

static bool ActorsCollide(PlayingActor &actor1, PlayingActor &actor2)
{
	if (actor1.IsActive() && actor2.IsActive())
	{
		ff::PointInt dist = (actor2.GetPixel() - actor1.GetPixel());
		size_t nDist = dist.x * dist.x + dist.y * dist.y;

		if (nDist < s_nMaxCollideDist)
		{
			return true;
		}
	}

	return false;
}

void PlayingMaze::CheckPacCollisions(PacActor &pac)
{
	// Check for eating fruit

	if (ActorsCollide(pac, _fruit))
	{
		OnPacEatFruit(pac, _fruit);
	}

	// Check for eating a dot

	ff::PointInt pacTile = pac.GetTile();
	TileContent content = _maze ? _maze->GetTileContent(pacTile) : CONTENT_NOTHING;

	if (content == CONTENT_DOT || content == CONTENT_POWER)
	{
		_maze->SetTileContent(pacTile, CONTENT_NOTHING);

		OnPacEatDot(pac, content == CONTENT_POWER);
	}

	// Check for ghost collisions

	if (_state != GS_WINNING && // Can't eat the last dot and a ghost at the same time
		!_ghostEatenCountdown) // Can't eat two ghosts at once
	{
		for (size_t i = 0; i < _countof(_ghosts); i++)
		{
			GhostActor &ghost = _ghosts[i];

			if (ghost.IsActive())
			{
				bool bCollide = ActorsCollide(pac, ghost);

				if (bCollide && ghost.GetMoveState() == MOVE_NORMAL)
				{
					OnGhostEatPac(pac, ghost);
					break;
				}
				else if ((bCollide && ghost.GetMoveState() == MOVE_SCARED) ||
					ghost.GetMoveState() == MOVE_WAITING_TO_BE_EATEN)
				{
					if (!_ghostEatenCountdown)
					{
						OnPacEatGhost(pac, ghost);
					}
					else
					{
						ghost.SetMoveState(MOVE_WAITING_TO_BE_EATEN);
					}
				}
			}
		}
	}
}

bool PlayingMaze::CheckTunnel(PlayingActor &actor)
{
	ff::PointInt dir = actor.GetDir();
	ff::PointInt tile = actor.GetTile();
	ff::PointInt pixel = actor.GetPixel();
	bool bMoved = false;

	if (dir.x < 0 && tile.x < -1)
	{
		pixel.x = TileBottomRightToPixel(_maze->GetSizeInTiles()).x;
		bMoved = true;
	}
	else if (dir.x > 0 && tile.x > _maze->GetSizeInTiles().x)
	{
		pixel.x = -PixelsPerTile().x;
		bMoved = true;
	}
	else if (dir.y < 0 && tile.y < -1)
	{
		pixel.y = TileBottomRightToPixel(_maze->GetSizeInTiles()).y;
		bMoved = true;
	}
	else if (dir.y > 0 && tile.y > _maze->GetSizeInTiles().y)
	{
		pixel.y = -PixelsPerTile().y;
		bMoved = true;
	}

	if (bMoved)
	{
		if (_host && &actor == &_pac)
		{
			_host->OnPacUsingTunnel();
		}

		actor.SetPixel(pixel);
	}

	return bMoved;
}

bool PlayingMaze::CheckGhostsScared(bool &scared, bool &eyes) const
{
	scared = false;
	eyes = false;

	for (size_t i = 0; i < _countof(_ghosts); i++)
	{
		const GhostActor &ghost = _ghosts[i];

		if (ghost.IsActive())
		{
			scared |= (ghost.GetMoveState() == MOVE_SCARED ||
				ghost.GetMoveState() == MOVE_EATEN ||
				ghost.GetMoveState() == MOVE_WAITING_TO_BE_EATEN);

			eyes |= (ghost.GetMoveState() == MOVE_EYES);
		}
	}

	return scared || eyes;
}

void PlayingMaze::OnPacEatDot(PacActor &pac, bool bPower)
{
	_lastDotCounter = 0;

#ifdef _DEBUG
	if (ff::MetroGlobals::Get()->GetKeys()->GetKey('6'))
	{
		_stats._cheated = true;
		_dotCount = 1;
	}
#endif

	if (_dotCount > 0 && !--_dotCount)
	{
		// Ate the last dot!

		SetGameState(GS_WINNING);
	}

	if (_globalDotIndex)
	{
		_globalDotCounter++;
	}
	else
	{
		for (size_t i = 0; i < _countof(_ghosts); i++)
		{
			GhostActor &ghost = _ghosts[i];

			if (ghost.IsActive() &&
				ghost.GetMoveState() == MOVE_NORMAL &&
				ghost.GetHouseState() == HOUSE_INSIDE &&
				ghost.GetDotCounter())
			{
				ghost.SetDotCounter(ghost.GetDotCounter() - 1);
				break;
			}
		}
	}

	if (bPower)
	{
		AddPoints(_difficulty.GetPowerPoints());

		ScareGhosts(true);

		pac.AddDelay(3);

		_stats._powerEaten++;
	}
	else
	{
		AddPoints(_difficulty.GetDotPoints());

		pac.AddDelay(1);

		_stats._dotsEaten++;
	}

	if (GetGameState() == GS_WINNING)
	{
		PlayEffect(EFFECT_LEVEL_WIN);
	}
	else
	{
		PlayEffect(!_dotEffect ? EFFECT_EAT_DOT1 : EFFECT_EAT_DOT2);
		_dotEffect = _dotEffect ? 0 : 1;
	}

	AddBubble(pac,
		bPower ? POWER_BUBBLE_COUNT : DOT_BUBBLE_COUNT,
		bPower ? POWER_BUBBLE_SPREAD : DOT_BUBBLE_SPREAD);
}

void PlayingMaze::OnPacEatFruit(PacActor &pac, FruitActor &fruit)
{
	_nFruitCounter = 0;
	_stats._fruitsEaten++;

	size_t nPoints = _difficulty.GetFruitPoints(_fruit.GetType());
	AddPoints(nPoints);

	AddPointDisplay(
		_fruit.GetPixel(),
		ff::PointFloat(1, 1),
		nPoints,
		120,
		s_fruitPointsTextColor,
		false);

	PlayEffect(EFFECT_EAT_FRUIT);
	AddBubble(fruit, FRUIT_BUBBLE_COUNT, FRUIT_BUBBLE_SPREAD);

	fruit.Reset();
}

void PlayingMaze::OnPacEatGhost(PacActor &pac, GhostActor &ghost)
{
	ghost.SetMoveState(MOVE_EATEN);

	_ghostEatenCountdown = _difficulty.GetEatenFrames();
	_stats._ghostsEaten[GhostIndex(ghost)]++;

	size_t nPoints = _difficulty.GetGhostPoints(_ghostEatenIndex);
	AddPoints(nPoints);

	AddPointDisplay(
		ghost.GetPixel(),
		ff::PointFloat(1, 1),
		nPoints,
		_ghostEatenCountdown,
		s_ghostPointsTextColor, false);

	_ghostEatenIndex++;

	PlayEffect(EFFECT_EAT_GHOST);
	AddBubble(ghost, GHOST_BUBBLE_COUNT, GHOST_BUBBLE_SPREAD);
}

void PlayingMaze::OnGhostEatPac(PacActor &pac, GhostActor &ghost)
{
#ifdef _DEBUG
	if (ff::MetroGlobals::Get()->GetKeys()->GetKey('3'))
	{
		_stats._cheated = true;
		return;
	}
#endif

	_stats._ghostDeathCount[GhostIndex(ghost)]++;

	SetGameState(GS_CAUGHT);
}

size_t PlayingMaze::GhostIndex(GhostActor &ghost)
{
	for (size_t i = 0 ; i < _countof(_ghosts); i++)
	{
		if (&ghost == &_ghosts[i])
		{
			return i;
		}
	}

	assert(false);
	return 0;
}

ff::PointInt PlayingMaze::GhostDecidePress(GhostActor &ghost)
{
	return GhostDecidePress(
		ghost.GetBrains(),
		ghost.GetMoveState(),
		ghost.GetTile() + ghost.GetDir(),
		ghost.GetDir());
}

ff::PointInt PlayingMaze::GhostDecidePress(IGhostBrains *brains, MoveState move, ff::PointInt tile, ff::PointInt dir)
{
	ff::PointInt press(0, 0);
	ff::PointInt prevTile = tile - dir;
	TileZone zone = _maze->GetTileZone(tile);

	bool bAllowTurn = (zone != ZONE_OUT_OF_BOUNDS);

	if (bAllowTurn && move == MOVE_NORMAL)
	{
		// Can't turn while moving horizontally into a "no turn" zone

		bAllowTurn = (!dir.x || zone != ZONE_GHOST_NO_TURN);
	}

	if (bAllowTurn)
	{
		ff::PointInt leftTile = tile + ff::PointInt(-1, 0);
		ff::PointInt rightTile = tile + ff::PointInt( 1, 0);
		ff::PointInt upTile = tile + ff::PointInt( 0, -1);
		ff::PointInt downTile = tile + ff::PointInt( 0, 1);

		// Create a list of test tiles in priority order

		ff::Vector<ff::PointInt, 4> tiles;

		if (upTile != prevTile && !IsWall(upTile))
		{
			tiles.Push(upTile);
		}

		if (leftTile != prevTile && !IsWall(leftTile))
		{
			tiles.Push(leftTile);
		}

		if (downTile != prevTile && !IsWall(downTile))
		{
			tiles.Push(downTile);
		}

		if (rightTile != prevTile && !IsWall(rightTile))
		{
			tiles.Push(rightTile);
		}

		if (!tiles.Size())
		{
			// Dead end, this level is stupid

			press = -dir;
		}
		else if (tiles.Size() == 1)
		{
			// Only one choice

			press = tiles[0] - tile;
		}
		else if (brains)
		{
			// Need to use brains to decide which way to go

			ff::PointInt targetTile = brains->Decide(this, tiles.Data(), tiles.Size());

			if (tiles.Find(targetTile) != ff::INVALID_SIZE)
			{
				press = targetTile - tile;
			}
			else
			{
				// Poorly design AI, doesn't do what I ask of it

				press = tiles[rand() % tiles.Size()] - tile;
			}
		}
		else
		{
			// No brains, just use random selection

			press = tiles[rand() % tiles.Size()] - tile;
		}
	}

	if (press == dir)
	{
		press.SetPoint(0, 0);
	}

	return press;
}

void PlayingMaze::FlipGhosts()
{
	for (size_t i = 0; i < _countof(_ghosts); i++)
	{
		GhostActor &ghost = _ghosts[i];

		if (ghost.IsActive() &&
			ghost.GetMoveState() != MOVE_EYES &&
			ghost.GetMoveState() != MOVE_EATEN &&
			ghost.GetMoveState() != MOVE_WAITING_TO_BE_EATEN &&
			ghost.GetHouseState() == HOUSE_OUTSIDE)
		{
			ff::PointInt dir = ghost.GetDir();
			ghost.SetPressDir(-dir);
		}
	}
}

void PlayingMaze::ScareGhosts(bool bScared)
{
	_ghostEatenIndex = 0;

	if (bScared)
	{
		FlipGhosts();

		_ghostScaredCountdown = _difficulty.GetScaredFrames();
	}

	if (!bScared || _ghostScaredCountdown)
	{
		for (size_t i = 0; i < _countof(_ghosts); i++)
		{
			GhostActor &ghost = _ghosts[i];

			if (ghost.IsActive())
			{
				if (bScared && ghost.GetMoveState() == MOVE_NORMAL)
				{
					ghost.SetMoveState(MOVE_SCARED);
				}
				else if (!bScared &&
					ghost.GetMoveState() != MOVE_EYES &&
					ghost.GetMoveState() != MOVE_EATEN &&
					ghost.GetMoveState() != MOVE_WAITING_TO_BE_EATEN)
				{
					ghost.SetMoveState(MOVE_NORMAL);
				}
			}
		}
	}
}

bool PlayingMaze::ReleaseGhost()
{
	for (size_t i = 0; i < _countof(_ghosts); i++)
	{
		GhostActor &ghost = _ghosts[i];

		if (ghost.IsActive() &&
			ghost.GetMoveState() == MOVE_NORMAL &&
			ghost.GetHouseState() == HOUSE_INSIDE)
		{
			ghost.SetHouseState(HOUSE_LEAVING);

			_lastDotCounter = 0;

			return true;
		}
	}

	return false;
}

size_t PlayingMaze::CountGhostsInHouse()
{
	size_t nCount = 0;

	for (size_t i = 0; i < _countof(_ghosts); i++)
	{
		GhostActor &ghost = _ghosts[i];

		if (ghost.IsActive() &&
			ghost.GetMoveState() == MOVE_NORMAL &&
			ghost.GetHouseState() == HOUSE_INSIDE)
		{
			nCount++;
		}
	}

	return nCount;
}

class __declspec(uuid("f821b73f-f234-4830-9884-dca48abaec1a"))
	CFruitBrains : public ff::ComBase, public IGhostBrains
{
public:
	DECLARE_HEADER(CFruitBrains);

	void Init(bool bExiting, ff::PointInt exitTile);

	virtual ff::PointInt Decide(IPlayingMaze *pPlay, const ff::PointInt *pTiles, size_t nTiles) override;
	virtual ff::PointInt GetTargetPixel(IPlayingMaze *pPlay) override;

private:
	ff::Set<std::pair<ff::PointInt, ff::PointInt>> _chosenRandom;
	ff::Set<std::pair<ff::PointInt, ff::PointInt>> _chosenExit;
};

BEGIN_INTERFACES(CFruitBrains)
	HAS_INTERFACE(IGhostBrains)
END_INTERFACES()

CFruitBrains::CFruitBrains()
{
}

CFruitBrains::~CFruitBrains()
{
}

ff::PointInt CFruitBrains::Decide(IPlayingMaze *pPlay, const ff::PointInt *pTiles, size_t nTiles)
{
	assertRetVal(pPlay && pTiles && nTiles, ff::PointInt(0, 0));

	ff::Vector<ff::PointInt> tiles;
	tiles.Push(pTiles, nTiles);

	while (tiles.Size())
	{
		bool bExiting = (pPlay->GetFruitState() == FRUIT_EXITING);

		// Either go towards the exit, or pick a random tile
		ff::PointInt tile = bExiting
			? DecideForTarget(GetTargetPixel(pPlay), tiles.Data(), tiles.Size())
			: tiles[rand() % tiles.Size()];

		std::pair<ff::PointInt, ff::PointInt> choice(pPlay->GetFruit()->GetTile(), tile);

		// See if the current choice has been made already
		ff::Set<std::pair<ff::PointInt, ff::PointInt>> &chosen = bExiting ? _chosenExit : _chosenRandom;

		if (chosen.Exists(choice) ||
			(!bExiting && pPlay->GetMaze()->GetTileZone(tile) == ZONE_GHOST_SLOW))
		{
			// Bad choice, try again

			tiles.Delete(tiles.Find(tile));
		}
		else
		{
			chosen.SetKey(choice);

			return tile;
		}
	}

	// All choices have been made already, fall back to a random tile

	return pTiles[rand() % nTiles];
}

ff::PointInt CFruitBrains::GetTargetPixel(IPlayingMaze *pPlay)
{
	assertRetVal(pPlay, ff::PointInt(0, 0));

	return TileCenterToPixel(pPlay->GetFruitExitTile());
}

ff::PointInt PlayingMaze::FruitDecideDir(FruitActor &fruit)
{
	if (!fruit.GetBrains())
	{
		CFruitBrains *brains = new ff::ComObject<CFruitBrains>();
		fruit.SetBrains(brains);
	}

	ff::PointInt dir = GhostDecidePress(fruit.GetBrains(), MOVE_SCARED, fruit.GetTile(), fruit.GetDir());

	return (dir.x || dir.y) ? dir : fruit.GetDir();
}

bool PlayingMaze::IsWall(ff::PointInt tile)
{
	TileContent content = _maze->GetTileContent(tile);

	return
		content == CONTENT_WALL ||
		content == CONTENT_GHOST_WALL ||
		content == CONTENT_GHOST_DOOR;
}

bool PlayingMaze::HitWall(ff::PointInt tile, ff::PointInt dir)
{
	return IsWall(tile + dir);
}

void PlayingMaze::Render(ff::I2dRenderer *pRenderer)
{
	assertRet(pRenderer);

	bool bRenderPac = (_state >= GS_READY && !_ghostEatenCountdown);
	bool bRenderGhosts = (_state >= GS_READY && (_state <= GS_CAUGHT));
	bool bRenderCustom = bRenderGhosts;

	// Render the maze
	{
		_renderMaze->RenderBackground(pRenderer);

		if (_state != GS_WON && (_state != GS_WINNING || _stateCounter < 240))
		{
			_renderMaze->RenderTheMaze(pRenderer);
		}

		if (_state != GS_DIED && (_state != GS_DEAD || _stateCounter < 20))
		{
			_renderMaze->RenderDots(pRenderer);
		}

		_renderMaze->RenderActors(pRenderer, bRenderPac, bRenderGhosts, bRenderCustom, this);

		if (_state == GS_PLAYING)
		{
			_renderMaze->RenderPoints(pRenderer, this);
		}

#ifdef _DEBUG
		if (ff::MetroGlobals::Get()->GetKeys()->GetKey('2'))
		{
			_stats._cheated = true;

			if (bRenderPac && bRenderGhosts)
			{
				RenderDebugGhostPaths(pRenderer);
			}
		}
#endif
	}
}

void PlayingMaze::Reset()
{
	// Reset the state of any animations

	_renderMaze->Reset();

	// Reset each actor
	{
		_pac.Reset();
		_fruit.Reset();

		for (size_t i = 0; i < _countof(_ghosts); i++)
		{
			_ghosts[i].Reset();
		}

		for (CustomActor *actor : _customs)
		{
			delete actor;
		}

		_points.Clear();
		_customs.Clear();
	}

	_lastDotCounter = 0;
	_globalDotCounter = 0;
	_globalDotIndex = 1; // very important!
	_ghostScaredCountdown = 0;
	_ghostEatenCountdown = 0;
	_ghostEatenIndex = 0;
	_ghostScatterChaseIndex = 0;
	_nFruitCounter = 0;

	UpdateScatterChaseTimes(_ghostScatterChaseIndex);

	InitActorPositions();

	SetGameState(GS_READY);
}

void PlayingMaze::RenderDebugGhostPaths(ff::I2dRenderer *pRenderer)
{
#ifdef _DEBUG
	assertRet(pRenderer);

	for (size_t i = 0; i < _countof(_ghosts); i++)
	{
		GhostActor &ghost = _ghosts[i];

		if (ghost.IsActive() &&
			ghost.GetMoveState() == MOVE_NORMAL &&
			ghost.GetHouseState() == HOUSE_OUTSIDE &&
			ghost.GetBrains())
		{
			ff::PointInt tile = ghost.GetTile();
			ff::PointInt dir = ghost.GetDir();
			ff::PointFloat lineOffset = ff::PointFloat((float)i - 2, (float)i - 2);

			ff::PointFloat points[35];
			points[0] = TileCenterToPixelF(tile) + lineOffset;

			for (size_t h = 1; h < _countof(points); h++)
			{
				ff::PointInt press = GhostDecidePress(ghost.GetBrains(), ghost.GetMoveState(), tile, dir);

				if (press.x || press.y)
				{
					dir = press;
				}

				tile += dir;
				points[h] = TileCenterToPixelF(tile) + lineOffset;
			}

			DirectX::XMFLOAT4 color(1, 1, 1, 1);
			color.w = 0.5f;

			ff::PointInt target = ghost.GetBrains()->GetTargetPixel(this);

			target.x = std::max(target.x, 0);
			target.x = std::min(target.x, TileTopLeftToPixel(_maze->GetSizeInTiles()).x);

			target.y = std::max(target.y, 0);
			target.y = std::min(target.y, TileTopLeftToPixel(_maze->GetSizeInTiles()).y);

			ff::PointFloat targetF((float)target.x, (float)target.y);

			pRenderer->DrawFilledRectangle(
				&ff::RectFloat(targetF - PixelsPerTileF() / 2.0f, targetF + PixelsPerTileF() / 2.0f),
				&color, 1);

			for (size_t h = 0; h < _countof(points) - 1; h++)
			{
				DirectX::XMStoreFloat4(&color,
					DirectX::XMVectorSubtract(
						DirectX::XMLoadFloat4(&color),
						DirectX::XMVectorSet(0, 0, 0, 0.019231f)));

				pRenderer->DrawLine(&points[h], &points[h + 1], &color);
			}
		}
	}

#endif
}

GameState PlayingMaze::GetGameState() const
{
	return _state;
}

IMaze *PlayingMaze::GetMaze() const
{
	return _maze;
}

const Stats &PlayingMaze::GetStats() const
{
	return _stats;
}

PacState PlayingMaze::GetPacState() const
{
	if (!_pac.IsActive())
	{
		return PAC_INVALID;
	}

	if (_state == GS_DYING)
	{
		return PAC_DYING;
	}

	if (_state == GS_DEAD || _state == GS_DIED)
	{
		return PAC_DEAD;
	}

	return PAC_NORMAL;
}

IPlayingActor *PlayingMaze::GetPac()
{
	return &_pac;
}

CharType PlayingMaze::GetCharType() const
{
	return _maze ? _maze->GetCharType() : CHAR_DEFAULT;
}

bool PlayingMaze::IsPowerPac() const
{
	bool scared, eyes;
	return CheckGhostsScared(scared, eyes) && scared;
}

size_t PlayingMaze::GetGhostCount() const
{
	return _countof(_ghosts);
}

ff::PointInt PlayingMaze::GetGhostEyeDir(size_t nGhost) const
{
	assertRetVal(nGhost >= 0 && nGhost < GetGhostCount(), ff::PointInt(0, 0));

	const GhostActor &ghost = _ghosts[nGhost];

	if (ghost.GetHouseState() != HOUSE_OUTSIDE)
	{
		return ghost.GetDir();
	}
	else
	{
		ff::PointInt press = _ghosts[nGhost].GetPressDir();

		return (!press.x && !press.y)
			? _ghosts[nGhost].GetDir()
			: press;
	}
}

GhostState PlayingMaze::GetGhostState(size_t nGhost) const
{
	assertRetVal(nGhost >= 0 && nGhost < GetGhostCount(), GHOST_INVALID);

	const GhostActor &ghost = _ghosts[nGhost];

	if (!ghost.IsActive() ||
		ghost.GetMoveState() == MOVE_EATEN ||
		ghost.GetMoveState() == MOVE_WAITING_TO_BE_EATEN)
	{
		return GHOST_INVALID;
	}
	else if (ghost.GetMoveState() == MOVE_SCARED)
	{
		return (_ghostScaredCountdown < 122 && (_ghostScaredCountdown % 27) < 14)
			? GHOST_SCARED_FLASH
			: GHOST_SCARED;
	}
	else if (ghost.GetMoveState() == MOVE_EYES)
	{
		return GHOST_EYES;
	}
	else
	{
		return (!_ghostScatterCountdown && _pac.IsActive())
			? GHOST_CHASE
			: GHOST_SCATTER;
	}
}

IPlayingActor *PlayingMaze::GetGhost(size_t nGhost)
{
	assertRetVal(nGhost >= 0 && nGhost < GetGhostCount(), nullptr);

	return &_ghosts[nGhost];
}

ff::PointInt PlayingMaze::GetGhostStartPixel() const
{
	return _ghostStartPixel;
}

ff::PointInt PlayingMaze::GetGhostStartTile() const
{
	return _ghostStartTile;
}

FruitState PlayingMaze::GetFruitState() const
{
	if (!_fruit.IsActive())
	{
		return FRUIT_INVALID;
	}
	else if (!_nFruitCounter)
	{
		return FRUIT_EXITING;
	}
	else
	{
		return FRUIT_NORMAL;
	}
}

IPlayingActor *PlayingMaze::GetFruit()
{
	return &_fruit;
}

FruitType PlayingMaze::GetFruitType() const
{
	assertRetVal(GetFruitState() != FRUIT_INVALID, FRUIT_NONE);

	return _fruit.GetType();
}

ff::PointInt PlayingMaze::GetFruitExitTile() const
{
	assertRetVal(GetFruitState() == FRUIT_EXITING, ff::PointInt(0, 0));

	return _fruit.GetExitTile();
}

const PointActor *PlayingMaze::GetPointDisplays(size_t &nCount) const
{
	nCount = _points.Size();

	return nCount ? _points.Data() : nullptr;
}

CustomActor * const *PlayingMaze::GetCustomActors(size_t &nCount) const
{
	nCount = _customs.Size();

	return nCount ? _customs.ConstData() : nullptr;
}
