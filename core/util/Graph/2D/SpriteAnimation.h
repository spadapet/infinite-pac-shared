#pragma once

#include "Graph/GraphDeviceChild.h"

namespace ff
{
	class I2dRenderer;
	class ISprite;
	enum AnimTweenType;

	class __declspec(uuid("289b6bef-1714-40b1-88c6-2e1f4bce4e96")) __declspec(novtable)
		ISpriteAnimation : public IGraphDeviceChild
	{
	public:
		virtual void Clear() = 0;

		virtual void Render(
			I2dRenderer *pRender,
			AnimTweenType type,
			float frame,
			PointFloat pos,
			const PointFloat *pScale,
			float rotate,
			const DirectX::XMFLOAT4 *pColor) = 0;

		virtual void SetSprite(float frame, size_t nPart, size_t nSprite, ISprite *pSprite) = 0;
		virtual void SetColor(float frame, size_t nPart, size_t nSprite, const DirectX::XMFLOAT4 &color) = 0;
		virtual void SetOffset(float frame, size_t nPart, PointFloat offset) = 0;
		virtual void SetScale(float frame, size_t nPart, PointFloat scale) = 0;
		virtual void SetRotate(float frame, size_t nPart, float rotate) = 0;
		virtual void SetHitBox(float frame, size_t nPart, RectFloat hitBox) = 0;
		virtual RectFloat GetHitBox(float frame, size_t nPart, AnimTweenType type) = 0;

		virtual void SetLastFrame(float frame) = 0;
		virtual float GetLastFrame() const = 0;

		virtual void SetFPS(float fps) = 0;
		virtual float GetFPS() const = 0;
	};

	UTIL_API bool CreateSpriteAnimation(IGraphDevice *pDevice, ISpriteAnimation **ppAnim);
}
