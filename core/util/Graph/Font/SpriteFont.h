#pragma once

#include "Graph/GraphDeviceChild.h"

namespace ff
{
	class I2dEffect;
	class I2dRenderer;
	class ISprite;
	class IFontData;

	class __declspec(uuid("b4356d2d-1b85-400c-b3d6-cbff44352305")) __declspec(novtable)
		ISpriteFont : public IGraphDeviceChild
	{
	public:
		virtual PointFloat DrawText(
			I2dRenderer *pRender,
			StringRef text,
			PointFloat pos,
			PointFloat scale,
			PointFloat spacing,
			const DirectX::XMFLOAT4 *pColor) = 0;

		virtual PointFloat MeasureText(StringRef text, PointFloat scale, PointFloat spacing) = 0;
	};

	UTIL_API bool CreateSpriteFont(IGraphDevice *pDevice, const LOGFONT &logFont, ISpriteFont **ppFont);
	UTIL_API bool CreateSpriteFont(IGraphDevice *pDevice, IFontData *pData, const LOGFONT &logFont, ISpriteFont **ppFont);

	UTIL_API bool CreateSpriteFromText(
		I2dRenderer *pRender,
		I2dEffect *pEffect,
		ISpriteFont *pFont,
		StringRef text,
		ISprite **ppSprite);

	UTIL_API bool CreateSpriteFromText(
		I2dRenderer *pRender,
		I2dEffect *pEffect,
		ISpriteFont *pFont,
		StringRef text,
		PointInt size,
		PointFloat scale,
		PointFloat spacing,
		PointFloat offset,
		bool bCenter,
		ISprite **ppSprite);
}
