#include "pch.h"
#include "Core/Actors.h"
#include "Core/GlobalResources.h"
#include "Core/Helpers.h"
#include "Core/Maze.h"
#include "Core/PlayingMaze.h"
#include "Core/RenderMaze.h"
#include "Core/RenderText.h"
#include "Core/Tiles.h"

struct WallDefine
{
    size_t _index;
    const char* _tiles;
};

class RenderMaze : public IRenderMaze, public IMazeListener
{
public:
    RenderMaze(std::shared_ptr<IMaze> pMaze);
    virtual ~RenderMaze() override;

    // IRenderMaze
    virtual void Reset() override;
    virtual void Advance(bool bPac, bool bGhosts, bool bDots, IPlayingMaze* pPlay) override;
    virtual void RenderBackground(ff::dxgi::draw_base& draw) override;
    virtual void RenderTheMaze(ff::dxgi::draw_base& draw) override;
    virtual void RenderDots(ff::dxgi::draw_base& draw) override;
    virtual void RenderActors(ff::dxgi::draw_base& draw, bool bPac, bool bGhosts, bool bCustom, IPlayingMaze* pPlay) override;
    virtual void RenderPoints(ff::dxgi::draw_base& draw, IPlayingMaze* pPlay) override;
    virtual void RenderFreeLives(ff::dxgi::draw_base& draw, IPlayingMaze* play, size_t nLives, ff::point_float leftPixel) override;
    virtual void RenderStatusFruits(ff::dxgi::draw_base& draw, const FruitType* pTypes, size_t nCount, ff::point_float rightPixel) override;
    virtual std::shared_ptr<IMaze> GetMaze() const override;

    // IMazeListener
    virtual void OnTileChanged(ff::point_int tile, TileContent oldContent, TileContent newContent) override;
    virtual void OnAllTilesChanged() override;

private:
    void RenderFruit(ff::dxgi::draw_base& draw, IPlayingMaze* pPlay);
    void RenderScaredGhosts(ff::dxgi::draw_base& draw, IPlayingMaze* pPlay);
    void RenderGhosts(ff::dxgi::draw_base& draw, IPlayingMaze* pPlay);
    void RenderPac(ff::dxgi::draw_base& draw, IPlayingMaze* pPlay);
    void RenderCustom(ff::dxgi::draw_base& draw, IPlayingMaze* pPlay);
    ff::animation_base* GetPacAnim(IPlayingMaze* play, bool allowPowerPac);
    ff::animation_base* GetPacDyingAnim(IPlayingMaze* play);

    std::shared_ptr<IMaze> _maze;
    std::shared_ptr<IRenderText> _renderText;

    size_t _pacCounter{};
    size_t _pacDyingCounter{};
    size_t _ghostCounter{};
    size_t _winningCounter{};
    size_t _powerCounter{};
    size_t _frameCounter{};
    size_t _pacDyingFrame{};
    float _pacFrame{};

    // Maze sprites:
    ff::auto_resource<ff::sprite_list> _wallSprites;
    ff::auto_resource<ff::sprite_list> _outlineSprites;
    ff::auto_resource<ff::sprite_list> _wallBgSprites;
    ff::auto_resource<ff::sprite_base> _fruitSprites[13];
    std::vector<std::vector<size_t>> _mazeSprites;

    // Pac and Ghost sprites:
    ff::auto_resource<ff::animation_base> _pacAnim[2];
    ff::auto_resource<ff::animation_base> _pacPowerAnim[2];
    ff::auto_resource<ff::animation_base> _pacDyingAnim[2];
    ff::auto_resource<ff::animation_base> _ghostMoveAnim[4];
    ff::auto_resource<ff::animation_base> _ghostScaredAnim[4];
    ff::auto_resource<ff::animation_base> _ghostFlashAnim[4];
    ff::auto_resource<ff::animation_base> _ghostEyesAnim[4];
    ff::auto_resource<ff::animation_base> _ghostPupilsAnim[4];
    ff::auto_resource<ff::animation_base> _powerAnim;
    ff::auto_resource<ff::animation_base> _dotAnim;
    ff::auto_resource<ff::animation_base> _powerAuraAnim;
    ff::auto_resource<ff::animation_base> _keepAliveAnim[3];

