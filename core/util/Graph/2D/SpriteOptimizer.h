#pragma once

namespace ff
{
	class ISpriteList;

	UTIL_API bool OptimizeSprites(
		ISpriteList *pSprites,
		DXGI_FORMAT format,
		size_t nMipMapLevels,
		ISpriteList **ppOptimizedSprites);
}
