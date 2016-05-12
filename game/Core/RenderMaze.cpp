#include "pch.h"
#include "COM/ComObject.h"
#include "Core/Actors.h"
#include "Core/GlobalResources.h"
#include "Core/Helpers.h"
#include "Core/Maze.h"
#include "Core/PlayingMaze.h"
#include "Core/RenderMaze.h"
#include "Core/RenderText.h"
#include "Core/Tiles.h"
#include "Graph/Anim/AnimKeys.h"
#include "Graph/Anim/AnimPos.h"
#include "Graph/2D/SpriteAnimation.h"
#include "Graph/2D/2dRenderer.h"
#include "Graph/2D/Sprite.h"
#include "Graph/2D/SpriteList.h"
#include "Module/Module.h"
#include "Resource/ResourceValue.h"

struct WallDefine
{
	size_t _index;
	wchar_t *_tiles;
};

class __declspec(uuid("8f252e27-a2ee-43c0-bee7-6600a48fabff"))
	RenderMaze
		: public ff::ComBase
		, public IRenderMaze
		, public IMazeListener
{
public:
	DECLARE_HEADER(RenderMaze);

	bool Init(IMaze *pMaze);

	// IRenderMaze
	virtual void Reset() override;
	virtual void Advance(bool bPac, bool bGhosts, bool bDots, IPlayingMaze *pPlay) override;
	virtual void RenderBackground(ff::I2dRenderer *pRenderer) override;
	virtual void RenderTheMaze(ff::I2dRenderer *pRenderer) override;
	virtual void RenderDots(ff::I2dRenderer *pRenderer) override;
	virtual void RenderActors(ff::I2dRenderer *pRenderer, bool bPac, bool bGhosts, bool bCustom, IPlayingMaze *pPlay) override;
	virtual void RenderPoints(ff::I2dRenderer *pRenderer, IPlayingMaze *pPlay) override;
	virtual void RenderFreeLives(ff::I2dRenderer *pRenderer, IPlayingMaze *play, size_t nLives, ff::PointFloat leftPixel) override;
	virtual void RenderStatusFruits(ff::I2dRenderer *pRenderer, const FruitType *pTypes, size_t nCount, ff::PointFloat rightPixel) override;
	virtual IMaze* GetMaze() const override;

	// IMazeListener
	virtual void OnTileChanged(ff::PointInt tile, TileContent oldContent, TileContent newContent) override;
	virtual void OnAllTilesChanged() override;

private:
	void RenderFruit(ff::I2dRenderer *pRenderer, IPlayingMaze *pPlay);
	void RenderScaredGhosts(ff::I2dRenderer *pRenderer, IPlayingMaze *pPlay);
	void RenderGhosts(ff::I2dRenderer *pRenderer, IPlayingMaze *pPlay);
	void RenderPac(ff::I2dRenderer *pRenderer, IPlayingMaze *pPlay);
	void RenderCustom(ff::I2dRenderer *pRenderer, IPlayingMaze *pPlay);
	ff::ISpriteAnimation *GetPacAnim(IPlayingMaze *play, bool allowPowerPac);
	ff::ISpriteAnimation *GetPacDyingAnim(IPlayingMaze *play);

	ff::ComPtr<IMaze> _maze;
	ff::ComPtr<IRenderText> _renderText;

	size_t _pacCounter;
	size_t _pacDyingCounter;
	size_t _ghostCounter;
	size_t _winningCounter;
	size_t _powerCounter;
	size_t _frameCounter;
	size_t _pacDyingFrame;
	float _pacFrame;

	// Maze sprites:
	ff::TypedResource<ff::ISpriteList> _wallSprites;
	ff::TypedResource<ff::ISpriteList> _outlineSprites;
	ff::TypedResource<ff::ISpriteList> _wallBgSprites;
	ff::TypedResource<ff::ISprite> _fruitSprites[13];
	ff::Vector<ff::Vector<size_t>> _mazeSprites;

	// Pac and Ghost sprites:
	ff::TypedResource<ff::ISpriteAnimation> _pacAnim[2];
	ff::TypedResource<ff::ISpriteAnimation> _pacPowerAnim[2];
	ff::TypedResource<ff::ISpriteAnimation> _pacDyingAnim[2];
	ff::TypedResource<ff::ISpriteAnimation> _ghostMoveAnim[4];
	ff::TypedResource<ff::ISpriteAnimation> _ghostScaredAnim[4];
	ff::TypedResource<ff::ISpriteAnimation> _ghostFlashAnim[4];
	ff::TypedResource<ff::ISpriteAnimation> _ghostEyesAnim[4];
	ff::TypedResource<ff::ISpriteAnimation> _ghostPupilsAnim[4];
	ff::TypedResource<ff::ISpriteAnimation> _powerAnim;
	ff::TypedResource<ff::ISpriteAnimation> _dotAnim;
	ff::TypedResource<ff::ISpriteAnimation> _powerAuraAnim;
	ff::TypedResource<ff::ISpriteAnimation> _keepAliveAnim[3];

	static const DirectX::XMFLOAT4 _colorGhostDoor;
	static const ff::PointFloat _spriteScale;
	static const size_t _pacDyingFirstFrameCount = 30;
	static const size_t _pacDyingFrameCount = 12;
	static const size_t _pacDyingFrameRepeat = 8;

	static const WallDefine _wallDefines[46];
};

