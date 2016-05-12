#include "pch.h"
#include "Core/Actors.h"
#include "Core/Difficulty.h"
#include "Core/GhostBrains.h"
#include "Core/Helpers.h"
#include "Graph/2D/2dRenderer.h"
#include "Graph/2D/SpriteAnimation.h"
#include "Graph/Anim/AnimKeys.h"
#include "Module/Module.h"

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

PlayingActor::PlayingActor(const PlayingActor &rhs)
{
	*this = rhs;
}

PlayingActor::~PlayingActor()
{
}

PlayingActor &PlayingActor::operator=(const PlayingActor &rhs)
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
	_pressDir = ff::PointInt(0, 0);
	_pixel = ff::PointInt(0, 0);
	_dir = ff::PointInt(0, 0);
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

ff::PointInt PlayingActor::GetTile() const
{
	return PixelAndDirToTile(_pixel, _dir);
}

ff::PointInt PlayingActor::GetPixel() const
{
	return _pixel;
}

ff::PointInt PlayingActor::GetDir() const
{
	return _dir;
}

ff::PointInt PlayingActor::GetPressDir() const
{
	return _pressDir;
}

void PlayingActor::SetPixel(ff::PointInt pixel)
{
	if (_active)
	{
		ff::PointInt oldTile = GetTile();

		_pixel = pixel;

		if (oldTile != GetTile())
		{
			OnTileChanged();
		}
	}
}

void PlayingActor::SetDir(ff::PointInt dir)
{
	if (_active)
	{
		assert(abs(dir.x) <= 1 && abs(dir.y) <= 1);

		_dir = dir;
	}
}

void PlayingActor::SetPressDir(ff::PointInt dir)
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

PacActor::PacActor(const PacActor &rhs)
{
	*this = rhs;
}

PacActor::~PacActor()
{
}

PacActor &PacActor::operator=(const PacActor &rhs)
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

GhostActor::GhostActor(const GhostActor &rhs)
{
	*this = rhs;
}

GhostActor::~GhostActor()
{
}

GhostActor &GhostActor::operator=(const GhostActor &rhs)
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

IGhostBrains *GhostActor::GetBrains()
{
	return _brains;
}

void GhostActor::SetBrains(IGhostBrains *brains)
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

FruitActor::FruitActor(const FruitActor &rhs)
{
	*this = rhs;
}

FruitActor::~FruitActor()
{
}

FruitActor &FruitActor::operator=(const FruitActor &rhs)
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
	_exitTile = ff::PointInt(0, 0);
	_brains = nullptr;
}

IGhostBrains *FruitActor::GetBrains()
{
	return _brains;
}

void FruitActor::SetBrains(IGhostBrains *brains)
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

ff::PointInt FruitActor::GetExitTile() const
{
	return _exitTile;
}

void FruitActor::SetExitTile(ff::PointInt tile)
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

PointActor::PointActor(const PointActor &rhs)
{
	*this = rhs;
}

PointActor::~PointActor()
{
}

PointActor &PointActor::operator=(const PointActor &rhs)
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
	_scale = ff::PointFloat(1, 1);
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

const DirectX::XMFLOAT4 &PointActor::GetColor() const
{
	return _color;
}

void PointActor::SetColor(const DirectX::XMFLOAT4 &color, bool bFades)
{
	_color = color;
	_fadeAlpha = (bFades && _countdown) ? (1.0f / _countdown) : 0.0f;
}

ff::PointFloat PointActor::GetScale() const
{
	return _scale;
}

void PointActor::SetScale(ff::PointFloat scale)
{
	_scale = scale;
}

void CustomActor::DeleteThis()
{
	delete this;
}

SpriteAnimActor::SpriteAnimActor(
	const wchar_t *animName,
	ff::PointFloat pos,
	ff::PointFloat scale,
	DirectX::XMFLOAT4 color,
	ff::AnimTweenType tweenType,
	ff::PointFloat velocity,
	ff::PointFloat acceleration,
	float timeScale,
	bool forceOpaque)
	: _anim(animName)
	, _pos(pos)
	, _scale(scale)
	, _color(color)
	, _timeScale(timeScale)
	, _tweenType(tweenType)
	, _velocity(velocity)
	, _accel(acceleration)
	, _frame(0)
	, _forceOpaque(forceOpaque)
{
}

typedef std::array<BYTE, sizeof(SpriteAnimActor)> SpriteAnimPoolItem;
static ff::PoolAllocator<SpriteAnimPoolItem> s_spriteAnimPool;

void *SpriteAnimActor::operator new(std::size_t size)
{
	assert(size <= sizeof(SpriteAnimPoolItem));
	return s_spriteAnimPool.New();
}

void SpriteAnimActor::operator delete(void *mem)
{
	s_spriteAnimPool.DeleteVoid(mem);
}

void SpriteAnimActor::DeleteThis()
{
	delete this;
}

bool SpriteAnimActor::Advance()
{
	ff::ISpriteAnimation *anim = _anim.GetObject();
	noAssertRetVal(anim, true);

	_frame += anim->GetFPS() * _timeScale / 60.0f;
	_pos += _velocity;
	_velocity += _accel;

	if (_frame > anim->GetLastFrame())
	{
		if (_tweenType == ff::POSE_TWEEN_LINEAR_CLAMP ||
			_tweenType == ff::POSE_TWEEN_SPLINE_CLAMP)
		{
			return false;
		}
	}

	return true;
}

void SpriteAnimActor::Render(ff::I2dRenderer *render)
{
	ff::ISpriteAnimation *anim = _anim.GetObject();
	noAssertRet(anim);

	bool oldOpaque;
	if (_forceOpaque)
	{
		oldOpaque = render->ForceOpaqueUntilFlush(true);
	}

	anim->Render(render, _tweenType, _frame, _pos, &_scale, 0, &_color);

	if (_forceOpaque)
	{
		render->ForceOpaqueUntilFlush(oldOpaque);
	}
}
