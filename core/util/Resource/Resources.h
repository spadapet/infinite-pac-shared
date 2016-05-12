#pragma once

#include "Dict/Dict.h"
#include "Resource/ResourceValue.h"

namespace ff
{
	class AppGlobals;

	class __declspec(uuid("1894118b-40da-4055-8ffe-e28688a61a3f")) __declspec(novtable)
		IResources : public IUnknown
	{
	public:
		virtual void SetResources(const Dict &dict) = 0;
		virtual Dict GetResources() const = 0;
		virtual void Clear() = 0;
		virtual bool IsLoading() const = 0;

		virtual SharedResourceValue GetResource(StringRef name) = 0;
		virtual SharedResourceValue FlushResource(SharedResourceValue value) = 0;
		virtual AppGlobals *GetContext() const = 0;
	};

	UTIL_API bool CreateResources(AppGlobals *globals, const Dict &dict, IResources **obj);
}
