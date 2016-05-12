#pragma once

namespace ff
{
	// All of your component classes must derive from this.
	struct Component
	{
	protected:
		UTIL_API static size_t GetNextFactoryIndex();
	};
}
