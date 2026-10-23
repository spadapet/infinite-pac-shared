#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <array>
#define assert_ret_val(condition, value) do { if (!(condition)) { assert(false); return value; } } while (0)
#define _countof(value) (sizeof(value) / sizeof((value)[0]))
namespace ff {
template<class T> struct point_t {
    T x{}, y{};
    point_t() = default;
    point_t(T x, T y) : x(x), y(y) {}
    template<class U> point_t<U> cast() const { return {static_cast<U>(x), static_cast<U>(y)}; }
    point_t operator+(point_t b) const { return {x + b.x, y + b.y}; }
    point_t operator-(point_t b) const { return {x - b.x, y - b.y}; }
    point_t operator-() const { return {-x, -y}; }
    point_t& operator+=(point_t b) { x += b.x; y += b.y; return *this; }
    bool operator==(point_t b) const { return x == b.x && y == b.y; }
    bool operator!=(point_t b) const { return !(*this == b); }
};
using point_int = point_t<int>;
using point_float = point_t<float>;
struct rect_int { int top{}, bottom{}; };
template<class T, size_t N> using stack_vector = std::vector<T>;
namespace constants {
template<class T> constexpr T updates_per_second() { return 60; }
template<class T> constexpr T invalid_unsigned() { return static_cast<T>(-1); }
}
}
enum FruitType { FRUIT_0, FRUIT_1, FRUIT_2, FRUIT_3, FRUIT_4, FRUIT_5, FRUIT_6, FRUIT_7, FRUIT_8, FRUIT_9, FRUIT_10, FRUIT_11, FRUIT_RANDOM, FRUIT_NONE };
enum CharType { CHAR_MR, CHAR_MS };
enum GhostState { GHOST_INVALID, GHOST_CHASE, GHOST_SCATTER, GHOST_EYES, GHOST_SCARED, GHOST_SCARED_FLASH };
enum FruitState { FRUIT_INVALID, FRUIT_MOVING, FRUIT_EXITING };
enum TileZone { ZONE_NORMAL, ZONE_GHOST_SLOW, ZONE_GHOST_NO_TURN, ZONE_OUT_OF_BOUNDS };
enum TileContent { CONTENT_NOTHING, CONTENT_WALL, CONTENT_GHOST_WALL, CONTENT_GHOST_DOOR };
class Maze {
public:
    int width = 28, height = 31;
    std::vector<int> contents, zones;
    ff::point_int GetSizeInTiles() const { return {width, height}; }
    TileZone GetTileZone(ff::point_int tile) const {
        if (tile.x < 0 || tile.y < 0 || tile.x >= width || tile.y >= height) return ZONE_OUT_OF_BOUNDS;
        return static_cast<TileZone>(zones[tile.y * width + tile.x]);
    }
    TileContent GetTileContent(ff::point_int tile) const {
        if (tile.x < 0 || tile.y < 0 || tile.x >= width || tile.y >= height) return CONTENT_NOTHING;
        return static_cast<TileContent>(contents[tile.y * width + tile.x]);
    }
};
class Difficulty {
public:
    size_t _pacSpeed = 70, _pacSubtract = 0, _pacAdd = 15, _elroyDots = 20, _scaredSeconds = 10, _ghostDotCounter[4]{}, _lastDotSeconds = 4;
    FruitType _fruit = FRUIT_0;
    size_t GetPacSpeed(bool) const;
    size_t GetFruitSpeed() const;
    size_t GetGhostSpeed(MoveState, HouseState, bool, bool, size_t) const;
    size_t GetScaredFrames() const;
    size_t GetEatenFrames() const;
    size_t GetGhostDotCounter(size_t) const;
    size_t GetGlobalDotCounter(size_t) const;
    size_t GetLastDotFrames() const;
    size_t GetFruitFrames(CharType) const;
    FruitType GetFruit() const;
    bool IsFruitMoving(CharType) const;
    bool HasRandomGhostMovement(CharType) const;
};
class IPlayingMaze;
class IGhostBrains {
public:
    virtual ~IGhostBrains() = default;
    static std::shared_ptr<IGhostBrains> Create(size_t);
    virtual ff::point_int Decide(IPlayingMaze*, const ff::point_int*, size_t) = 0;
    virtual ff::point_int GetTargetPixel(IPlayingMaze*) = 0;
};
class IPlayingMaze {
public:
    virtual std::shared_ptr<Maze> GetMaze() = 0;
    virtual GhostState GetGhostState(size_t) const = 0;
    virtual const Difficulty& GetDifficulty() const = 0;
    virtual CharType GetCharType() const = 0;
    virtual std::shared_ptr<IPlayingActor> GetPac() = 0;
    virtual std::shared_ptr<IPlayingActor> GetGhost(size_t) = 0;
    virtual std::shared_ptr<IPlayingActor> GetFruit() = 0;
    virtual ff::point_int GetGhostStartPixel() const = 0;
    virtual ff::point_int GetFruitExitTile() const = 0;
    virtual FruitState GetFruitState() const = 0;
};
struct OracleHost { void OnPacUsingTunnel() {} };
class PlayingMaze : public IPlayingMaze {
public:
    std::shared_ptr<Maze> _maze = std::make_shared<Maze>();
    std::shared_ptr<PacActor> _pac = std::make_shared<PacActor>();
    std::shared_ptr<GhostActor> _ghosts[4];
    std::shared_ptr<FruitActor> _fruit = std::make_shared<FruitActor>();
    ff::point_int _ghostStartPixel{112, 92};
    ff::rect_int _ghostHouseRect{112, 120};
    Difficulty _difficulty;
    CharType character = CHAR_MR;
    OracleHost* _host = nullptr;
    struct { size_t _tunnelsUsed{}; } _stats;
    size_t _ghostScaredCountdown{}, _ghostEatenCountdown{}, _ghostEatenIndex{}, _ghostScatterCountdown{}, _ghostChaseCountdown{}, _ghostScatterChaseIndex{};
    size_t _lastDotCounter{}, _globalDotIndex{}, _globalDotCounter{}, _dotCount{200};
    bool exiting = false;
    PlayingMaze() { for (size_t i = 0; i < 4; ++i) { _ghosts[i] = std::make_shared<GhostActor>(); } }
    void AdvancePac(PacActor&);
    void AdvanceGhost(GhostActor&);
    void AdvanceFruit(FruitActor&);
    ff::point_int GhostDecidePress(GhostActor&);
    ff::point_int GhostDecidePress(IGhostBrains*, MoveState, ff::point_int, ff::point_int);
    ff::point_int FruitDecideDir(FruitActor&);
    bool IsWall(ff::point_int);
    bool HitWall(ff::point_int, ff::point_int);
    bool CheckTunnel(PlayingActor&);
    void PlayEffect(int) {}
    size_t GhostIndex(GhostActor& ghost) { for (size_t i = 0; i < 4; i++) if (_ghosts[i].get() == &ghost) return i; std::abort(); }
    void FlipGhosts();
    void ScareGhosts(bool);
    bool ReleaseGhost();
    size_t CountGhostsInHouse();
    void UpdateGhostMode();
    void UpdateGhostHouse();
    void UpdateActorSpeeds();
    void UpdateScatterChaseTimes(size_t index) { _ghostScatterChaseIndex = index; _ghostScatterCountdown = 60; _ghostChaseCountdown = 18000; }
    std::shared_ptr<Maze> GetMaze() override { return _maze; }
    GhostState GetGhostState(size_t index) const override {
        const auto& ghost = *_ghosts[index];
        if (!ghost.IsActive() || ghost.GetMoveState() == MOVE_EATEN || ghost.GetMoveState() == MOVE_WAITING_TO_BE_EATEN) return GHOST_INVALID;
        if (ghost.GetMoveState() == MOVE_SCARED) return GHOST_SCARED;
        if (ghost.GetMoveState() == MOVE_EYES) return GHOST_EYES;
        return !_ghostScatterCountdown && _pac->IsActive() ? GHOST_CHASE : GHOST_SCATTER;
    }
    const Difficulty& GetDifficulty() const override { return _difficulty; }
    CharType GetCharType() const override { return character; }
    std::shared_ptr<IPlayingActor> GetPac() override { return _pac; }
    std::shared_ptr<IPlayingActor> GetGhost(size_t index) override { return _ghosts[index]; }
    std::shared_ptr<IPlayingActor> GetFruit() override { return _fruit; }
    ff::point_int GetGhostStartPixel() const override { return _ghostStartPixel; }
    ff::point_int GetFruitExitTile() const override { return _fruit->GetExitTile(); }
    FruitState GetFruitState() const override { return exiting ? FRUIT_EXITING : FRUIT_MOVING; }
};
constexpr int EFFECT_FRUIT_BOUNCE = 0;
ff::point_int DecideForTarget(ff::point_int, const ff::point_int*, size_t);
