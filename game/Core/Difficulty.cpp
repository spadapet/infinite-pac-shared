#include "pch.h"
#include "Core/Actors.h"
#include "Core/Difficulty.h"
#include "Core/Helpers.h"
#include "Dict/Dict.h"

static const Difficulty s_emptyDifficulty
{
	4, // _ghostCount
	80, // _pacSpeed
	0, // _pacSubtract
	0, // _pacAdd
	20, // _elroyDots
	6, // _scaredSeconds
	{ 7, 20, 7, 20, 5, 20, 5, 0 }, // _ghostModeSeconds
	{ 0, 0, 30, 60 }, // _ghostDotCounter
	4, // _lastDotSeconds
	FRUIT_0 // _fruit
};

const Difficulty &GetEmptyDifficulty()
{
	return s_emptyDifficulty;
}

static ff::StaticString PROP_GHOSTS(L"ghosts");
static ff::StaticString PROP_SPEED(L"speed");
static ff::StaticString PROP_SUBTRACT(L"speedSubtract");
static ff::StaticString PROP_ADD(L"speedAdd");
static ff::StaticString PROP_ELROY(L"elroyDots");
static ff::StaticString PROP_SCARED(L"scaredSeconds");
static ff::StaticString PROP_LAST_DOT(L"lastDotSecondsUntilGhost");
static ff::StaticString PROP_FRUIT(L"fruit");
static ff::StaticString PROP_GHOST_MODE(L"ghostModeSeconds");
static ff::StaticString PROP_GHOST_DOTS(L"ghostDots");

static bool GetDifficultyBasics(const ff::Dict &dict, ff::Vector<Difficulty> &diffs)
{
	Difficulty diff = GetEmptyDifficulty();

	diff._ghostCount = dict.GetSize(PROP_GHOSTS, diff._ghostCount);
	diff._pacSpeed = dict.GetSize(PROP_SPEED, diff._pacSpeed);
	diff._pacSubtract = dict.GetSize(PROP_SUBTRACT, diff._pacSubtract);
	diff._pacAdd = dict.GetSize(PROP_ADD, diff._pacAdd);
	diff._elroyDots = dict.GetSize(PROP_ELROY, diff._elroyDots);
	diff._scaredSeconds = dict.GetSize(PROP_SCARED, diff._scaredSeconds);
	diff._lastDotSeconds = dict.GetSize(PROP_LAST_DOT, diff._lastDotSeconds);
	diff._fruit = dict.GetEnum<FruitType>(PROP_FRUIT, diff._fruit);
	assertRetVal(diff._fruit >= FRUIT_0 && diff._fruit <= FRUIT_NONE, false);

	diffs.Push(diff);

	return true;
}

