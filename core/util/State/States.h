#pragma once

#include "State/State.h"

namespace ff
{
	// Deals with a list of State objects that all advance and render together
	class States : public ff::State
	{
	public:
		UTIL_API States();
		virtual ~States();

		UTIL_API void AddTop(std::shared_ptr<State> state);
		UTIL_API void AddBottom(std::shared_ptr<State> state);

		virtual std::shared_ptr<State> Advance(AppGlobals *globals) override;
		virtual void Render(AppGlobals *globals, IRenderTarget *target) override;
		virtual void SaveState(AppGlobals *globals) override;
		virtual void LoadState(AppGlobals *globals) override;
		virtual bool Notify(hash_t eventId, int data1, void *data2) override;
		virtual Status GetStatus() override;

	private:
		std::list<std::shared_ptr<State>> _states;
	};
}
