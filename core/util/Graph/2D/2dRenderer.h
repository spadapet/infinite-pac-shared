#pragma once

#include "Graph/GraphDeviceChild.h"

namespace ff
{
	class I2dEffect;
	class IRenderDepth;
	class IRenderTarget;
	class ISprite;
	enum MatrixType;

	enum MatrixType
	{
		MATRIX_WORLD,
		MATRIX_VIEW,
		MATRIX_PROJECTION,
		MATRIX_COUNT
	};

	class __declspec(uuid("02ab96f5-252c-427b-b257-91ee52802c64")) __declspec(novtable)
		I2dRenderer : public IGraphDeviceChild
	{
	public:
		virtual bool BeginRender(
			IRenderTarget *pTarget,
			IRenderDepth *pDepth,
			RectFloat viewRect,
			PointFloat worldTopLeft,
			PointFloat worldScale,
			I2dEffect *pEffect) = 0;

		virtual void Flush() = 0;
		virtual void EndRender(bool resetDepth = true) = 0;
		virtual I2dEffect *GetEffect() = 0;
		virtual IRenderTarget *GetTarget() = 0;
		virtual bool PushEffect(I2dEffect *pEffect) = 0;
		virtual void PopEffect() = 0;
		virtual void NudgeDepth() = 0;
		virtual void ResetDepth() = 0;
		virtual const RectFloat &GetRenderViewRect() const = 0;
		virtual const RectFloat &GetRenderWorldRect() const = 0;
		virtual PointFloat GetZBounds() const = 0;
		virtual bool ForceOpaqueUntilFlush(bool forceOpaque) = 0;

		virtual void DrawSprite(ISprite *pSprite, const PointFloat *pPos, const PointFloat *pScale, const float rotate, const DirectX::XMFLOAT4 *pColor) = 0;
		virtual void DrawMultiSprite(ISprite **ppSprite, size_t nSpriteCount, const PointFloat *pPos, const PointFloat *pScale, const float rotate,const DirectX::XMFLOAT4 *pColor, size_t nColorCount) = 0;
		virtual void DrawPolyLine(const PointFloat *pPoints, size_t nCount, const DirectX::XMFLOAT4 *pColor) = 0;
		virtual void DrawClosedPolyLine(const PointFloat *pPoints, size_t nCount, const DirectX::XMFLOAT4 *pColor) = 0;
		virtual void DrawLine(const PointFloat *pStart, const PointFloat *pEnd, const DirectX::XMFLOAT4 *pColor) = 0;
		virtual void DrawRectangle(const RectFloat *pRect, const DirectX::XMFLOAT4 *pColor) = 0;
		virtual void DrawRectangle(const RectFloat *pRect, float thickness, const DirectX::XMFLOAT4 *pColor) = 0;
		virtual void DrawFilledRectangle(const RectFloat *pRect, const DirectX::XMFLOAT4 *pColor, size_t nColorCount) = 0;
		virtual void DrawFilledTriangle(const PointFloat *pPoints, const DirectX::XMFLOAT4 *pColor, size_t nColorCount) = 0;

		virtual const DirectX::XMFLOAT4X4 &GetMatrix(MatrixType type) = 0;
		virtual void SetMatrix(MatrixType type, const DirectX::XMFLOAT4X4 *pMatrix) = 0;
		virtual void TransformMatrix(MatrixType type, const DirectX::XMFLOAT4X4 *pMatrix) = 0;
		virtual void PushMatrix(MatrixType type) = 0;
		virtual void PopMatrix(MatrixType type) = 0;
	};

	UTIL_API bool Create2dRenderer(IGraphDevice *pDevice, I2dRenderer **ppRender);
}