    static const DirectX::XMFLOAT4 _colorGhostDoor;
    static const ff::point_float _spriteScale;
    static const size_t _pacDyingFirstFrameCount = 30;
    static const size_t _pacDyingFrameCount = 12;
    static const size_t _pacDyingFrameRepeat = 8;

    static const WallDefine _wallDefines[46];
};

const DirectX::XMFLOAT4 RenderMaze::_colorGhostDoor(1, 0.7216f, 1, 1);
const ff::point_float RenderMaze::_spriteScale(0.125f, 0.125f);

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
    "WWW"
    "WWW"
    "WWW" },

    { 1,
    " WW"
    ".WW"
    " WW" },

    { 2,
    " . "
    "WWW"
    "WWW" },

    { 3,
    "WW "
    "WW."
    "WW " },

    { 4,
    "WWW"
    "WWW"
    " . " },

    { 5,
    "..."
    ".WW"
    ".WW" },

    { 6,
    "..."
    "WW."
    "WW." },

    { 7,
    "WW."
    "WW."
    "..." },

    { 8,
    ".WW"
    ".WW"
    "..." },

    { 9,
    ".WW"
    "WWW"
    "WWW" },

    { 10,
    "WW."
    "WWW"
    "WWW" },

    { 11,
    "WWW"
    "WWW"
    "WW." },

    { 12,
    "WWW"
    "WWW"
    ".WW" },

    { 17,
    ".. "
    ".WX"
    " WO" },

    { 18,
    " .."
    "XW."
    "OW " },

    { 19,
    "OW "
    "XW."
    " .." },

    { 20,
    " WO"
    ".WX"
    ".. " },

    { 13,
    "   "
    ".AO"
    "   " },

    { 14,
    " . "
    " A "
    " O " },

    { 15,
    "   "
    "OA."
    "   " },

    { 16,
    " O "
    " A "
    " . " },

    { 21,
    "   "
    " - "
    "   " },

    { 22,
    ".W "
    "WWO"
    " OO" },

    { 23,
    " W."
    "OWW"
    "OO " },

    { 24,
    "OO "
    "OWW"
    " W." },

    { 25,
    " OO"
    "WWO"
    ".W " },

    { 26,
    ".WO"
    "WWO"
    "WWO" },

    { 27,
    "OW."
    "OWW"
    "OWW" },

    { 28,
    "OWW"
    "OWW"
    "OW." },

    { 29,
    "WWO"
    "WWO"
    ".WO" },

    { 30,
    ".WW"
    "WWW"
    "OOO" },

    { 31,
    "WW."
    "WWW"
    "OOO" },

    { 32,
    "OOO"
    "WWW"
    "WW." },

    { 33,
    "OOO"
    "WWW"
    ".WW" },

    { 34,
    "..."
    ".GG"
    ".GO" },

    { 35,
    "..."
    "GG."
    "OG." },

    { 36,
    "OG."
    "GG."
    "..." },

    { 37,
    ".GO"
    ".GG"
    "..." },

    { 38,
    "O. "
    "OWW"
    "OWW" },

    { 39,
    " .O"
    "WWO"
    "WWO" },

    { 40,
    "OWW"
    "OWW"
    "O. " },

    { 41,
    "WWO"
    "WWO"
    " .O" },

    { 42,
    "OOO"
    ".WW"
    " WW" },

    { 43,
    "OOO"
    "WW."
    "WW " },

    { 44,
    " WW"
    ".WW"
    "OOO" },

    { 45,
    "WW "
    "WW."
    "OOO" },
};

