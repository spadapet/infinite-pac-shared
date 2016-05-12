#include "pch.h"
#include "Globals/AppGlobals.h"
#include "Globals/MetroGlobals.h"
#include "Graph/Anim/AnimPos.h"
#include "Graph/2D/2dRenderer.h"
#include "Graph/Font/SpriteFont.h"
#include "Graph/RenderTarget/RenderTarget.h"
#include "State/FpsDisplay.h"
#include "Input/InputMapping.h"
#include "Input/KeyboardDevice.h"
#include "Resource/ResourceValue.h"
#include "Types/Timer.h"

static ff::hash_t EVENT_TOGGLE_NUMBERS = ff::HashFunc(L"toggleNumbers");
static ff::hash_t EVENT_TOGGLE_CHARTS = ff::HashFunc(L"toggleCharts");

ff::FpsDisplay::FpsDisplay()
	: _enabledNumbers(false)
	, _enabledCharts(false)
	, _totalSeconds(0)
	, _oldSeconds(0)
	, _totalAdvanceCount(0)
	, _totalRenderCount(0)
	, _advanceCount(0)
	, _apsCounter(0)
	, _rpsCounter(0)
	, _lastAps(0)
	, _lastRps(0)
	, _advanceTimeTotal(0)
	, _advanceTimeAverage(0)
	, _renderTime(0)
	, _flipTime(0)
	, _bankTime(0)
	, _bankPercent(0)
	, _font(L"SmallMonoFont")
{
}

ff::FpsDisplay::~FpsDisplay()
{
}

std::shared_ptr<ff::State> ff::FpsDisplay::Advance(AppGlobals *globals)
{
	_apsCounter++;

	if (!_input.DidInit())
	{
		_input.SetFilter([globals](ff::ComPtr<ff::IInputMapping> &obj)
		{
			ff::IInputDevice *keys = globals->GetKeys();

			ff::ComPtr<ff::IInputMapping> newObj;
			if (obj->Clone(&keys, 1, &newObj))
			{
				obj = newObj;
			}
		});

		_input.Init(L"FpsDisplayInput");
	}

	ff::IInputMapping *input = _input.GetObject();

	if (input)
	{
		input->Advance(globals->GetGlobalTime()._secondsPerAdvance);

		if (input->HasStartEvent(EVENT_TOGGLE_CHARTS))
		{
			if (_enabledCharts)
			{
				_enabledCharts = false;
				_enabledNumbers = false;
			}
			else
			{
				_enabledCharts = true;
				_enabledNumbers = true;
			}
		}
		else if (input->HasStartEvent(EVENT_TOGGLE_NUMBERS))
		{
			if (_enabledNumbers)
			{
				_enabledNumbers = false;
				_enabledCharts = false;
			}
			else
			{
				_enabledNumbers = true;
			}
		}
	}

	if (!_enabledNumbers && !_enabledCharts)
	{
		return nullptr;
	}

	const ff::FrameTime &ft = globals->GetFrameTime();
	const ff::GlobalTime &gt = globals->GetGlobalTime();
	bool updateFastNumbers = (gt._advanceCount % 8) == 0;

	INT64 advanceTimeTotalInt = 0;
	for (size_t i = 0; i < ft._advanceCount; i++)
	{
		advanceTimeTotalInt += ft._advanceTime[i];
	}

	_totalAdvanceCount = gt._advanceCount;
	_totalRenderCount = gt._renderCount;
	_totalSeconds = gt._absoluteSeconds;

	if (floor(_oldSeconds) != floor(_totalSeconds))
	{
		_lastAps = _apsCounter / (_totalSeconds - _oldSeconds);
		_lastRps = _rpsCounter / (_totalSeconds - _oldSeconds);
		_oldSeconds = _totalSeconds;
		_apsCounter = 0;
		_rpsCounter = 0;
	}

	if (updateFastNumbers)
	{
		_advanceCount = ft._advanceCount;
		_advanceTimeTotal = advanceTimeTotalInt / ft._freqD;
		_advanceTimeAverage = _advanceTimeTotal / ft._advanceCount;
		_renderTime = ft._renderTime / ft._freqD;
		_flipTime = ft._flipTime / ft._freqD;
		_bankTime = gt._bankSeconds;
		_bankPercent = _bankTime / gt._secondsPerAdvance;
	}

	FrameInfo frameInfo;
	frameInfo.advanceTime = (float)(advanceTimeTotalInt / ft._freqD);
	frameInfo.renderTime = (float)(ft._renderTime / ft._freqD);
	frameInfo.totalTime = (float)((advanceTimeTotalInt + ft._renderTime + ft._flipTime) / ft._freqD);
	_frames.push_back(frameInfo);

	while (_frames.size() > MAX_QUEUE_SIZE)
	{
		_frames.erase(_frames.begin());
	}

	return nullptr;
}

void ff::FpsDisplay::Render(AppGlobals *globals, IRenderTarget *target)
{
	_rpsCounter++;

	if (_enabledNumbers || _enabledCharts)
	{
		RenderIntro(globals, target);
	}

	if (_enabledNumbers)
	{
		RenderNumbers(globals, target);
	}

	if (_enabledCharts)
	{
		RenderCharts(globals, target);
	}
}

ff::State::Status ff::FpsDisplay::GetStatus()
{
	return State::Status::Ignore;
}

