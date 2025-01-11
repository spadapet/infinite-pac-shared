#include "pch.h"
#include "Core/Difficulty.h"
#include "Core/Maze.h"
#include "Core/Mazes.h"
#include "Core/Stats.h"
#include "Core/Tiles.h"

class Mazes : public IMazes
{
public:
    Mazes();
    virtual ~Mazes() override;

    bool Init();

    // IMazes

    virtual std::string_view GetID() const override;
    virtual void SetID(std::string_view id) override;

    virtual size_t GetMazeCount() const override;
    virtual std::shared_ptr<IMaze> GetMaze(size_t nMaze) const override;
    virtual size_t FindMaze(IMaze* pMaze) const override;

    virtual void AddMaze(size_t nMaze, std::shared_ptr<IMaze> pMaze) override;
    virtual std::shared_ptr<IMaze> RemoveMaze(size_t nMaze) override;

    virtual size_t GetDifficultyCount() const override;
    virtual const Difficulty& GetDifficulty(size_t nDiff) const;
    virtual void AddDifficulty(size_t nDiff, const Difficulty& diff) override;
    virtual void RemoveDifficulty(size_t nDiff) override;

    virtual size_t GetStartingLives() const override;
    virtual void SetStartingLives(size_t nLives) override;

    virtual size_t GetFreeLifeScore() const override;
    virtual size_t GetFreeLifeRepeat() const override;
    virtual size_t GetMaxFreeLives() const override;
    virtual void SetFreeLifeScore(size_t nScore, size_t nRepeat, size_t nMaxFree) override;

    virtual const Stats& GetStats() const override;
    virtual void SetStats(const Stats& stats) override;

private:
    std::vector<std::shared_ptr<IMaze>> _mazes;
    std::vector<Difficulty> _diffs;

    std::string _id;
    size_t _lives;
    size_t _freeLife;
    size_t _freeRepeat;
    size_t _freeMax;
    Stats _stats;
};

std::shared_ptr<IMazes> IMazes::Create()
{
    return std::make_shared<Mazes>();
}

std::shared_ptr<IMazes> CreateMazesFromId(std::string_view id)
{
    static std::shared_ptr<ff::resource_values> values_mazes;
    if (!values_mazes)
    {
        values_mazes = ff::auto_resource<ff::resource_values>("values_mazes").object();
    }

    ff::value_ptr rawValue = values_mazes->get_resource_value(id);
    assert_ret_val(rawValue, nullptr);

    ff::value_ptr dictValue = rawValue->try_convert<ff::dict>();
    assert_ret_val(dictValue, nullptr);

    const ff::dict& dict = dictValue->get<ff::dict>();
    size_t lives = dict.get<size_t>("lives", 3);
    size_t firstLife = dict.get<size_t>("firstLife", 10000);
    size_t repeatLife = dict.get<size_t>("repeatLife", 0);
    size_t maxFreeLives = dict.get<size_t>("maxFreeLives", 1);

    ff::value_ptr mazesValue = dict.get("mazes");
    ff::value_ptr diffsValue = dict.get("difficulties");

    std::shared_ptr<IMazes> mazes = IMazes::Create();
    mazes->SetStartingLives(lives);
    mazes->SetFreeLifeScore(firstLife, repeatLife, maxFreeLives);
    mazes->SetID(id);

    ff::value_ptr mazesStrings = mazesValue->try_convert<std::vector<std::string>>();
    assert_ret_val(mazesStrings, nullptr);

    for (std::string_view mazeName : mazesStrings->get<std::vector<std::string>>())
    {
        std::shared_ptr<IMaze> maze = CreateMazeFromResource(mazeName);
        assert_ret_val(maze, nullptr);
        mazes->AddMaze(mazes->GetMazeCount(), maze);
    }

    assert_ret_val(diffsValue && diffsValue->is_type<std::vector<ff::value_ptr>>(), nullptr);
    for (ff::value_ptr diffValue : diffsValue->get<std::vector<ff::value_ptr>>())
    {
        ff::value_ptr diffDictValue = diffValue->try_convert<ff::dict>();
        assert_ret_val(diffDictValue, nullptr);

        std::vector<Difficulty> diffs;
        assert_ret_val(CreateDifficultiesFromDict(diffDictValue->get<ff::dict>(), diffs), nullptr);

        for (Difficulty& diff : diffs)
        {
            size_t index = mazes->GetDifficultyCount();
            mazes->AddDifficulty(index, diff);
        }
    }

    return mazes;
}