const DirectX::XMFLOAT4 RenderMaze::_colorGhostDoor(1, 0.7216f, 1, 1);
const ff::PointFloat RenderMaze::_spriteScale(0.125f, 0.125f);

const WallDefine RenderMaze::_wallDefines[46] =
{
	// Key:
	// 'W' = Wall
	// 'G' = Ghost wall
	// 'A' = Any wall
	// 'X' = Any wall or out of bounds
	// '-' = Ghost door
	// 'O' = Out-of-bounds
	// '.' = Normal path

	{ 0,
	L"WWW"
	L"WWW"
	L"WWW" },

	{ 1,
	L" WW"
	L".WW"
	L" WW" },

	{ 2,
	L" . "
	L"WWW"
	L"WWW" },

	{ 3,
	L"WW "
	L"WW."
	L"WW " },

	{ 4,
	L"WWW"
	L"WWW"
	L" . " },

	{ 5,
	L"..."
	L".WW"
	L".WW" },

	{ 6,
	L"..."
	L"WW."
	L"WW." },

	{ 7,
	L"WW."
	L"WW."
	L"..." },

	{ 8,
	L".WW"
	L".WW"
	L"..." },

	{ 9,
	L".WW"
	L"WWW"
	L"WWW" },

	{ 10,
	L"WW."
	L"WWW"
	L"WWW" },

	{ 11,
	L"WWW"
	L"WWW"
	L"WW." },

	{ 12,
	L"WWW"
	L"WWW"
	L".WW" },

	{ 17,
	L".. "
	L".WX"
	L" WO" },

	{ 18,
	L" .."
	L"XW."
	L"OW " },

	{ 19,
	L"OW "
	L"XW."
	L" .." },

	{ 20,
	L" WO"
	L".WX"
	L".. " },

	{ 13,
	L"   "
	L".AO"
	L"   " },

	{ 14,
	L" . "
	L" A "
	L" O " },

	{ 15,
	L"   "
	L"OA."
	L"   " },

	{ 16,
	L" O "
	L" A "
	L" . " },

	{ 21,
	L"   "
	L" - "
	L"   " },

	{ 22,
	L".W "
	L"WWO"
	L" OO" },

	{ 23,
	L" W."
	L"OWW"
	L"OO " },

	{ 24,
	L"OO "
	L"OWW"
	L" W." },

	{ 25,
	L" OO"
	L"WWO"
	L".W " },

	{ 26,
	L".WO"
	L"WWO"
	L"WWO" },

	{ 27,
	L"OW."
	L"OWW"
	L"OWW" },

	{ 28,
	L"OWW"
	L"OWW"
	L"OW." },

	{ 29,
	L"WWO"
	L"WWO"
	L".WO" },

	{ 30,
	L".WW"
	L"WWW"
	L"OOO" },

	{ 31,
	L"WW."
	L"WWW"
	L"OOO" },

	{ 32,
	L"OOO"
	L"WWW"
	L"WW." },

	{ 33,
	L"OOO"
	L"WWW"
	L".WW" },

	{ 34,
	L"..."
	L".GG"
	L".GO" },

	{ 35,
	L"..."
	L"GG."
	L"OG." },

	{ 36,
	L"OG."
	L"GG."
	L"..." },

	{ 37,
	L".GO"
	L".GG"
	L"..." },

	{ 38,
	L"O. "
	L"OWW"
	L"OWW" },

	{ 39,
	L" .O"
	L"WWO"
	L"WWO" },

	{ 40,
	L"OWW"
	L"OWW"
	L"O. " },

	{ 41,
	L"WWO"
	L"WWO"
	L" .O" },

	{ 42,
	L"OOO"
	L".WW"
	L" WW" },

	{ 43,
	L"OOO"
	L"WW."
	L"WW " },

	{ 44,
	L" WW"
	L".WW"
	L"OOO" },

	{ 45,
	L"WW "
	L"WW."
	L"OOO" },
};

