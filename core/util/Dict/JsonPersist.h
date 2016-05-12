#pragma once

#include "Dict/Dict.h"

namespace ff
{
	UTIL_API Dict JsonParse(StringRef text, size_t *errorPos = nullptr);
	UTIL_API String JsonWrite(const Dict &dict);
}
