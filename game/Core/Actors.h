#pragma once

#include "Resource/ResourceValue.h"

namespace ff
{
	class I2dRenderer;
	class ISpriteAnimation;
	enum AnimTweenType;
}

class IGhostBrains;
enum FruitType;

enum MoveState
{
	MOVE_NORMAL,
	MOVE_SCARED,
	MOVE_EYES,
	MOVE_EATEN,
	MOVE_WAITING_TO_BE_EATEN,
};

enum HouseState
{
	HOUSE_OUTSIDE,
	HOUSE_INSIDE,
	HOUSE_LEAVING,
};

class IPlayingActor
{
public:
	virtual ~IPlayingActor() { }

	virtual ff::PointInt GetTile() const = 0;
	virtual ff::PointInt GetPixel() const = 0;
	virtual void SetPixel(ff::PointInt pixel) = 0;

	virtual ff::PointInt GetDir() const = 0;
	virtual void SetDir(ff::PointInt dir) = 0;

	virtual ff::PointInt GetPressDir() const = 0;
	virtual void SetPressDir(ff::PointInt dir) = 0;

	virtual bool IsActive() const = 0;
	virtual void SetActive(bool bActive) = 0;
};

// Represents something that moves in the maze
class PlayingActor : public IPlayingActor
{
public:
	PlayingActor();
	PlayingActor(const PlayingActor &rhs);
	~PlayingActor();

	PlayingActor &operator=(const PlayingActor &rhs);

	virtual void Reset(); // between lives

	size_t GetAdvanceCount();
	void SetSpeed(size_t speed); // 0 - 100
	void AddDelay(size_t delay);

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

protected:
	virtual void OnTileChanged();
	virtual void OnPressDirChanged();

	bool _active;
	ff::PointInt _pressDir;
	ff::PointInt _pixel;
	ff::PointInt _dir;

	size_t _speed;
	int _advance;
	int _advancePos;
};

class PacActor : public PlayingActor
{
public:
	PacActor();
	PacActor(const PacActor &rhs);
	~PacActor();

	PacActor &operator=(const PacActor &rhs);

	virtual void Reset() override;

	bool CanTurn() const;
	void SetCanTurn(bool canTurn);

	bool IsStuck() const;
	void SetStuck(bool stuck);

protected:
	virtual void OnTileChanged() override;
	virtual void OnPressDirChanged() override;

	bool _canTurn;
	bool _stuck;
};

class GhostActor : public PlayingActor
{
public:
	GhostActor();
	GhostActor(const GhostActor &rhs);
	~GhostActor();

	GhostActor &operator=(const GhostActor &rhs);

	virtual void Reset() override;

	IGhostBrains *GetBrains();
	void SetBrains(IGhostBrains *brains);

	MoveState GetMoveState() const;
	void SetMoveState(MoveState state);

	HouseState GetHouseState() const;
	void SetHouseState(HouseState state);

	size_t GetDotCounter() const;
	void SetDotCounter(size_t dotCount);

protected:
	size_t _dotCount;
	MoveState _move;
	HouseState _house;
	ff::ComPtr<IGhostBrains> _brains;
};

class FruitActor : public PlayingActor
{
public:
	FruitActor();
	FruitActor(const FruitActor &rhs);
	~FruitActor();

	FruitActor &operator=(const FruitActor &rhs);

	virtual void Reset() override;

	IGhostBrains *GetBrains();
	void SetBrains(IGhostBrains *brains);

	FruitType GetType() const;
	void SetType(FruitType type);

	ff::PointInt GetExitTile() const;
	void SetExitTile(ff::PointInt tile);

protected:
	FruitType _type;
	ff::PointInt _exitTile;
	ff::ComPtr<IGhostBrains> _brains;
};

class PointActor : public PlayingActor
{
public:
	PointActor();
	PointActor(const PointActor &rhs);
	~PointActor();

	PointActor &operator=(const PointActor &rhs);

	virtual void Reset() override;

	void SetCountdown(size_t nFrames);
	bool AdvanceCountdown();

	size_t GetPoints() const;
	void SetPoints(size_t nPoints);

	const DirectX::XMFLOAT4 &GetColor() const;
	void SetColor(const DirectX::XMFLOAT4 &color, bool bFades);

	ff::PointFloat GetScale() const;
	void SetScale(ff::PointFloat scale);

protected:
	size_t _countdown;
	size_t _points;
	DirectX::XMFLOAT4 _color;
	float _fadeAlpha;
	ff::PointFloat _scale;
};

class CustomActor : public PlayingActor
{
public:
	virtual void DeleteThis();
	virtual bool Advance() = 0;
	virtual void Render(ff::I2dRenderer *render) = 0;
};

class SpriteAnimActor : public CustomActor
{
public:
	SpriteAnimActor(
		const wchar_t *animName,
		ff::PointFloat pos,
		ff::PointFloat scale,
		DirectX::XMFLOAT4 color,
		ff::AnimTweenType tweenType,
		ff::PointFloat velocity,
		ff::PointFloat acceleration,
		float timeScale,
		bool forceOpaque);

	static void *operator new(std::size_t size);
	static void operator delete(void *mem);

	virtual void DeleteThis() override;
	virtual bool Advance() override;
	virtual void Render(ff::I2dRenderer *render) override;

private:
	ff::TypedResource<ff::ISpriteAnimation> _anim;
	ff::PointFloat _pos;
	ff::PointFloat _scale;
	ff::PointFloat _velocity;
	ff::PointFloat _accel;
	DirectX::XMFLOAT4 _color;
	ff::AnimTweenType _tweenType;
	float _timeScale;
	float _frame;
	bool _forceOpaque;
};