static size_t GetSpriteOffsetForDir(ff::PointInt dir)
{
	if (dir.x < 0)
	{
		// Left
		return 0;
	}
	
	if (dir.x > 0)
	{
		// Right
		return 2;
	}

	if (dir.y < 0)
	{
		// Up
		return 1;
	}

	// Down
	return 3;
}

static const float PI_DIV_4 = ff::PI_F / -4.0f;

static float GetRotationForPacDir(ff::PointInt dir)
{
	if (dir.y < 0)
	{
		// Up
		return PI_DIV_4 * 2;
	}

	if (dir.y > 0)
	{
		// Down
		return PI_DIV_4 * 6;
	}

	// Left or Right
	return 0;
}

static ff::PointFloat GetScaleForPacDir(ff::PointInt dir)
{
	if (dir.x > 0)
	{
		return ff::PointFloat(-1, 1);
	}

	return ff::PointFloat(1, 1);
}

BEGIN_INTERFACES(RenderMaze)
	HAS_INTERFACE(IRenderMaze)
END_INTERFACES()

// static
bool IRenderMaze::Create(IMaze *pMaze, IRenderMaze **ppRender)
{
	assertRetVal(ppRender && pMaze, false);
	*ppRender = nullptr;

	ff::ComPtr<RenderMaze> pRender = new ff::ComObject<RenderMaze>;
	assertRetVal(pRender->Init(pMaze), false);

	*ppRender = ff::GetAddRef<IRenderMaze>(pRender);

	return true;
}

RenderMaze::RenderMaze()
	: _pacCounter(0)
	, _pacDyingCounter(0)
	, _ghostCounter(0)
	, _winningCounter(0)
	, _frameCounter(0)
	, _powerCounter(0)
	, _pacFrame(0)
	, _pacDyingFrame(0)
{
}

RenderMaze::~RenderMaze()
{
	if (_maze)
	{
		_maze->RemoveListener(this);
	}
}

bool RenderMaze::Init(IMaze *pMaze)
{
	assertRetVal(pMaze, false);

	_maze = pMaze;
	_pacFrame = 0;

	_maze->AddListener(this);

	assertRetVal(IRenderText::Create(&_renderText), false);

	_wallSprites = GetWallSpritePage();
	_outlineSprites = GetOutlineSpritePage();
	_wallBgSprites = GetWallBackgroundSpritePage();

	_pacAnim[0].Init(L"mr-char-anim");
	_pacAnim[1].Init(L"ms-char-anim");

	_pacPowerAnim[0].Init(L"mr-power-anim");
	_pacPowerAnim[1].Init(L"ms-power-anim");

	_pacDyingAnim[0].Init(L"mr-death-anim");
	_pacDyingAnim[1].Init(L"ms-death-anim");

	_ghostMoveAnim[0].Init(L"ghost-1-move-anim");
	_ghostMoveAnim[1].Init(L"ghost-2-move-anim");
	_ghostMoveAnim[2].Init(L"ghost-3-move-anim");
	_ghostMoveAnim[3].Init(L"ghost-4-move-anim");

	_ghostScaredAnim[0].Init(L"ghost-1-scared-anim");
	_ghostScaredAnim[1].Init(L"ghost-2-scared-anim");
	_ghostScaredAnim[2].Init(L"ghost-3-scared-anim");
	_ghostScaredAnim[3].Init(L"ghost-4-scared-anim");

	_ghostFlashAnim[0].Init(L"ghost-1-flash-anim");
	_ghostFlashAnim[1].Init(L"ghost-2-flash-anim");
	_ghostFlashAnim[2].Init(L"ghost-3-flash-anim");
	_ghostFlashAnim[3].Init(L"ghost-4-flash-anim");

	_ghostEyesAnim[0].Init(L"ghost-1-eyes-anim");
	_ghostEyesAnim[1].Init(L"ghost-2-eyes-anim");
	_ghostEyesAnim[2].Init(L"ghost-3-eyes-anim");
	_ghostEyesAnim[3].Init(L"ghost-4-eyes-anim");

	_ghostPupilsAnim[0].Init(L"ghost-1-pupils-anim");
	_ghostPupilsAnim[1].Init(L"ghost-2-pupils-anim");
	_ghostPupilsAnim[2].Init(L"ghost-3-pupils-anim");
	_ghostPupilsAnim[3].Init(L"ghost-4-pupils-anim");

	_fruitSprites[0].Init(L"char-sprites.fruit 0");
	_fruitSprites[1].Init(L"char-sprites.fruit 1");
	_fruitSprites[2].Init(L"char-sprites.fruit 2");
	_fruitSprites[3].Init(L"char-sprites.fruit 3");
	_fruitSprites[4].Init(L"char-sprites.fruit 4");
	_fruitSprites[5].Init(L"char-sprites.fruit 5");
	_fruitSprites[6].Init(L"char-sprites.fruit 6");
	_fruitSprites[7].Init(L"char-sprites.fruit 7");
	_fruitSprites[8].Init(L"char-sprites.fruit 8");
	_fruitSprites[9].Init(L"char-sprites.fruit 9");
	_fruitSprites[10].Init(L"char-sprites.fruit 10");
	_fruitSprites[11].Init(L"char-sprites.fruit 11");
	_fruitSprites[12].Init(L"char-sprites.fruit 12");

	_dotAnim.Init(L"dot-anim");
	_powerAnim.Init(L"power-anim");
	_powerAuraAnim.Init(L"power-aura-anim");
	_keepAliveAnim[0].Init(L"bubble-1-anim");
	_keepAliveAnim[1].Init(L"bubble-2-anim");
	_keepAliveAnim[2].Init(L"bubble-3-anim");

	OnAllTilesChanged();

	return true;
}