void ff::FpsDisplay::RenderIntro(AppGlobals *globals, IRenderTarget *target)
{
	ff::ISpriteFont *font = _font.GetObject();
	noAssertRet(font);

	ff::I2dRenderer *render = globals->Get2dRender();
	ff::PointFloat targetSize = target->GetRotatedSize().ToFloat();
	ff::RectFloat targetRect(targetSize);

	if (render->BeginRender(
		target,
		nullptr,
		targetRect,
		ff::PointFloat::Zeros(),
		ff::PointDouble(target->GetDpiScale(), target->GetDpiScale()).ToFloat(),
		globals->Get2dEffect()))
	{
		ff::StaticString buffer(L"Press <F8> to close stats.\n");
		font->DrawText(render, buffer,
			ff::PointFloat(8, 8),
			ff::PointFloat(1, 1),
			ff::PointFloat(0, 0),
			&ff::GetColorWhite());

		render->EndRender();
	}
}

void ff::FpsDisplay::RenderNumbers(AppGlobals *globals, IRenderTarget *target)
{
	ff::ISpriteFont *font = _font.GetObject();
	noAssertRet(font);

	ff::I2dRenderer *render = globals->Get2dRender();
	ff::PointFloat targetSize = target->GetRotatedSize().ToFloat();
	ff::RectFloat targetRect(targetSize);

	if (render->BeginRender(
		target,
		nullptr,
		targetRect,
		ff::PointFloat::Zeros(),
		ff::PointDouble(target->GetDpiScale(), target->GetDpiScale()).ToFloat(),
		globals->Get2dEffect()))
	{
		wchar_t buffer[1024];
		size_t bufferLen = (size_t)_sntprintf_s(
			buffer, _countof(buffer),
			L"\nTime:%.2fs, Updates:%Iu, Renders:%Iu\n\n\n\n"
			L"Present:%.2fms\n"
			L"Total:%.2fms\n",
			_totalSeconds, _totalAdvanceCount, _totalRenderCount,
			_flipTime * 1000.0,
			(_advanceTimeTotal + _renderTime + _flipTime) * 1000.0);

		font->DrawText(render,
			ff::StaticString(buffer, bufferLen),
			ff::PointFloat(8, 8),
			ff::PointFloat(1, 1),
			ff::PointFloat(0, 0),
			&ff::GetColorWhite());

		bufferLen = (size_t)_sntprintf_s(
			buffer, _countof(buffer),
			L"\n\n\nUpdate:%.2fms*%Iu/%.fHz",
			_advanceTimeAverage * 1000.0, _advanceCount, _lastAps);

		font->DrawText(render,
			ff::StaticString(buffer, bufferLen),
			ff::PointFloat(8, 8),
			ff::PointFloat(1, 1),
			ff::PointFloat(0, 0),
			&DirectX::XMFLOAT4(1, 0, 1, 1));

		bufferLen = (size_t)_sntprintf_s(
			buffer, _countof(buffer),
			L"\n\n\n\nRender:%.2fms/%.fHz",
			_renderTime * 1000.0, _lastRps);

		font->DrawText(render,
			ff::StaticString(buffer, bufferLen),
			ff::PointFloat(8, 8),
			ff::PointFloat(1, 1),
			ff::PointFloat(0, 0),
			&DirectX::XMFLOAT4(0, 1, 0, 1));

		render->EndRender();
	}
}

void ff::FpsDisplay::RenderCharts(AppGlobals *globals, IRenderTarget *target)
{
	ff::I2dRenderer *render = globals->Get2dRender();
	ff::PointFloat targetSize = target->GetRotatedSize().ToFloat();
	ff::RectFloat targetRect(targetSize);

	const float viewFps = 60;
	const float viewSeconds = MAX_QUEUE_SIZE / viewFps;
	const float scale = 16;
	const float viewFpsInverse = 1 / viewFps;

	if (targetSize.x > 0 &&
		targetSize.y > 0 &&
		render->BeginRender(
			target,
			nullptr,
			targetRect,
			ff::PointFloat(-viewSeconds, 1),
			targetSize / ff::PointFloat(viewSeconds, -1),
			globals->Get2dEffect()))
	{
		ff::PointFloat advancePoints[MAX_QUEUE_SIZE];
		ff::PointFloat renderPoints[MAX_QUEUE_SIZE];
		ff::PointFloat totalPoints[MAX_QUEUE_SIZE];

		render->DrawLine(
			&ff::PointFloat(0, viewFpsInverse * scale),
			&ff::PointFloat(-viewSeconds, viewFpsInverse * scale),
			&DirectX::XMFLOAT4(1, 1, 0, 1));

		if (_frames.size() > 1)
		{
			size_t timeIndex = 0;
			for (auto i = _frames.rbegin(); i != _frames.rend(); i++, timeIndex++)
			{
				float x = (float)(timeIndex * -viewFpsInverse);
				advancePoints[timeIndex].SetPoint(x, i->advanceTime * scale);
				renderPoints[timeIndex].SetPoint(x, (i->renderTime + i->advanceTime) * scale);
				totalPoints[timeIndex].SetPoint(x, i->totalTime * scale);
			}

			render->DrawPolyLine(advancePoints, _frames.size(), &DirectX::XMFLOAT4(1, 0, 1, 1));
			render->DrawPolyLine(renderPoints, _frames.size(), &DirectX::XMFLOAT4(0, 1, 0, 1));
			render->DrawPolyLine(totalPoints, _frames.size(), &DirectX::XMFLOAT4(1, 1, 1, 1));
		}

		render->EndRender();
	}
}