Mazes::Mazes()
    : _lives(3)
    , _freeLife(10000)
    , _freeRepeat(0)
    , _freeMax(1)
{
    _id = ff::uuid::create().to_string();
}

Mazes::~Mazes()
{
}

bool Mazes::Init()
{
    return true;
}

std::string_view Mazes::GetID() const
{
    return _id;
}

void Mazes::SetID(std::string_view id)
{
    _id = id;
}

size_t Mazes::GetMazeCount() const
{
    return _mazes.size();
}

std::shared_ptr<IMaze> Mazes::GetMaze(size_t nMaze) const
{
    assert_ret_val(nMaze >= 0 && nMaze < _mazes.size(), nullptr);

    return _mazes[nMaze];
}

size_t Mazes::FindMaze(IMaze* pMaze) const
{
    for (size_t i = 0; i < _mazes.size(); i++)
    {
        if (_mazes[i].get() == pMaze)
        {
            return i;
        }
    }

    return ff::constants::invalid_unsigned<size_t>();
}

void Mazes::AddMaze(size_t nMaze, std::shared_ptr<IMaze> pMaze)
{
    assert_ret(pMaze);

    nMaze = std::min(nMaze, _mazes.size());

    _mazes.insert(_mazes.begin() + nMaze, pMaze);
}

std::shared_ptr<IMaze> Mazes::RemoveMaze(size_t nMaze)
{
    std::shared_ptr<IMaze> maze;
    assert_ret_val(nMaze >= 0 && nMaze < _mazes.size(), maze);

    maze = _mazes[nMaze];
    _mazes.erase(_mazes.begin() + nMaze);
    return maze;
}

size_t Mazes::GetDifficultyCount() const
{
    return _diffs.size();
}

// STATIC_DATA(object)
static Difficulty s_badDiff = { 0 };

const Difficulty& Mazes::GetDifficulty(size_t nDiff) const
{
    if (nDiff < _diffs.size())
    {
        return _diffs[nDiff];
    }
    else if (_diffs.size())
    {
        return _diffs[_diffs.size() - 1];
    }
    else
    {
        return GetEmptyDifficulty();
    }
}

void Mazes::AddDifficulty(size_t nDiff, const Difficulty& diff)
{
    nDiff = std::min(nDiff, _diffs.size());

    _diffs.insert(_diffs.begin() + nDiff, diff);
}

void Mazes::RemoveDifficulty(size_t nDiff)
{
    assert_ret(nDiff >= 0 && nDiff < _diffs.size());

    _diffs.erase(_diffs.begin() + nDiff);
}

size_t Mazes::GetStartingLives() const
{
    return _lives;
}

void Mazes::SetStartingLives(size_t nLives)
{
    _lives = nLives;
}

size_t Mazes::GetFreeLifeScore() const
{
    return _freeLife;
}

size_t Mazes::GetFreeLifeRepeat() const
{
    return _freeRepeat;
}

size_t Mazes::GetMaxFreeLives() const
{
    return _freeMax;
}

void Mazes::SetFreeLifeScore(size_t nScore, size_t nRepeat, size_t nMaxFree)
{
    _freeLife = nScore;
    _freeRepeat = nRepeat;
    _freeMax = nMaxFree;
}

const Stats& Mazes::GetStats() const
{
    return _stats;
}

void Mazes::SetStats(const Stats& stats)
{
    _stats = stats;
}
