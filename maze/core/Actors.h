#pragma once

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
    virtual ~IPlayingActor() = default;

    virtual ff::point_int GetTile() const = 0;
    virtual ff::point_int GetPixel() const = 0;
    virtual void SetPixel(ff::point_int pixel) = 0;

    virtual ff::point_int GetDir() const = 0;
    virtual void SetDir(ff::point_int dir) = 0;

    virtual ff::point_int GetPressDir() const = 0;
    virtual void SetPressDir(ff::point_int dir) = 0;

    virtual bool IsActive() const = 0;
    virtual void SetActive(bool bActive) = 0;
};

// Represents something that moves in the maze
class PlayingActor : public IPlayingActor
{
public:
    PlayingActor();
    PlayingActor(const PlayingActor& rhs);
    ~PlayingActor();

    PlayingActor& operator=(const PlayingActor& rhs);

    virtual void Reset(); // between lives

    size_t GetAdvanceCount();
    void SetSpeed(size_t speed); // 0 - 100
    void AddDelay(size_t delay);

    // IPlayingActor

    virtual ff::point_int GetTile() const override;
    virtual ff::point_int GetPixel() const override;
    virtual void SetPixel(ff::point_int pixel) override;

    virtual ff::point_int GetDir() const override;
    virtual void SetDir(ff::point_int dir) override;

    virtual ff::point_int GetPressDir() const override;
    virtual void SetPressDir(ff::point_int dir) override;

    virtual bool IsActive() const override;
    virtual void SetActive(bool bActive) override;

protected:
    virtual void OnTileChanged();
    virtual void OnPressDirChanged();

    bool _active;
    ff::point_int _pressDir;
    ff::point_int _pixel;
    ff::point_int _dir;

    size_t _speed;
    int _advance;
    int _advancePos;
};

class PacActor : public PlayingActor
{
public:
    PacActor();
    PacActor(const PacActor& rhs);
    ~PacActor();

    PacActor& operator=(const PacActor& rhs);

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
    GhostActor(const GhostActor& rhs);
    ~GhostActor();

    GhostActor& operator=(const GhostActor& rhs);

    virtual void Reset() override;

    IGhostBrains* GetBrains();
    void SetBrains(std::shared_ptr<IGhostBrains> brains);

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
    std::shared_ptr<IGhostBrains> _brains;
};

class FruitActor : public PlayingActor
{
public:
    FruitActor();
    FruitActor(const FruitActor& rhs);
    ~FruitActor();

    FruitActor& operator=(const FruitActor& rhs);

    virtual void Reset() override;

    IGhostBrains* GetBrains();
    void SetBrains(std::shared_ptr<IGhostBrains> brains);

    FruitType GetType() const;
    void SetType(FruitType type);

    ff::point_int GetExitTile() const;
    void SetExitTile(ff::point_int tile);

protected:
    FruitType _type;
    ff::point_int _exitTile;
    std::shared_ptr<IGhostBrains> _brains;
};

class PointActor : public PlayingActor
{
public:
    PointActor();
    PointActor(const PointActor& rhs);
    ~PointActor();

    PointActor& operator=(const PointActor& rhs);

    virtual void Reset() override;

    void SetCountdown(size_t nFrames);
    bool AdvanceCountdown();

    size_t GetPoints() const;
    void SetPoints(size_t nPoints);

    const DirectX::XMFLOAT4& GetColor() const;
    void SetColor(const DirectX::XMFLOAT4& color, bool bFades);

    ff::point_float GetScale() const;
    void SetScale(ff::point_float scale);

protected:
    size_t _countdown;
    size_t _points;
    DirectX::XMFLOAT4 _color;
    float _fadeAlpha;
    ff::point_float _scale;
};

class CustomActor : public PlayingActor
{
public:
    virtual bool Advance() = 0;
    virtual void Render(ff::dxgi::draw_base& draw) = 0;
};

class SpriteAnimActor : public CustomActor
{
public:
    SpriteAnimActor(
        std::string_view animName,
        ff::point_float pos,
        ff::point_float scale,
        DirectX::XMFLOAT4 color,
        bool looping,
        ff::point_float velocity,
        ff::point_float acceleration,
        float timeScale,
        bool forceOpaque);

    virtual bool Advance() override;
    virtual void Render(ff::dxgi::draw_base& draw) override;

private:
    ff::auto_resource<ff::animation_base> _anim;
    ff::point_float _pos;
    ff::point_float _scale;
    ff::point_float _velocity;
    ff::point_float _accel;
    DirectX::XMFLOAT4 _color;
    bool _looping;
    float _timeScale;
    float _frame;
    bool _forceOpaque;
};