static size_t GetSpriteOffsetForDir(ff::point_int dir)
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

// static const float PI_DIV_4 = std::numbers::pi_v<float> / -4.0f;

static float GetRotationForPacDir(ff::point_int dir)
{
    if (dir.y < 0)
    {
        // Up
        return 90; // PI_DIV_4 * 2;
    }

    if (dir.y > 0)
    {
        // Down
        return 270; //  PI_DIV_4 * 6;
    }

    // Left or Right
    return 0;
}

static ff::point_float GetScaleForPacDir(ff::point_int dir)
{
    if (dir.x > 0)
    {
        return ff::point_float(-1, 1);
    }

    return ff::point_float(1, 1);
}

std::shared_ptr<IRenderMaze> IRenderMaze::Create(std::shared_ptr<IMaze> pMaze)
{
    return std::make_shared<RenderMaze>(pMaze);
}

RenderMaze::RenderMaze(std::shared_ptr<IMaze> maze)
    : _maze(maze)
    , _renderText(IRenderText::Create())
{
    _maze->AddListener(this);

    _wallSprites = GetWallSpritePage();
    _outlineSprites = GetOutlineSpritePage();
    _wallBgSprites = GetWallBackgroundSpritePage();

    _pacAnim[0] = "mr-char-anim";
    _pacAnim[1] = "ms-char-anim";

    _pacPowerAnim[0] = "mr-power-anim";
    _pacPowerAnim[1] = "ms-power-anim";

    _pacDyingAnim[0] = "mr-death-anim";
    _pacDyingAnim[1] = "ms-death-anim";

    _ghostMoveAnim[0] = "ghost-1-move-anim";
    _ghostMoveAnim[1] = "ghost-2-move-anim";
    _ghostMoveAnim[2] = "ghost-3-move-anim";
    _ghostMoveAnim[3] = "ghost-4-move-anim";

    _ghostScaredAnim[0] = "ghost-1-scared-anim";
    _ghostScaredAnim[1] = "ghost-2-scared-anim";
    _ghostScaredAnim[2] = "ghost-3-scared-anim";
    _ghostScaredAnim[3] = "ghost-4-scared-anim";

    _ghostFlashAnim[0] = "ghost-1-flash-anim";
    _ghostFlashAnim[1] = "ghost-2-flash-anim";
    _ghostFlashAnim[2] = "ghost-3-flash-anim";
    _ghostFlashAnim[3] = "ghost-4-flash-anim";

    _ghostEyesAnim[0] = "ghost-1-eyes-anim";
    _ghostEyesAnim[1] = "ghost-2-eyes-anim";
    _ghostEyesAnim[2] = "ghost-3-eyes-anim";
    _ghostEyesAnim[3] = "ghost-4-eyes-anim";

    _ghostPupilsAnim[0] = "ghost-1-pupils-anim";
    _ghostPupilsAnim[1] = "ghost-2-pupils-anim";
    _ghostPupilsAnim[2] = "ghost-3-pupils-anim";
    _ghostPupilsAnim[3] = "ghost-4-pupils-anim";

    _fruitSprites[0] = "char-sprites.fruit[0]";
    _fruitSprites[1] = "char-sprites.fruit[1]";
    _fruitSprites[2] = "char-sprites.fruit[2]";
    _fruitSprites[3] = "char-sprites.fruit[3]";
    _fruitSprites[4] = "char-sprites.fruit[4]";
    _fruitSprites[5] = "char-sprites.fruit[5]";
    _fruitSprites[6] = "char-sprites.fruit[6]";
    _fruitSprites[7] = "char-sprites.fruit[7]";
    _fruitSprites[8] = "char-sprites.fruit[8]";
    _fruitSprites[9] = "char-sprites.fruit[9]";
    _fruitSprites[10] = "char-sprites.fruit[10]";
    _fruitSprites[11] = "char-sprites.fruit[11]";
    _fruitSprites[12] = "char-sprites.fruit[12]";

    _dotAnim = "dot-anim";
    _powerAnim = "power-anim";
    _powerAuraAnim = "power-aura-anim";
    _keepAliveAnim[0] = "bubble-1-anim";
    _keepAliveAnim[1] = "bubble-2-anim";
    _keepAliveAnim[2] = "bubble-3-anim";

    OnAllTilesChanged();
}

