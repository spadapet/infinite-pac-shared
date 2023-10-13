#include "pch.h"
#include "Core/Actors.h"
#include "Core/Difficulty.h"
#include "Core/GhostBrains.h"
#include "Core/Helpers.h"

PlayingActor::PlayingActor()
    : _active(true)
    , _pressDir(0, 0)
    , _pixel(0, 0)
    , _dir(0, 0)
    , _speed(0)
    , _advance(0)
    , _advancePos(0)
{
}

PlayingActor::PlayingActor(const PlayingActor& rhs)
{
    *this = rhs;
}

PlayingActor::~PlayingActor()
{
}

PlayingActor& PlayingActor::operator=(const PlayingActor& rhs)
{
    if (this != &rhs)
    {
        _active = rhs._active;
        _pressDir = rhs._pressDir;
        _pixel = rhs._pixel;
        _dir = rhs._dir;
        _speed = rhs._speed;
        _advance = rhs._advance;
        _advancePos = rhs._advancePos;
    }

    return *this;
}

void PlayingActor::Reset()
{
    _active = true;
    _pressDir = ff::point_int(0, 0);
    _pixel = ff::point_int(0, 0);
    _dir = ff::point_int(0, 0);
    _speed = 0;
    _advance = 0;
    _advancePos = 0;
}

// STATIC_DATA(pod)
static const int s_nFixedPoint = 0x10000;

size_t PlayingActor::GetAdvanceCount()
{
    size_t nAdvanceCount = 0;

    if (_active)
    {
        _advancePos += _advance;

        while (_advancePos >= s_nFixedPoint)
        {
            nAdvanceCount++;
            _advancePos -= s_nFixedPoint;
        }
    }

    return nAdvanceCount;
}

void PlayingActor::SetSpeed(size_t speed)
{
    if (_speed != speed && _active)
    {
        _speed = speed;

        double renderMultiply = (_speed * PacsPerSecondF()) / (100.0 * IdealFramesPerSecondF());

        _advance = (int)(renderMultiply * s_nFixedPoint);
    }
}

void PlayingActor::AddDelay(size_t delay)
{
    if (_active)
    {
        _advancePos -= (int)delay * s_nFixedPoint;
    }
}

ff::point_int PlayingActor::GetTile() const
{
    return PixelAndDirToTile(_pixel, _dir);
}

ff::point_int PlayingActor::GetPixel() const
{
    return _pixel;
}

ff::point_int PlayingActor::GetDir() const
{
    return _dir;
}

ff::point_int PlayingActor::GetPressDir() const
{
    return _pressDir;
}

void PlayingActor::SetPixel(ff::point_int pixel)
{
    if (_active)
    {
        ff::point_int oldTile = GetTile();

        _pixel = pixel;

        if (oldTile != GetTile())
        {
            OnTileChanged();
        }
    }
}

void PlayingActor::SetDir(ff::point_int dir)
{
    if (_active)
    {
        assert(abs(dir.x) <= 1 && abs(dir.y) <= 1);

        _dir = dir;
    }
}

void PlayingActor::SetPressDir(ff::point_int dir)
{
    if (_active)
    {
        assert(abs(dir.x) <= 1 && abs(dir.y) <= 1);

        if (_pressDir != dir)
        {
            _pressDir = dir;

            OnPressDirChanged();
        }
    }
}

bool PlayingActor::IsActive() const
{
    return _active;
}

void PlayingActor::SetActive(bool bActive)
{
    _active = bActive;
}

void PlayingActor::OnTileChanged()
{
}

void PlayingActor::OnPressDirChanged()
{
}

PacActor::PacActor()
    : _canTurn(true)
    , _stuck(false)
{
}

PacActor::PacActor(const PacActor& rhs)
{
    *this = rhs;
}

PacActor::~PacActor()
{
}

PacActor& PacActor::operator=(const PacActor& rhs)
{
    if (this != &rhs)
    {
        __super::operator=(rhs);

        _canTurn = rhs._canTurn;
        _stuck = rhs._stuck;
    }

    return *this;
}

void PacActor::Reset()
{
    __super::Reset();

    _canTurn = true;
    _stuck = false;
}

bool PacActor::CanTurn() const
{
    return _canTurn;
}

void PacActor::SetCanTurn(bool canTurn)
{
    _canTurn = canTurn;
}

bool PacActor::IsStuck() const
{
    return _stuck;
}

void PacActor::SetStuck(bool stuck)
{
    _stuck = stuck;

    if (_stuck)
    {
        _canTurn = true;
    }
}

void PacActor::OnTileChanged()
{
    _canTurn = true;
}

void PacActor::OnPressDirChanged()
{
    _canTurn = true;
}

GhostActor::GhostActor()
    : _move(MOVE_NORMAL)
    , _house(HOUSE_OUTSIDE)
    , _dotCount(0)
{
}

GhostActor::GhostActor(const GhostActor& rhs)
{
    *this = rhs;
}

GhostActor::~GhostActor()
{
}

GhostActor& GhostActor::operator=(const GhostActor& rhs)
{
    if (this != &rhs)
    {
        __super::operator=(rhs);

        _move = rhs._move;
        _house = rhs._house;
        _dotCount = rhs._dotCount;
        _brains = rhs._brains;
    }

    return *this;
}

void GhostActor::Reset()
{
    __super::Reset();

    _move = MOVE_NORMAL;
    _house = HOUSE_OUTSIDE;

    // Leave _pBrains the _nDots alone
}