static bool GetDifficultyVectors(const ff::Dict &dict, ff::Vector<Difficulty> &diffs)
{
	ff::ValuePtr ghosts;
	ff::ValuePtr speeds;
	ff::ValuePtr subs;
	ff::ValuePtr adds;
	ff::ValuePtr elroys;
	ff::ValuePtr scareds;
	ff::ValuePtr lastDots;
	ff::ValuePtr fruits;

	assertRetVal(dict.GetValue(PROP_GHOSTS) && dict.GetValue(PROP_GHOSTS)->Convert(ff::Value::Type::IntVector, &ghosts), false);
	assertRetVal(dict.GetValue(PROP_SPEED) && dict.GetValue(PROP_SPEED)->Convert(ff::Value::Type::IntVector, &speeds), false);
	assertRetVal(dict.GetValue(PROP_SUBTRACT) && dict.GetValue(PROP_SUBTRACT)->Convert(ff::Value::Type::IntVector, &subs), false);
	assertRetVal(dict.GetValue(PROP_ADD) && dict.GetValue(PROP_ADD)->Convert(ff::Value::Type::IntVector, &adds), false);
	assertRetVal(dict.GetValue(PROP_ELROY) && dict.GetValue(PROP_ELROY)->Convert(ff::Value::Type::IntVector, &elroys), false);
	assertRetVal(dict.GetValue(PROP_SCARED) && dict.GetValue(PROP_SCARED)->Convert(ff::Value::Type::IntVector, &scareds), false);
	assertRetVal(dict.GetValue(PROP_LAST_DOT) && dict.GetValue(PROP_LAST_DOT)->Convert(ff::Value::Type::IntVector, &lastDots), false);
	assertRetVal(dict.GetValue(PROP_FRUIT) && dict.GetValue(PROP_FRUIT)->Convert(ff::Value::Type::IntVector, &fruits), false);

	size_t size = ghosts->AsIntVector().Size();
	assertRetVal(size, false);

	assertRetVal(speeds->AsIntVector().Size() == size, false);
	assertRetVal(subs->AsIntVector().Size() == size, false);
	assertRetVal(adds->AsIntVector().Size() == size, false);
	assertRetVal(elroys->AsIntVector().Size() == size, false);
	assertRetVal(scareds->AsIntVector().Size() == size, false);
	assertRetVal(lastDots->AsIntVector().Size() == size, false);
	assertRetVal(fruits->AsIntVector().Size() == size, false);

	for (size_t i = 0; i < size; i++)
	{
		Difficulty diff = GetEmptyDifficulty();

		diff._ghostCount = (size_t)ghosts->AsIntVector().GetAt(i);
		diff._pacSpeed = (size_t)speeds->AsIntVector().GetAt(i);
		diff._pacSubtract = (size_t)subs->AsIntVector().GetAt(i);
		diff._pacAdd = (size_t)adds->AsIntVector().GetAt(i);
		diff._elroyDots = (size_t)elroys->AsIntVector().GetAt(i);
		diff._scaredSeconds = (size_t)scareds->AsIntVector().GetAt(i);
		diff._lastDotSeconds = (size_t)lastDots->AsIntVector().GetAt(i);
		diff._fruit = (FruitType)fruits->AsIntVector().GetAt(i);

		diffs.Push(diff);
	}

	return !diffs.IsEmpty();
}

bool CreateDifficultiesFromDict(const ff::Dict &dict, ff::Vector<Difficulty> &diffs)
{
	diffs.Clear();

	if (dict.GetValue(PROP_SPEED) && dict.GetValue(PROP_SPEED)->IsType(ff::Value::Type::ValueVector))
	{
		assertRetVal(GetDifficultyVectors(dict, diffs), false);
	}
	else
	{
		assertRetVal(GetDifficultyBasics(dict, diffs), false);
	}

	ff::ValuePtr modeValue = dict.GetValue(PROP_GHOST_MODE);
	if (modeValue)
	{
		ff::ValuePtr vectorValue;
		assertRetVal(modeValue->Convert(ff::Value::Type::IntVector, &vectorValue), false);

		for (size_t i = 0; i < _countof(Difficulty::_ghostModeSeconds) && i < vectorValue->AsIntVector().Size(); i++)
		{
			for (Difficulty &diff : diffs)
			{
				diff._ghostModeSeconds[i] = (size_t)vectorValue->AsIntVector().GetAt(i);
			}
		}
	}

	ff::ValuePtr dotsValue = dict.GetValue(PROP_GHOST_DOTS);
	if (dotsValue)
	{
		ff::ValuePtr vectorValue;
		assertRetVal(dotsValue->Convert(ff::Value::Type::IntVector, &vectorValue), false);

		for (size_t i = 0; i < _countof(Difficulty::_ghostDotCounter) && i < vectorValue->AsIntVector().Size(); i++)
		{
			for (Difficulty &diff : diffs)
			{
				diff._ghostDotCounter[i] = (size_t)vectorValue->AsIntVector().GetAt(i);
			}
		}
	}

	return !diffs.IsEmpty();
}

Difficulty &Difficulty::operator=(const Difficulty &rhs)
{
	if (this != &rhs)
	{
		CopyMemory(this, &rhs, sizeof(*this));
	}

	return *this;
}

size_t Difficulty::GetPacSpeed(bool bPower) const
{
	size_t speed = _pacSpeed - _pacSubtract + _pacAdd;

	if (bPower)
	{
		size_t speed2 = (_pacSpeed + 100) / 2;
		speed = std::max(speed, speed2);
	}

	return speed;
}

size_t Difficulty::GetFruitSpeed() const
{
	return _pacSpeed / 2;
}

