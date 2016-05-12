#include "pch.h"
#include "COM/ComObject.h"
#include "Core/Difficulty.h"
#include "Core/Maze.h"
#include "Core/Mazes.h"
#include "Core/Stats.h"
#include "Core/Tiles.h"
#include "Dict/Dict.h"
#include "Module/Module.h"
#include "String/StringUtil.h"

class __declspec(uuid("e4ee88f8-4ef1-4220-bde7-17792124a64c"))
	Mazes : public ff::ComBase, public IMazes
{
public:
	DECLARE_HEADER(Mazes);

	bool Init();

	// IMazes

	virtual ff::StringRef GetID() const override;
	virtual void SetID(ff::StringRef id) override;

	virtual size_t GetMazeCount() const override;
	virtual IMaze *GetMaze(size_t nMaze) const override;
	virtual size_t FindMaze(IMaze *pMaze) const override;

	virtual void AddMaze (size_t nMaze, IMaze *pMaze) override;
	virtual void RemoveMaze(size_t nMaze, IMaze **ppMaze) override;

	virtual size_t GetDifficultyCount() const override;
	virtual const Difficulty &GetDifficulty (size_t nDiff) const;
	virtual void AddDifficulty (size_t nDiff, const Difficulty &diff) override;
	virtual void RemoveDifficulty(size_t nDiff) override;

	virtual size_t GetStartingLives() const override;
	virtual void SetStartingLives(size_t nLives) override;

	virtual size_t GetFreeLifeScore() const override;
	virtual size_t GetFreeLifeRepeat() const override;
	virtual size_t GetMaxFreeLives() const override;
	virtual void SetFreeLifeScore(size_t nScore, size_t nRepeat, size_t nMaxFree) override;

	virtual const Stats& GetStats() const override;
	virtual void SetStats(const Stats &stats) override;

private:
	ff::Vector<ff::ComPtr<IMaze>> _mazes;
	ff::Vector<Difficulty> _diffs;

	ff::String _id;
	size_t _lives;
	size_t _freeLife;
	size_t _freeRepeat;
	size_t _freeMax;
	Stats _stats;
};

BEGIN_INTERFACES(Mazes)
	HAS_INTERFACE(IMazes)
END_INTERFACES()

// static
bool IMazes::Create(IMazes **ppMazes)
{
	assertRetVal(ppMazes, false);
	*ppMazes = nullptr;

	ff::ComPtr<Mazes> pMazes = new ff::ComObject<Mazes>;
	assertRetVal(pMazes->Init(), false);

	*ppMazes = ff::GetAddRef<IMazes>(pMazes);

	return true;
}

bool CreateMazesFromId(ff::StringRef id, IMazes **obj)
{
	ff::String resource = id;
	ff::ComPtr<IMazes> mazes;
	assertRetVal(CreateMazesFromResource(resource, &mazes), false);

	mazes->SetID(id);

	*obj = mazes.Detach();
	return *obj != nullptr;
}