void RenderMaze::Reset()
{
	_pacCounter = 0;
	_pacDyingCounter = 0;
	_ghostCounter = 0;
	_winningCounter = 0;
	_frameCounter = 0;
	_powerCounter = 0;
	_pacFrame = 0;
	_pacDyingFrame = 0;
}

void RenderMaze::OnTileChanged(ff::PointInt tile, TileContent oldContent, TileContent newContent)
{
}

void RenderMaze::OnAllTilesChanged()
{
	assertRet(_maze);

	ff::ComPtr<IMaze> pClone;
	assertRet(_maze->Clone(false, &pClone));

	// Create my array of sprite tiles

	ff::PointInt tiles = pClone->GetSizeInTiles();

	_mazeSprites.Resize(tiles.y);

	for (int y = 0; y < tiles.y; y++)
	{
		_mazeSprites[y].Resize(tiles.x);

		for (int x = 0; x < tiles.x; x++)
		{
			_mazeSprites[y][x] = ff::INVALID_SIZE;
		}
	}

	// Figure out each wall sprite

	for (int y = 0; y < tiles.y; y++)
	{
		for (int x = 0; x < tiles.x; x++)
		{
			bool bWall = false;

			switch (pClone->GetTileContent(ff::PointInt(x, y)))
			{
			case CONTENT_WALL:
			case CONTENT_GHOST_WALL:
			case CONTENT_GHOST_DOOR:
				bWall = true;
				break;
			}

			if (!bWall)
			{
				continue;
			}

			// Get everything that surrounds the wall

			TileContent content[9] =
			{
				pClone->GetTileContent(ff::PointInt(x - 1, y - 1)),
				pClone->GetTileContent(ff::PointInt(x + 0, y - 1)),
				pClone->GetTileContent(ff::PointInt(x + 1, y - 1)),
				pClone->GetTileContent(ff::PointInt(x - 1, y + 0)),
				pClone->GetTileContent(ff::PointInt(x + 0, y + 0)),
				pClone->GetTileContent(ff::PointInt(x + 1, y + 0)),
				pClone->GetTileContent(ff::PointInt(x - 1, y + 1)),
				pClone->GetTileContent(ff::PointInt(x + 0, y + 1)),
				pClone->GetTileContent(ff::PointInt(x + 1, y + 1)),
			};

			TileZone zone[9] =
			{
				pClone->GetTileZone(ff::PointInt(x - 1, y - 1)),
				pClone->GetTileZone(ff::PointInt(x + 0, y - 1)),
				pClone->GetTileZone(ff::PointInt(x + 1, y - 1)),
				pClone->GetTileZone(ff::PointInt(x - 1, y + 0)),
				pClone->GetTileZone(ff::PointInt(x + 0, y + 0)),
				pClone->GetTileZone(ff::PointInt(x + 1, y + 0)),
				pClone->GetTileZone(ff::PointInt(x - 1, y + 1)),
				pClone->GetTileZone(ff::PointInt(x + 0, y + 1)),
				pClone->GetTileZone(ff::PointInt(x + 1, y + 1)),
			};

			// Check every sprite definition until a match is found

			for (size_t i = 0; i < _countof(_wallDefines); i++)
			{
				size_t nSprite = _wallDefines[i]._index;
				wchar_t *check = _wallDefines[i]._tiles;

				assert(wcslen(check) == 9);

				bool bMatch = true;

				for (size_t i = 0; bMatch && i < 9; i++)
				{
					switch (check[i])
					{
					default:
						bMatch = false;
						assertSz(false, L"Invalid character in wall sprite check");
						break;

					case ' ': // anything
						break;

					case '.': // normal
						bMatch = (zone[i] != ZONE_OUT_OF_BOUNDS &&
							(content[i] == CONTENT_NOTHING ||
							 content[i] == CONTENT_DOT ||
							 content[i] == CONTENT_POWER ||
							 content[i] == CONTENT_PAC_START ||
							 content[i] == CONTENT_FRUIT_START ));
						break;

					case 'O':
						bMatch = (zone[i] == ZONE_OUT_OF_BOUNDS);
						break;

					case 'W':
						bMatch = (content[i] == CONTENT_WALL);
						break;

					case 'G':
						bMatch = (content[i] == CONTENT_GHOST_WALL);
						break;

					case 'A':
						bMatch = (content[i] == CONTENT_WALL || content[i] == CONTENT_GHOST_WALL);
						break;

					case 'X':
						bMatch = (content[i] == CONTENT_WALL || content[i] == CONTENT_GHOST_WALL || zone[i] == ZONE_OUT_OF_BOUNDS);
						break;

					case '-':
						bMatch = (content[i] == CONTENT_GHOST_DOOR);
						break;
					}
				}

				if (bMatch)
				{
					_mazeSprites[y][x] = nSprite;
					break;
				}
			}

			assertSz(_mazeSprites[y][x] != ff::INVALID_SIZE, L"Couldn't find a wall sprite match");
		}
	}
}

