#include "pch.h"
#include "Types/Timer.h"

ff::GlobalTime::GlobalTime()
{
	ff::ZeroObject(*this);
	_advancesPerSecond = 60;
	_secondsPerAdvance = 1 / _advancesPerSecond;
}

ff::FrameTime::FrameTime()
{
	ff::ZeroObject(*this);

	ff::Timer timer;
	_freq = timer.GetRawFreq();
	_freqD = timer.GetRawFreqD();
}

ff::Timer::Timer()
	: _numTicks(0)
	, _tpsSecond(0)
	, _tpsCurSecond(0)
	, _tpsCount(0)
	, _tps(0)
	, _timeScale(1)
	, _startSeconds(0)
	, _seconds(0)
	, _freqDouble(0)
	, _passSec(0)
{
	QueryPerformanceFrequency((LARGE_INTEGER *)&_freq);

	_startTime = GetCurrentRawTime();
	_curTime = _startTime;
	_storedTime = _startTime;
	_freqDouble = (double)_freq;
}

ff::Timer::~Timer()
{
}

double ff::Timer::Tick()
{
	double oldTime = _seconds;

	_curTime = GetCurrentRawTime();
	_seconds = _startSeconds + (_timeScale * (_curTime - _startTime) / _freqDouble);
	_passSec = _seconds - oldTime;

	if (_passSec < 0)
	{
		// something weird happened (sleep mode? processor switch?)
		Reset();
	}

	// ticks per second stuff
	_numTicks++;
	_tpsCount++;
	_tpsCurSecond = (size_t)_seconds;

	if (_tpsCurSecond > _tpsSecond)
	{
		_tps = _tpsCount / (_tpsCurSecond - _tpsSecond);
		_tpsCount = 0;
		_tpsSecond = _tpsCurSecond;
	}

	return _passSec;
}

void ff::Timer::Reset()
{
	_startTime = GetCurrentRawTime();
	_curTime = _startTime;
	_storedTime = _startTime;
	_numTicks = 0;
	_tpsSecond = 0;
	_tpsCount = 0;
	_tpsCurSecond = 0;
	_tps = 0;
	_startSeconds = 0;
	_seconds = 0;
	_passSec = 0;

	// _timeScale stays the same
}

double ff::Timer::GetSeconds() const
{
	return _seconds;
}

double ff::Timer::GetTickSeconds() const
{
	return _passSec;
}

size_t ff::Timer::GetNumTicks() const
{
	return _numTicks;
}

size_t ff::Timer::GetTicksPerSecond() const
{
	return _tps;
}

double ff::Timer::GetTimeScale() const
{
	return _timeScale;
}

void ff::Timer::SetTimeScale(double scale)
{
	if (scale != _timeScale)
	{
		_timeScale = scale;
		_startSeconds = _seconds;
		_startTime = _curTime;
	}
}

INT64 ff::Timer::GetLastTickRawTime() const
{
	return _curTime;
}

INT64 ff::Timer::GetCurrentRawTime() const
{
	INT64 curTime;
	QueryPerformanceCounter((LARGE_INTEGER *)&curTime);
	return curTime;
}

INT64 ff::Timer::GetRawFreq() const
{
	return _freq;
}

double ff::Timer::GetRawFreqD() const
{
	return _freqDouble;
}

void ff::Timer::StoreLastTickTime()
{
	_storedTime = _curTime;
}

INT64 ff::Timer::GetLastTickStoredRawTime()
{
	INT64 diff = _curTime - _storedTime;
	_storedTime = _curTime;

	return diff;
}

INT64 ff::Timer::GetCurrentStoredRawTime()
{
	INT64 curTime = GetCurrentRawTime();
	INT64 diff = curTime - _storedTime;
	_storedTime = curTime;

	return diff;
}
