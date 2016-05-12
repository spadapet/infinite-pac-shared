#pragma once

#include "Data/Data.h"

namespace ff
{
	class __declspec(uuid("125774c8-eb02-45bf-9e45-0f9094e83aec")) __declspec(novtable)
		IFontData : public IData
	{
	public:
		virtual bool SetFile(StringRef path) = 0;
		virtual bool SetData(IData *pData) = 0;
	};

	UTIL_API bool CreateFontData(IFontData **ppData);
}