void RenderMaze::Advance(bool bPac, bool bGhosts, bool bDots, IPlayingMaze *pPlay)
{
	GameState gameState = pPlay ? pPlay->GetGameState() : GS_PLAYING;
	PacState pacState = pPlay ? pPlay->GetPacState() : PAC_INVALID;

	if (gameState == GS_WINNING)
	{
		_winningCounter++;
	}

	if (bPac)
	{
		if (pacState == PAC_NORMAL)
		{
			if (gameState == GS_WINNING || gameState == GS_WON)
			{
				_pacFrame = 0;
			}
			else
			{
				ff::ISpriteAnimation *pacAnim = GetPacAnim(pPlay, true);
				if (pacAnim)
				{
					_pacFrame = _pacCounter * pacAnim->GetFPS() / 60.0f;
				}

				if (gameState != GS_CAUGHT)
				{
					_pacCounter++;
				}
			}
		}
		else if (pacState == PAC_DYING)
		{
			_pacDyingFrame = (_pacDyingCounter >= _pacDyingFirstFrameCount)
				? std::min(
					(_pacDyingCounter - _pacDyingFirstFrameCount),
					(_pacDyingFrameCount - 1) * _pacDyingFrameRepeat)
				: 0;

			_pacDyingCounter++;
		}
	}

	if (bGhosts)
	{
		_ghostCounter++;
	}
	
	if (bDots)
	{
		_powerCounter++;
	}

	_frameCounter++;
}

void RenderMaze::RenderBackground(ff::I2dRenderer *pRenderer)
{
}