size_t Difficulty::GetGhostSpeed(
		MoveState move,
		HouseState house,
		bool bTunnel,
		bool bElroy,
		size_t nDotsLeft) const
{
	if (move == MOVE_EYES)
	{
		return std::max<size_t>(150, _pacSpeed - 5);
	}
	else if (bTunnel || house != HOUSE_OUTSIDE)
	{
		return _pacSpeed / 2;
	}
	else if (move == MOVE_SCARED)
	{
		return _pacSpeed / 2 + 10;
	}
	if (bElroy && nDotsLeft <= _elroyDots / 2)
	{
		return _pacSpeed + 5;
	}
	else if (bElroy && nDotsLeft <= _elroyDots)
	{
		return _pacSpeed;
	}
	else
	{
		return _pacSpeed - 5;
	}
}

size_t Difficulty::GetScaredFrames() const
{
	return _scaredSeconds * IdealFramesPerSecond();
}

size_t Difficulty::GetEatenFrames() const
{
	return IdealFramesPerSecond() * 9 / 10;
}

size_t Difficulty::GetGhostDotCounter(size_t nGhost) const
{
	assertRetVal(nGhost >= 0 && nGhost < _countof(_ghostDotCounter), 0);

	return _ghostDotCounter[nGhost];
}

size_t Difficulty::GetGlobalDotCounter(size_t nIndex) const
{
	switch (nIndex)
	{
	default:
	case 0: return 0;
	case 1: return 7;
	case 2: return 17;
	case 3: return 32;
	}
}

size_t Difficulty::GetLastDotFrames() const
{
	return _lastDotSeconds * IdealFramesPerSecond();
}

size_t Difficulty::GetFruitFrames(CharType type) const
{
	int seconds = IsFruitMoving(type) ? 14 : 9;
	return seconds * IdealFramesPerSecond() + (rand() % IdealFramesPerSecond());
}

FruitType Difficulty::GetFruit() const
{
	return _fruit;
}

bool Difficulty::IsFruitMoving(CharType type) const
{
	return true;
}

bool Difficulty::HasRandomGhostMovement(CharType type) const
{
	return type == CHAR_MS;
}

void Difficulty::GetGhostModeFrames(size_t nIndex, size_t &nScatter, size_t &nChase) const
{
	size_t nMinScatter = 1 * IdealFramesPerSecond();
	size_t nMaxChase = 5 * 60 * IdealFramesPerSecond();

	if (nIndex >= 0 && nIndex < _countof(_ghostModeSeconds) / 2)
	{
		nScatter = _ghostModeSeconds[nIndex * 2] * IdealFramesPerSecond();
		nChase = _ghostModeSeconds[nIndex * 2 + 1] * IdealFramesPerSecond();

		nScatter = nScatter ? std::max<size_t>(nMinScatter, nScatter) : nMinScatter;
		nChase = nChase ? std::min<size_t>(nMaxChase, nChase) : nMaxChase;
	}
	else
	{
		nScatter = nMinScatter;
		nChase = nMaxChase;
	}
}

void Difficulty::GetFruitDotCount(size_t nTotalDots, size_t &nFruit1, size_t &nFruit2) const
{
	nFruit1 = nTotalDots * 143 / 200;
	nFruit2 = nTotalDots * 61 / 200;
}

size_t Difficulty::GetDotPoints() const
{
	return 10;
}

size_t Difficulty::GetPowerPoints() const
{
	return 50;
}

size_t Difficulty::GetGhostPoints(size_t nGhost) const
{
	switch (nGhost)
	{
	default:
	case 0: return 200;
	case 1: return 400;
	case 2: return 800;
	case 3: return 1600;
	}
}

size_t Difficulty::GetFruitPoints(FruitType fruit) const
{
	switch (fruit)
	{
	case FRUIT_0: return 100;
	case FRUIT_1: return 300;
	case FRUIT_2: return 500;
	case FRUIT_3: return 700;
	case FRUIT_4: return 1000;
	case FRUIT_5: return 1500;
	case FRUIT_6: return 2000;
	case FRUIT_7: return 2500;
	case FRUIT_8: return 3000;
	case FRUIT_9: return 3500;
	case FRUIT_10: return 4000;
	case FRUIT_11: return 5000;
	default: return 0;
	}
}