RenderMaze::~RenderMaze()
{
    if (_maze)
    {
        _maze->RemoveListener(this);
    }
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

void RenderMaze::OnTileChanged(ff::point_int tile, TileContent oldContent, TileContent newContent)
{
}

void RenderMaze::OnAllTilesChanged()
{
    assert_ret(_maze);

    std::shared_ptr<IMaze> pClone = _maze->Clone(false);

    // Create my array of sprite tiles

    ff::point_int tiles = pClone->GetSizeInTiles();

    _mazeSprites.resize(tiles.y);

    for (int y = 0; y < tiles.y; y++)
    {
        _mazeSprites[y].resize(tiles.x);

        for (int x = 0; x < tiles.x; x++)
        {
            _mazeSprites[y][x] = ff::constants::invalid_unsigned<size_t>();
        }
    }

    // Figure out each wall sprite

    for (int y = 0; y < tiles.y; y++)
    {
        for (int x = 0; x < tiles.x; x++)
        {
            bool bWall = false;

            switch (pClone->GetTileContent(ff::point_int(x, y)))
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
                pClone->GetTileContent(ff::point_int(x - 1, y - 1)),
                pClone->GetTileContent(ff::point_int(x + 0, y - 1)),
                pClone->GetTileContent(ff::point_int(x + 1, y - 1)),
                pClone->GetTileContent(ff::point_int(x - 1, y + 0)),
                pClone->GetTileContent(ff::point_int(x + 0, y + 0)),
                pClone->GetTileContent(ff::point_int(x + 1, y + 0)),
                pClone->GetTileContent(ff::point_int(x - 1, y + 1)),
                pClone->GetTileContent(ff::point_int(x + 0, y + 1)),
                pClone->GetTileContent(ff::point_int(x + 1, y + 1)),
            };

            TileZone zone[9] =
            {
                pClone->GetTileZone(ff::point_int(x - 1, y - 1)),
                pClone->GetTileZone(ff::point_int(x + 0, y - 1)),
                pClone->GetTileZone(ff::point_int(x + 1, y - 1)),
                pClone->GetTileZone(ff::point_int(x - 1, y + 0)),
                pClone->GetTileZone(ff::point_int(x + 0, y + 0)),
                pClone->GetTileZone(ff::point_int(x + 1, y + 0)),
                pClone->GetTileZone(ff::point_int(x - 1, y + 1)),
                pClone->GetTileZone(ff::point_int(x + 0, y + 1)),
                pClone->GetTileZone(ff::point_int(x + 1, y + 1)),
            };

            // Check every sprite definition until a match is found

            for (size_t i = 0; i < _countof(_wallDefines); i++)
            {
                size_t nSprite = _wallDefines[i]._index;
                const char* check = _wallDefines[i]._tiles;

                assert(strlen(check) == 9);

                bool bMatch = true;

                for (size_t i = 0; bMatch && i < 9; i++)
                {
                    switch (check[i])
                    {
                        default:
                            bMatch = false;
                            assert_msg(false, "Invalid character in wall sprite check");
                            break;

                        case ' ': // anything
                            break;

                        case '.': // normal
                            bMatch = (zone[i] != ZONE_OUT_OF_BOUNDS &&
                                (content[i] == CONTENT_NOTHING ||
                                    content[i] == CONTENT_DOT ||
                                    content[i] == CONTENT_POWER ||
                                    content[i] == CONTENT_PAC_START ||
                                    content[i] == CONTENT_FRUIT_START));
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

            assert_msg(_mazeSprites[y][x] != ff::constants::invalid_unsigned<size_t>(), "Couldn't find a wall sprite match");
        }
    }
}

void RenderMaze::Advance(bool bPac, bool bGhosts, bool bDots, IPlayingMaze* pPlay)
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
                ff::animation_base* pacAnim = GetPacAnim(pPlay, true);
                if (pacAnim)
                {
                    _pacFrame = _pacCounter * pacAnim->frames_per_second() / 60.0f;
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

void RenderMaze::RenderBackground(ff::dxgi::draw_base& draw)
{
}

void RenderMaze::RenderTheMaze(ff::dxgi::draw_base& draw)
{
    ff::sprite_list* wallSprites = _wallSprites.object().get();
    ff::sprite_list* outlineSprites = _outlineSprites.object().get();
    ff::sprite_list* bgSprites = _wallBgSprites.object().get();
    check_ret(wallSprites && outlineSprites && bgSprites);

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

    ff::point_int size = _maze->GetSizeInTiles();
    ff::point_float tileSize = PixelsPerTileF();
    ff::point_float topLeft(0, 0);

    for (ff::point_int tile(0, 0); tile.y < size.y; tile.x = 0, tile.y++, topLeft.x = 0, topLeft.y += tileSize.y)
    {
        size_t* pSprite = _mazeSprites[tile.y].data();

        for (; tile.x < size.x; tile.x++, topLeft.x += tileSize.x, pSprite++)
        {
            if (*pSprite != ff::constants::invalid_unsigned<size_t>())
            {
                const ff::sprite_base* pSprites[3] =
                {
                    bgSprites->get(*pSprite),
                    wallSprites->get(*pSprite),
                    outlineSprites->get(*pSprite)
                };

                bool bGhostDoor = (*pSprite == 21);
                const DirectX::XMFLOAT4* pColors = bGhostDoor ? ghostDoorColors : colors;

                for (size_t i = 0; i < 3; i++)
                {
                    draw.draw_sprite(pSprites[i]->sprite_data(), ff::transform(topLeft, _spriteScale, 0, pColors[i]));
                }
            }
            else if (_maze->GetTileZone(tile) != ZONE_OUT_OF_BOUNDS)
            {
                draw.draw_rectangle(ff::rect_float(topLeft, topLeft + tileSize), colors[0]);
            }
        }
    }
}

void RenderMaze::RenderDots(ff::dxgi::draw_base& draw)
{
    ff::animation_base* powerAnim = _powerAnim.object().get();
    ff::animation_base* dotAnim = _dotAnim.object().get();
    check_ret(powerAnim && dotAnim);

    ff::point_int size = _maze->GetSizeInTiles();
    float dotFrame = _powerCounter * dotAnim->frames_per_second() / 60.0f;
    float powerFrame = _powerCounter * powerAnim->frames_per_second() / 60.0f + powerAnim->frame_length() / 2.0f;

    draw.push_opaque();

    for (ff::point_int tile(0, 0); tile.y < size.y; tile.x = 0, tile.y++)
    {
        for (; tile.x < size.x; tile.x++)
        {
            switch (_maze->GetTileContent(tile))
            {
                case CONTENT_DOT:
                    dotAnim->draw_frame(draw, ff::transform(TileCenterToPixelF(tile), _spriteScale), dotFrame + tile.x / 2 + tile.y / 2);
                    break;

                case CONTENT_POWER:
                    powerAnim->draw_frame(draw, ff::transform(TileCenterToPixelF(tile), _spriteScale), powerFrame);
                    break;
            }
        }
    }

    draw.pop_opaque();
}

void RenderMaze::RenderActors(ff::dxgi::draw_base& draw, bool bPac, bool bGhosts, bool bCustom, IPlayingMaze* pPlay)
{
    assert_ret(pPlay);

    if (bGhosts)
    {
        RenderFruit(draw, pPlay);
        RenderScaredGhosts(draw, pPlay);
    }

    if (bPac)
    {
        RenderPac(draw, pPlay);
    }

    if (bGhosts)
    {
        RenderGhosts(draw, pPlay);
    }

    if (bCustom)
    {
        RenderCustom(draw, pPlay);
    }
}

void RenderMaze::RenderPoints(ff::dxgi::draw_base& draw, IPlayingMaze* pPlay)
{
    size_t nCount = 0;
    const std::shared_ptr<PointActor>* pPoints = pPlay ? pPlay->GetPointDisplays(nCount) : nullptr;

    for (; nCount > 0; nCount--, pPoints++)
    {
        size_t nPoints = (*pPoints)->GetPoints();
        const DirectX::XMFLOAT4& color = (*pPoints)->GetColor();

        ff::point_float scale = (*pPoints)->GetScale();
        ff::point_float pos = (*pPoints)->GetPixel().cast<float>();
        _renderText->DrawSmallNumber(draw, nPoints, pos, &color, &scale);
    }
}

void RenderMaze::RenderFruit(ff::dxgi::draw_base& draw, IPlayingMaze* pPlay)
{
    check_ret(pPlay->GetFruitState() != FRUIT_INVALID);

    FruitType type = pPlay->GetFruitType();
    ff::point_int pix = pPlay->GetFruit()->GetPixel();
    ff::point_int center = TileCenterToPixel(pPlay->GetFruit()->GetTile());
    ff::sprite_base* sprite = nullptr;

    static float s_offsets[5] = { 2, 1.4142f, 0, -1.4142f, -2 };
    float offset = 0;

    if (pPlay->GetFruit()->GetDir() != ff::point_int(0, 0))
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
        sprite = _fruitSprites[type].object().get();
    }

    if (sprite)
    {
        draw.draw_sprite(sprite->sprite_data(), ff::transform(ff::point_float((float)pix.x, (float)pix.y + offset), _spriteScale));
    }
}

void RenderMaze::RenderScaredGhosts(ff::dxgi::draw_base& draw, IPlayingMaze* pPlay)
{
    for (size_t i = 0; i < pPlay->GetGhostCount(); i++)
    {
        GhostState state = pPlay->GetGhostState(i);
        if (state == GHOST_SCARED || state == GHOST_SCARED_FLASH)
        {
            ff::animation_base* anim = (state == GHOST_SCARED_FLASH)
                ? _ghostFlashAnim[i].object().get()
                : _ghostScaredAnim[i].object().get();

            if (anim)
            {
                float frame = _ghostCounter * anim->frames_per_second() / 60.0f;
                ff::point_float pos = pPlay->GetGhost(i)->GetPixel().cast<float>();

                anim->draw_frame(draw, ff::transform(pos, _spriteScale), frame);
            }
        }
    }
}

void RenderMaze::RenderGhosts(ff::dxgi::draw_base& draw, IPlayingMaze* pPlay)
{
    for (size_t i = 0; i < pPlay->GetGhostCount(); i++)
    {
        GhostState state = pPlay->GetGhostState(i);
        if (state == GHOST_CHASE || state == GHOST_SCATTER || state == GHOST_EYES)
        {
            ff::animation_base* bodyAnim = (state == GHOST_EYES)
                ? _ghostEyesAnim[i].object().get()
                : _ghostMoveAnim[i].object().get();
            ff::animation_base* pupilsAnim = _ghostPupilsAnim[i].object().get();

            if (bodyAnim)
            {
                float frame = _ghostCounter * bodyAnim->frames_per_second() / 60.0f;
                ff::point_float pos = pPlay->GetGhost(i)->GetPixel().cast<float>();

                bodyAnim->draw_frame(draw, ff::transform(pos, _spriteScale), frame);
            }

            if (pupilsAnim)
            {
                float frame = _ghostCounter * pupilsAnim->frames_per_second() / 60.0f;
                ff::point_float eyeDir = pPlay->GetGhostEyeDir(i).cast<float>();
                ff::point_float pos = pPlay->GetGhost(i)->GetPixel().cast<float>() + eyeDir;

                pupilsAnim->draw_frame(draw, ff::transform(pos, _spriteScale), frame);
            }
        }
    }
}

void RenderMaze::RenderPac(ff::dxgi::draw_base& draw, IPlayingMaze* pPlay)
{
    ff::animation_base* pacAnim = nullptr;
    PacState pacState = pPlay->GetPacState();
    ff::point_float pos = pPlay->GetPac()->GetPixel().cast<float>();
    ff::point_int dir = pPlay->GetPac()->GetDir();
    ff::point_float scale = _spriteScale * GetScaleForPacDir(dir);
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
    }

    if (pPlay->IsPowerPac())
    {
        ff::animation_base* auraAnim = _powerAuraAnim.object().get();
        if (auraAnim)
        {
            float auraFrame = _frameCounter * auraAnim->frames_per_second() / 60.0f;
            auraAnim->draw_frame(draw, ff::transform(pos, scale, rotate), auraFrame);
        }
    }

    if (pacAnim)
    {
        pacAnim->draw_frame(draw, ff::transform(pos, scale, rotate), frame);
    }
}

void RenderMaze::RenderCustom(ff::dxgi::draw_base& draw, IPlayingMaze* pPlay)
{
    size_t nCount = 0;
    const std::shared_ptr<CustomActor>* pCustoms = pPlay ? pPlay->GetCustomActors(nCount) : nullptr;

    for (size_t i = 0; i < nCount; i++)
    {
        CustomActor& actor = *pCustoms[i];
        actor.Render(draw);
    }
}

ff::animation_base* RenderMaze::GetPacAnim(IPlayingMaze* play, bool allowPowerPac)
{
    ff::auto_resource<ff::animation_base>* anims = allowPowerPac && play->IsPowerPac()
        ? _pacPowerAnim
        : _pacAnim;

    return anims[play->GetCharType()].object().get();
}

ff::animation_base* RenderMaze::GetPacDyingAnim(IPlayingMaze* play)
{
    return _pacDyingAnim[play->GetCharType()].object().get();
}

void RenderMaze::RenderFreeLives(ff::dxgi::draw_base& draw, IPlayingMaze* play, size_t nLives, ff::point_float leftPixel)
{
    ff::animation_base* pacAnim = GetPacAnim(play, false);
    check_ret(pacAnim);

    ff::point_float scale = _spriteScale * 0.875f;
    ff::point_float pixel = leftPixel;

    for (size_t i = 0; i < nLives; i++, pixel.x += PixelsPerTileF().x * 2.0f)
    {
        pacAnim->draw_frame(draw, ff::transform(pixel, scale), 0);
    }
}

void RenderMaze::RenderStatusFruits(ff::dxgi::draw_base& draw, const FruitType* pTypes, size_t nCount, ff::point_float rightPixel)
{
    if (pTypes && nCount)
    {
        ff::point_float pixel = rightPixel;
        ff::point_float scale = _spriteScale * 0.875f;

        for (size_t i = ff::constants::previous_unsigned<size_t>(nCount); i != ff::constants::invalid_unsigned<size_t>(); i = ff::constants::previous_unsigned<size_t>(i), pixel.x -= PixelsPerTileF().x * 2.0f)
        {
            ff::sprite_base* sprite = nullptr;

            if (pTypes[i] >= 0 && pTypes[i] < _countof(_fruitSprites))
            {
                sprite = _fruitSprites[pTypes[i]].object().get();
            }

            if (sprite)
            {
                draw.draw_sprite(sprite->sprite_data(), ff::transform(pixel, scale));
            }
        }
    }
}

std::shared_ptr<IMaze> RenderMaze::GetMaze() const
{
    return _maze;
}