void RenderMaze::RenderTheMaze(ff::I2dRenderer *pRenderer)
{
	assertRet(pRenderer);

	ff::ISpriteList *wallSprites = _wallSprites.GetObject();
	ff::ISpriteList *outlineSprites = _outlineSprites.GetObject();
	ff::ISpriteList *bgSprites = _wallBgSprites.GetObject();
	noAssertRet(wallSprites && outlineSprites && bgSprites);

	DirectX::XMFLOAT4 fillColor = _maze->GetFillColor();
	DirectX::XMFLOAT4 borderColor = _maze->GetBorderColor();
	DirectX::XMFLOAT4 bgColor = _maze->GetBackgroundColor();

	if (_winningCounter >= 120 && _winningCounter < 225 && (_winningCounter - 120) % 30 < 15)
	{
		bgColor = DirectX::XMFLOAT4(0, 0, 0, 0);
		fillColor = DirectX::XMFLOAT4(0, 0, 0, 0);
		borderColor = DirectX::XMFLOAT4(1, 1, 1, 1);
	}

	// Start fresh

	DirectX::XMFLOAT4 colors[3] =
	{
		bgColor,
		fillColor,
		borderColor,
	};

	DirectX::XMFLOAT4 ghostDoorColors[3] =
	{
		colors[0],
		_colorGhostDoor,
		_colorGhostDoor
	};

	ff::PointInt size = _maze->GetSizeInTiles();
	ff::PointFloat tileSize = PixelsPerTileF();
	ff::PointFloat topLeft(0, 0);

	for (ff::PointInt tile(0, 0); tile.y < size.y; tile.x = 0, tile.y++, topLeft.x = 0, topLeft.y += tileSize.y)
	{
		size_t *pSprite = _mazeSprites[tile.y].Data();

		for (; tile.x < size.x; tile.x++, topLeft.x += tileSize.x, pSprite++)
		{
			if (*pSprite != ff::INVALID_SIZE)
			{
				ff::ISprite* pSprites[3] =
				{
					bgSprites->Get(*pSprite),
					wallSprites->Get(*pSprite),
					outlineSprites->Get(*pSprite)
				};

				bool bGhostDoor = (*pSprite == 21);
				const DirectX::XMFLOAT4* pColors = bGhostDoor ? ghostDoorColors : colors;

				pRenderer->DrawMultiSprite(
					pSprites, 3,
					&topLeft,
					&_spriteScale,
					0,
					pColors, 3);
			}
			else if (_maze->GetTileZone(tile) != ZONE_OUT_OF_BOUNDS)
			{
				pRenderer->DrawFilledRectangle(&ff::RectFloat(topLeft, topLeft + tileSize), &colors[0], 1);
			}
		}
	}
}

void RenderMaze::RenderDots(ff::I2dRenderer *pRenderer)
{
	assertRet(pRenderer);

	ff::ISpriteAnimation *powerAnim = _powerAnim.GetObject();
	ff::ISpriteAnimation *dotAnim = _dotAnim.GetObject();
	noAssertRet(powerAnim && dotAnim);

	ff::PointInt size = _maze->GetSizeInTiles();
	float dotFame = _powerCounter * dotAnim->GetFPS() / 60.0f;
	float powerFrame = _powerCounter * powerAnim->GetFPS() / 60.0f + powerAnim->GetLastFrame() / 2.0f;

	bool oldForceOpaque = pRenderer->ForceOpaqueUntilFlush(true);

	for (ff::PointInt tile(0, 0); tile.y < size.y; tile.x = 0, tile.y++)
	{
		for (; tile.x < size.x; tile.x++)
		{
			switch (_maze->GetTileContent(tile))
			{
				case CONTENT_DOT:
					dotAnim->Render(
						pRenderer,
						ff::POSE_TWEEN_SPLINE_LOOP,
						dotFame + tile.x / 2 + tile.y / 2,
						TileCenterToPixelF(tile),
						&_spriteScale,
						0,
						nullptr);
					break;

				case CONTENT_POWER:
					powerAnim->Render(
						pRenderer,
						ff::POSE_TWEEN_LINEAR_LOOP,
						powerFrame,
						TileCenterToPixelF(tile),
						&_spriteScale,
						0,
						nullptr);
					break;
			}
		}
	}

	pRenderer->ForceOpaqueUntilFlush(oldForceOpaque);
}

void RenderMaze::RenderActors(ff::I2dRenderer *pRenderer, bool bPac, bool bGhosts, bool bCustom, IPlayingMaze *pPlay)
{
	assertRet(pRenderer && pPlay);

	if (bGhosts)
	{
		RenderFruit(pRenderer, pPlay);
		RenderScaredGhosts(pRenderer, pPlay);
	}

	if (bPac)
	{
		RenderPac(pRenderer, pPlay);
	}

	if (bGhosts)
	{
		RenderGhosts(pRenderer, pPlay);
	}

	if (bCustom)
	{
		RenderCustom(pRenderer, pPlay);
	}
}

