#include "pch.h"
#include "Core/Stats.h"
#include "Data/Data.h"
#include "Dict/Dict.h"
#include "Globals/MetroGlobals.h"
#include "Windows/FileUtil.h"
#include "String/StringUtil.h"

// STATIC_DATA(object)
static ff::Map<ff::String, Stats> s_stats;

Stats::Stats()
{
	ZeroMemory(this, sizeof(*this));
}

Stats::Stats(const Stats &rhs)
{
	*this = rhs;
}

Stats &Stats::operator=(const Stats &rhs)
{
	if (this != &rhs)
	{
		CopyMemory(this, &rhs, sizeof(*this));
	}

	return *this;
}

Stats &Stats::operator+=(const Stats &rhs)
{
	_cheated = _cheated || rhs._cheated;

	_gamesStarted += rhs._gamesStarted;
	_levelsBeaten += rhs._levelsBeaten;
	_dotsEaten += rhs._dotsEaten;
	_powerEaten += rhs._powerEaten;
	_fruitsEaten += rhs._fruitsEaten;
	_tunnelsUsed += rhs._tunnelsUsed;
	_score += rhs._score;

	for (size_t i = 0; i < _countof(_ghostsEaten); i++)
	{
		_ghostsEaten[i] += rhs._ghostsEaten[i];
		_ghostDeathCount[i] += rhs._ghostDeathCount[i];
	}

	return *this;
}

size_t Stats::GetHighScoreSlot(size_t nScore)
{
	for (size_t i = 0; i < _countof(_highScores); i++)
	{
		if (nScore > _highScores[i]._score)
		{
			return i;
		}
	}

	return ff::INVALID_SIZE;
}

void Stats::InsertHighScore(size_t nScore, size_t nLevel, const wchar_t *szName)
{
	size_t nSlot = GetHighScoreSlot(nScore);

	if (nSlot != ff::INVALID_SIZE)
	{
		// Shift down existing scores

		for (size_t i = ff::PreviousSize(_countof(_highScores)); i > nSlot; i--)
		{
			CopyMemory(&_highScores[i], &_highScores[i - 1], sizeof(_highScores[i]));
		}

		// Set the new score

		HighScore hs;
		ff::ZeroObject(hs);

		_tcsncpy_s(hs._name, szName, _TRUNCATE);
		hs._level = (DWORD)nLevel;
		hs._score = (DWORD)nScore;
		
		SYSTEMTIME st;
		GetSystemTime(&st);
		SystemTimeToFileTime(&st, &hs._time);

		_highScores[nSlot] = hs;
	}
}

static ff::StaticString s_scores(L"Scores");

// static
void Stats::Load()
{
	ff::Dict dict = ff::MetroGlobals::Get()->GetState(s_scores);

	for (ff::StringRef key : dict.GetAllNames())
	{
		ff::ComPtr<ff::IData> data = dict.GetData(key);

		Stats stats;
		if (data && data->GetSize() <= sizeof(stats))
		{
			CopyMemory(&stats, data->GetMem(), data->GetSize());
			s_stats.SetKey(key, stats);
		}
	}
}

// static
void Stats::Save()
{
	ff::Dict dict = ff::MetroGlobals::Get()->GetState(s_scores);

	for (ff::BucketIter iter = s_stats.StartIteration(); iter != ff::INVALID_ITER; iter = s_stats.Iterate(iter))
	{
		const Stats &stats = s_stats.ValueAt(iter);
		ff::String key = s_stats.KeyAt(iter);

		ff::ComPtr<ff::IDataVector> data;
		if (ff::CreateDataVector(0, &data))
		{
			data->GetVector().Push((const BYTE *)&stats, sizeof(stats));
			dict.SetData(key, data);
		}
	}

	ff::MetroGlobals::Get()->SetState(s_scores, dict);
}

// static
Stats &Stats::Get(ff::StringRef mazesId)
{
	ff::BucketIter iter = s_stats.Get(mazesId);

	if (iter == ff::INVALID_ITER)
	{
		iter = s_stats.SetKey(mazesId, Stats());
	}

	return s_stats.ValueAt(iter);
}
