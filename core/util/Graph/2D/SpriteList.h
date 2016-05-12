#pragma once

#include "Graph/GraphDeviceChild.h"

namespace ff
{
	class IGraphTexture;
	class ISprite;
	enum class SpriteType;

	// The source of sprites - they must all use the same texture
	class __declspec(uuid("9b4114d1-b515-4cfa-bafe-c27113cd4e75")) __declspec(novtable)
		ISpriteList : public IGraphDeviceChild
	{
	public:
		virtual ISprite *Add(
			IGraphTexture *pTexture,
			StringRef name,
			RectFloat rect,
			PointFloat handle,
			PointFloat scale,
			SpriteType type) = 0;
		virtual ISprite *Add(ISprite *pSprite) = 0;
		virtual bool Add(ISpriteList *pList) = 0;

		virtual size_t GetCount() = 0;
		virtual ISprite *Get(size_t nSprite) = 0;
		virtual ISprite *Get(StringRef name) = 0;
		virtual StringRef GetName(size_t nSprite) = 0;
		virtual size_t GetIndex(StringRef name) = 0;
		virtual bool Remove(ISprite *pSprite) = 0;
		virtual bool Remove(size_t nSprite) = 0;
	};

	UTIL_API bool CreateSpriteList(IGraphDevice *pDevice, ISpriteList **ppList);
}