void RenderMaze::RenderPoints(ff::I2dRenderer *pRenderer, IPlayingMaze *pPlay)
{
	size_t nCount = 0;
	const PointActor *pPoints = pPlay ? pPlay->GetPointDisplays(nCount) : nullptr;

	for (; nCount > 0; nCount--, pPoints++)
	{
		size_t nPoints = pPoints->GetPoints();
		const DirectX::XMFLOAT4 &color = pPoints->GetColor();

		ff::PointFloat scale = pPoints->GetScale();
		ff::PointFloat pos = pPoints->GetPixel().ToFloat();
		_renderText->DrawSmallNumber(pRenderer, nPoints, pos, &color, &scale);
	}
}

void RenderMaze::RenderFruit(ff::I2dRenderer *pRenderer, IPlayingMaze *pPlay)
{
	noAssertRet(pPlay->GetFruitState() != FRUIT_INVALID);

	FruitType type = pPlay->GetFruitType();
	ff::PointInt pix = pPlay->GetFruit()->GetPixel();
	ff::PointInt center = TileCenterToPixel(pPlay->GetFruit()->GetTile());
	ff::ISprite *sprite = nullptr;

	static float s_offsets[5] = { 2, 1.4142f, 0, -1.4142f, -2 };
	float offset = 0;

	if (pPlay->GetFruit()->GetDir() != ff::PointInt(0, 0))
	{
		if (pix.x == center.x)
		{
			offset = s_offsets[abs(pix.y - center.y) % _countof(s_offsets)];
		}
		else if (pix.y == center.y)
		{
			offset = s_offsets[abs(pix.x - center.x) % _countof(s_offsets)];
		}
	}

	if (type >= 0 && type < _countof(_fruitSprites))
	{
		sprite = _fruitSprites[type].GetObject();
	}

	if (sprite)
	{
		pRenderer->DrawSprite(sprite, &ff::PointFloat((float)pix.x, (float)pix.y + offset), &_spriteScale, 0, nullptr);
	}
}

void RenderMaze::RenderScaredGhosts(ff::I2dRenderer *pRenderer, IPlayingMaze *pPlay)
{
	for (size_t i = 0; i < pPlay->GetGhostCount(); i++)
	{
		GhostState state = pPlay->GetGhostState(i);
		if (state == GHOST_SCARED || state == GHOST_SCARED_FLASH)
		{
			ff::ISpriteAnimation *anim = (state == GHOST_SCARED_FLASH)
				? _ghostFlashAnim[i].GetObject()
				: _ghostScaredAnim[i].GetObject();

			if (anim)
			{
				float frame = _ghostCounter * anim->GetFPS() / 60.0f;
				ff::PointFloat pos = pPlay->GetGhost(i)->GetPixel().ToFloat();

				anim->Render(pRenderer, ff::POSE_TWEEN_LINEAR_LOOP, frame, pos, &_spriteScale, 0, nullptr);
			}
		}
	}
}

void RenderMaze::RenderGhosts(ff::I2dRenderer *pRenderer, IPlayingMaze *pPlay)
{
	for (size_t i = 0; i < pPlay->GetGhostCount(); i++)
	{
		GhostState state = pPlay->GetGhostState(i);
		if (state == GHOST_CHASE || state == GHOST_SCATTER || state == GHOST_EYES)
		{
			ff::ISpriteAnimation *bodyAnim = (state == GHOST_EYES)
				? _ghostEyesAnim[i].GetObject()
				: _ghostMoveAnim[i].GetObject();
			ff::ISpriteAnimation *pupilsAnim = _ghostPupilsAnim[i].GetObject();

			if (bodyAnim)
			{
				float frame = _ghostCounter * bodyAnim->GetFPS() / 60.0f;
				ff::PointFloat pos = pPlay->GetGhost(i)->GetPixel().ToFloat();

				bodyAnim->Render(pRenderer, ff::POSE_TWEEN_LINEAR_LOOP, frame, pos, &_spriteScale, 0, nullptr);
			}

			if (pupilsAnim)
			{
				float frame = _ghostCounter * pupilsAnim->GetFPS() / 60.0f;
				ff::PointFloat eyeDir = pPlay->GetGhostEyeDir(i).ToFloat();
				ff::PointFloat pos = pPlay->GetGhost(i)->GetPixel().ToFloat() + eyeDir;

				pupilsAnim->Render(pRenderer, ff::POSE_TWEEN_LINEAR_LOOP, frame, pos, &_spriteScale, 0, nullptr);
			}
		}
	}
}