IGhostBrains* GhostActor::GetBrains()
{
    return _brains.get();
}

void GhostActor::SetBrains(std::shared_ptr<IGhostBrains> brains)
{
    _brains = brains;
}

MoveState GhostActor::GetMoveState() const
{
    return _move;
}

void GhostActor::SetMoveState(MoveState state)
{
    if (_active)
    {
        _move = state;
    }
}

HouseState GhostActor::GetHouseState() const
{
    return _house;
}

void GhostActor::SetHouseState(HouseState state)
{
    if (_active)
    {
        _house = state;
    }
}

size_t GhostActor::GetDotCounter() const
{
    return _dotCount;
}

void GhostActor::SetDotCounter(size_t dotCount)
{
    if (_active)
    {
        _dotCount = dotCount;
    }
}

FruitActor::FruitActor()
    : _type(FRUIT_0)
    , _exitTile(0, 0)
{
    _active = false;
}

FruitActor::FruitActor(const FruitActor& rhs)
{
    *this = rhs;
}

FruitActor::~FruitActor()
{
}

FruitActor& FruitActor::operator=(const FruitActor& rhs)
{
    if (this != &rhs)
    {
        _type = rhs._type;
        _exitTile = rhs._exitTile;
        _brains = rhs._brains;

        __super::operator=(rhs);
    }

    return *this;
}

void FruitActor::Reset()
{
    __super::Reset();

    _active = false;
    _type = FRUIT_0;
    _exitTile = ff::point_int(0, 0);
    _brains = nullptr;
}

IGhostBrains* FruitActor::GetBrains()
{
    return _brains.get();
}

void FruitActor::SetBrains(std::shared_ptr<IGhostBrains> brains)
{
    _brains = brains;
}

FruitType FruitActor::GetType() const
{
    return _type;
}

void FruitActor::SetType(FruitType type)
{
    _type = type;
}

ff::point_int FruitActor::GetExitTile() const
{
    return _exitTile;
}

void FruitActor::SetExitTile(ff::point_int tile)
{
    _exitTile = tile;
}

PointActor::PointActor()
    : _countdown(0)
    , _points(0)
    , _color(0, 0, 0, 0)
    , _fadeAlpha(0)
    , _scale(1, 1)
{
    _active = false;
}

PointActor::PointActor(const PointActor& rhs)
{
    *this = rhs;
}

PointActor::~PointActor()
{
}

PointActor& PointActor::operator=(const PointActor& rhs)
{
    if (this != &rhs)
    {
        _countdown = rhs._countdown;
        _points = rhs._points;
        _color = rhs._color;
        _fadeAlpha = rhs._fadeAlpha;
        _scale = rhs._scale;

        __super::operator=(rhs);
    }

    return *this;
}

void PointActor::Reset()
{
    __super::Reset();

    _active = false;
    _countdown = 0;
    _points = 0;
    _color = DirectX::XMFLOAT4(0, 0, 0, 0);
    _fadeAlpha = 0;
    _scale = ff::point_float(1, 1);
}

void PointActor::SetCountdown(size_t nFrames)
{
    _countdown = nFrames;
}

bool PointActor::AdvanceCountdown()
{
    if (_active && _countdown)
    {
        _countdown--;
        _color.w = std::max(0.0f, _color.w - _fadeAlpha);

        return true;
    }
    else
    {
        return false;
    }
}

size_t PointActor::GetPoints() const
{
    return _points;
}

void PointActor::SetPoints(size_t nPoints)
{
    _points = nPoints;
}

const DirectX::XMFLOAT4& PointActor::GetColor() const
{
    return _color;
}

void PointActor::SetColor(const DirectX::XMFLOAT4& color, bool bFades)
{
    _color = color;
    _fadeAlpha = (bFades && _countdown) ? (1.0f / _countdown) : 0.0f;
}

ff::point_float PointActor::GetScale() const
{
    return _scale;
}

void PointActor::SetScale(ff::point_float scale)
{
    _scale = scale;
}

SpriteAnimActor::SpriteAnimActor(
    std::string_view animName,
    ff::point_float pos,
    ff::point_float scale,
    DirectX::XMFLOAT4 color,
    bool looping,
    ff::point_float velocity,
    ff::point_float acceleration,
    float timeScale,
    bool forceOpaque)
    : _anim(animName)
    , _pos(pos)
    , _scale(scale)
    , _color(color)
    , _timeScale(timeScale)
    , _looping(looping)
    , _velocity(velocity)
    , _accel(acceleration)
    , _frame(0)
    , _forceOpaque(forceOpaque)
{
}

bool SpriteAnimActor::Advance()
{
    ff::animation_base* anim = _anim.object().get();
    check_ret_val(anim, true);

    _frame += anim->frames_per_second() * _timeScale / 60.0f;
    _pos += _velocity;
    _velocity += _accel;

    if (_frame > anim->frame_length() && !_looping)
    {
        return false;
    }

    return true;
}

void SpriteAnimActor::Render(ff::dxgi::draw_base& draw)
{
    ff::animation_base* anim = _anim.object().get();
    check_ret(anim);

    if (_forceOpaque)
    {
        draw.push_opaque();
    }

    anim->draw_frame(draw, ff::transform(_pos, _scale, 0, _color), _frame);

    if (_forceOpaque)
    {
        draw.pop_opaque();
    }
}
