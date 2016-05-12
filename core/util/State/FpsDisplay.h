#pragma once

#include "Resource/ResourceValue.h"
#include "State/State.h"

namespace ff
{
	class IInputMapping;
	class ISpriteFont;

	class FpsDisplay : public ff::State
	{
	public:
		UTIL_API FpsDisplay();
		UTIL_API virtual ~FpsDisplay();

		virtual std::shared_ptr<State> Advance(AppGlobals *context) override;
		virtual void Render(AppGlobals *context, IRenderTarget *target) override;
		virtual Status GetStatus() override;

	private:
		void RenderIntro(AppGlobals *context, IRenderTarget *target);
		void RenderNumbers(AppGlobals *context, IRenderTarget *target);
		void RenderCharts(AppGlobals *context, IRenderTarget *target);

		static const size_t MAX_QUEUE_SIZE = 60 * 6;

		bool _enabledNumbers;
		bool _enabledCharts;
		ff::TypedResource<ff::ISpriteFont> _font;
		ff::TypedResource<ff::IInputMapping> _input;

		size_t _totalAdvanceCount;
		size_t _totalRenderCount;
		size_t _advanceCount;
		size_t _apsCounter;
		size_t _rpsCounter;
		double _lastAps;
		double _lastRps;
		double _totalSeconds;
		double _oldSeconds;
		double _advanceTimeTotal;
		double _advanceTimeAverage;
		double _renderTime;
		double _flipTime;
		double _bankTime;
		double _bankPercent;

		struct FrameInfo
		{
			float advanceTime;
			float renderTime;
			float totalTime;
		};

		std::list<FrameInfo> _frames;
	};
}
