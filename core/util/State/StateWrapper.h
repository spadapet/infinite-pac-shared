#pragma once

#include "State/State.h"

namespace ff
{
	// Automatically updates the state object based on requests from its Advance() call
	class StateWrapper : public ff::State
	{
	public:
		UTIL_API StateWrapper();
		UTIL_API StateWrapper(std::shared_ptr<State> state);
		virtual ~StateWrapper();

		UTIL_API StateWrapper &operator=(const std::shared_ptr<State> &state);

		virtual std::shared_ptr<State> Advance(AppGlobals *globals) override;
		virtual void Render(AppGlobals *globals, IRenderTarget *target) override;
		virtual void SaveState(AppGlobals *globals) override;
		virtual void LoadState(AppGlobals *globals) override;
		virtual bool Notify(hash_t eventId, int data1, void *data2) override;
		virtual Status GetStatus() override;

	private:
		void SetState(AppGlobals *globals, const std::shared_ptr<State> &state);
		void CheckState();

		std::shared_ptr<ff::State> _state;
	};
}
