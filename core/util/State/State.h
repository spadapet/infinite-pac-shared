#pragma once

namespace ff
{
	class AppGlobals;
	class IRenderTarget;

	// A State will advance 60hz and render up to 60fps
	class State : public std::enable_shared_from_this<State>
	{
	public:
		UTIL_API State();
		UTIL_API virtual ~State();

		UTIL_API virtual std::shared_ptr<State> Advance(AppGlobals *globals);
		UTIL_API virtual void Render(AppGlobals *globals, IRenderTarget *target);
		UTIL_API virtual void SaveState(AppGlobals *globals);
		UTIL_API virtual void LoadState(AppGlobals *globals);
		UTIL_API virtual bool Notify(hash_t eventId, int data1, void *data2);

		enum class Status
		{
			Loading,
			Alive,
			Dead,
			Ignore,
		};

		UTIL_API virtual Status GetStatus();
	};
}