bool CreateMazesFromResource(ff::String name, IMazes **obj)
{
	assertRetVal(obj, false);
	*obj = nullptr;

	ff::ComPtr<IMazes> mazes;
	assertRetVal(IMazes::Create(&mazes), false);

	ff::ValuePtr rawValue = ff::GetThisModule().GetValue(name);
	assertRetVal(rawValue, false);

	ff::ValuePtr dictValue;
	assertRetVal(rawValue->Convert(ff::Value::Type::Dict, &dictValue), false);

	const ff::Dict &dict = dictValue->AsDict();
	size_t lives = dict.GetSize(ff::String(L"lives"), 3);
	size_t firstLife = dict.GetSize(ff::String(L"firstLife"), 10000);
	size_t repeatLife = dict.GetSize(ff::String(L"repeatLife"), 0);
	size_t maxFreeLives = dict.GetSize(ff::String(L"maxFreeLives"), 1);

	ff::ValuePtr mazesValue = dict.GetValue(ff::String(L"mazes"));
	ff::ValuePtr diffsValue = dict.GetValue(ff::String(L"difficulties"));

	mazes->SetStartingLives(lives);
	mazes->SetFreeLifeScore(firstLife, repeatLife, maxFreeLives);

	ff::ValuePtr mazesStrings;
	assertRetVal(mazesValue && mazesValue->Convert(ff::Value::Type::StringVector, &mazesStrings), false);
	for (ff::StringRef mazeName : mazesStrings->AsStringVector())
	{
		ff::ComPtr<IMaze> maze;
		assertRetVal(CreateMazeFromResource(mazeName, &maze), false);
		mazes->AddMaze(mazes->GetMazeCount(), maze);
	}

	assertRetVal(diffsValue && diffsValue->IsType(ff::Value::Type::ValueVector), false);
	for (ff::ValuePtr diffValue : diffsValue->AsValueVector())
	{
		ff::ValuePtr diffDictValue;
		assertRetVal(diffValue->Convert(ff::Value::Type::Dict, &diffDictValue), false);

		ff::Vector<Difficulty> diffs;
		assertRetVal(CreateDifficultiesFromDict(diffDictValue->AsDict(), diffs), false);

		for (Difficulty &diff : diffs)
		{
			size_t index = mazes->GetDifficultyCount();
			mazes->AddDifficulty(index, diff);
		}
	}

	*obj = mazes.Detach();
	return *obj != nullptr;
}

Mazes::Mazes()
	: _lives(3)
	, _freeLife(10000)
	, _freeRepeat(0)
	, _freeMax(1)
{
	_id = ff::StringFromGuid(ff::CreateGuid());
}

Mazes::~Mazes()
{
}

bool Mazes::Init()
{
	return true;
}

ff::StringRef Mazes::GetID() const
{
	return _id;
}

void Mazes::SetID(ff::StringRef id)
{
	_id = id;
}

size_t Mazes::GetMazeCount() const
{
	return _mazes.Size();
}

IMaze *Mazes::GetMaze(size_t nMaze) const
{
	assertRetVal(nMaze >= 0 && nMaze < _mazes.Size(), nullptr);

	return _mazes[nMaze];
}

size_t Mazes::FindMaze(IMaze *pMaze) const
{
	return _mazes.Find(ff::ComPtr<IMaze>(pMaze));
}

void Mazes::AddMaze(size_t nMaze, IMaze *pMaze)
{
	assertRet(pMaze);

	nMaze = std::min(nMaze, _mazes.Size());

	_mazes.Insert(nMaze, ff::ComPtr<IMaze>(pMaze));
}

void Mazes::RemoveMaze(size_t nMaze, IMaze **ppMaze)
{
	assertRet(nMaze >= 0 && nMaze < _mazes.Size());

	if (ppMaze)
	{
		*ppMaze = ff::GetAddRef<IMaze>(_mazes[nMaze]);
	}

	_mazes.Delete(nMaze);
}

size_t Mazes::GetDifficultyCount() const
{
	return _diffs.Size();
}

// STATIC_DATA(object)
static Difficulty s_badDiff = { 0 };

const Difficulty &Mazes::GetDifficulty(size_t nDiff) const
{
	if (nDiff < _diffs.Size())
	{
		return _diffs[nDiff];
	}
	else if (_diffs.Size())
	{
		return _diffs[_diffs.Size() - 1];
	}
	else
	{
		return GetEmptyDifficulty();
	}
}

void Mazes::AddDifficulty(size_t nDiff, const Difficulty &diff)
{
	nDiff = std::min(nDiff, _diffs.Size());

	_diffs.Insert(nDiff, diff);
}

void Mazes::RemoveDifficulty(size_t nDiff)
{
	assertRet(nDiff >= 0 && nDiff < _diffs.Size());

	_diffs.Delete(nDiff);
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

const Stats &Mazes::GetStats() const
{
	return _stats;
}

void Mazes::SetStats(const Stats &stats)
{
	_stats = stats;
}