void RenderMaze::RenderPac(ff::I2dRenderer *pRenderer, IPlayingMaze *pPlay)
{
	ff::ISpriteAnimation *pacAnim = nullptr;
	PacState pacState = pPlay->GetPacState();
	ff::PointFloat pos = pPlay->GetPac()->GetPixel().ToFloat();
	ff::PointInt dir = pPlay->GetPac()->GetDir();
	ff::PointFloat scale = _spriteScale * GetScaleForPacDir(dir);
	ff::AnimTweenType tweenType = ff::POSE_TWEEN_LINEAR_LOOP;
	float rotate = GetRotationForPacDir(dir);
	float frame = 0;

	if (pacState == PAC_NORMAL)
	{
		pacAnim = GetPacAnim(pPlay, true);
		frame = _pacFrame;
	}
	else if (pacState == PAC_DYING || pacState == PAC_DEAD)
	{
		pacAnim = GetPacDyingAnim(pPlay);
		frame = (float)_pacDyingFrame / _pacDyingFrameRepeat;
		tweenType = ff::POSE_TWEEN_LINEAR_CLAMP;
	}

	if (pPlay->IsPowerPac())
	{
		ff::ISpriteAnimation *auraAnim = _powerAuraAnim.GetObject();
		if (auraAnim)
		{
			float auraFrame = _frameCounter * auraAnim->GetFPS() / 60.0f;
			auraAnim->Render(pRenderer, ff::POSE_TWEEN_LINEAR_LOOP, auraFrame, pos, &scale, rotate, nullptr);
		}
	}

	if (pacAnim)
	{
		pacAnim->Render(pRenderer, tweenType, frame, pos, &scale, rotate, nullptr);
	}
}

void RenderMaze::RenderCustom(ff::I2dRenderer *pRenderer, IPlayingMaze *pPlay)
{
	size_t nCount = 0;
	CustomActor * const *pCustoms = pPlay ? pPlay->GetCustomActors(nCount) : nullptr;

	for (size_t i = 0; i < nCount; i++)
	{
		CustomActor &actor = *pCustoms[i];
		actor.Render(pRenderer);
	}
}

ff::ISpriteAnimation *RenderMaze::GetPacAnim(IPlayingMaze *play, bool allowPowerPac)
{
	ff::TypedResource<ff::ISpriteAnimation> *anims = allowPowerPac && play->IsPowerPac()
		? _pacPowerAnim
		: _pacAnim;

	return anims[play->GetCharType()].GetObject();
}

ff::ISpriteAnimation *RenderMaze::GetPacDyingAnim(IPlayingMaze *play)
{
	return _pacDyingAnim[play->GetCharType()].GetObject();
}

void RenderMaze::RenderFreeLives(ff::I2dRenderer *pRenderer, IPlayingMaze *play, size_t nLives, ff::PointFloat leftPixel)
{
	assertRet(pRenderer);

	ff::ISpriteAnimation *pacAnim = GetPacAnim(play, false);
	noAssertRet(pacAnim);

	ff::PointFloat scale = _spriteScale * 0.875f;
	ff::PointFloat pixel = leftPixel;

	for (size_t i = 0; i < nLives; i++, pixel.x += PixelsPerTileF().x * 2.0f)
	{
		pacAnim->Render(
			pRenderer, ff::POSE_TWEEN_LINEAR_CLAMP,
			0, pixel, &scale, 0,
			&ff::GetColorWhite());
	}
}

void RenderMaze::RenderStatusFruits(ff::I2dRenderer *pRenderer, const FruitType *pTypes, size_t nCount, ff::PointFloat rightPixel)
{
	assertRet(pRenderer);

	if (pTypes && nCount)
	{
		ff::PointFloat pixel = rightPixel;
		ff::PointFloat scale = _spriteScale * 0.875f;

		for (size_t i = ff::PreviousSize(nCount); i != ff::INVALID_SIZE; i = ff::PreviousSize(i), pixel.x -= PixelsPerTileF().x * 2.0f)
		{
			ff::ISprite *sprite = nullptr;

			if (pTypes[i] >= 0 && pTypes[i] < _countof(_fruitSprites))
			{
				sprite = _fruitSprites[pTypes[i]].GetObject();
			}

			if (sprite)
			{
				pRenderer->DrawSprite(sprite, &pixel, &scale, 0, &ff::GetColorWhite());
			}
		}
	}
}

IMaze *RenderMaze::GetMaze() const
{
	return _maze;
}
