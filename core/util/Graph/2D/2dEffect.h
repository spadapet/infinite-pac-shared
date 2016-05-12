#pragma once

#include "Graph/GraphDeviceChild.h"

namespace ff
{
	class I2dRenderer;
	class IGraphTexture;
	enum MatrixType;

	enum DrawType2d
	{
		// Choose one
		DRAW_TYPE_SPRITE = 0x000000,
		DRAW_TYPE_MULTI_SPRITE = 0x000001,
		DRAW_TYPE_TRIANGLES = 0x000002,
		DRAW_TYPE_LINES = 0x000003,
		DRAW_TYPE_POINTS = 0x000004,
		DRAW_TYPE_BITS = 0x00000F,

		// Choose one
		DRAW_DEPTH_ENABLE = 0x000010, // same as not being set
		DRAW_DEPTH_DISABLE = 0x000020,
		DRAW_DEPTH_BITS = 0x0000F0,

		// Choose one
		DRAW_STENCIL_NONE = 0x000100, // same as not being set
		DRAW_STENCIL_WRITE = 0x000200,
		DRAW_STENCIL_IS_SET = 0x000300,
		DRAW_STENCIL_NOT_SET = 0x000400,
		DRAW_STENCIL_BITS = 0x000F00,

		// Choose multiple
		DRAW_SAMPLE_MIN_POINT = 0x000000, // default (no bits)
		DRAW_SAMPLE_MAG_POINT = 0x000000, // default (no bits)
		DRAW_SAMPLE_MIN_LINEAR = 0x001000,
		DRAW_SAMPLE_MAG_LINEAR = 0x002000,
		DRAW_SAMPLE_WRAP = 0x004000,
		DRAW_SAMPLE_BITS = 0x00F000,

		// Choose one
		DRAW_BLEND_ALPHA = 0x010000, // same as not being set
		DRAW_BLEND_ALPHA_MAX = 0x020000, // blend color and copy max alpha
		DRAW_BLEND_ADD = 0x030000, // color and alpha
		DRAW_BLEND_SUB = 0x040000, // color and alpha
		DRAW_BLEND_MULTIPLY = 0x050000, // color and alpha
		DRAW_BLEND_INV_MUL = 0x060000, // inverse color and alpha
		DRAW_BLEND_COPY_ALPHA = 0x070000, // only copy alpha
		DRAW_BLEND_COPY_ALL = 0x080000, // direct copy of all bits
		DRAW_BLEND_COPY_PMA = 0x090000, // direct copy of all bits after multiplying colors by alpha
		DRAW_BLEND_BITS = 0x0F0000,

		// Choose multiple
		DRAW_TEXTURE_RGBA = 0x000000, // default (no bits)
		DRAW_TEXTURE_RGB = 0x100000,
		DRAW_TEXTURE_ALPHA = 0x200000,
		DRAW_TEXTURE_BITS = 0xF00000,

		DRAW_FORCE_DWORD = 0x7fffffff,
	};

	class __declspec(uuid("b76a3740-9411-4968-abbe-1c572013d73d")) __declspec(novtable)
		I2dEffect : public IGraphDeviceChild
	{
	public:
		virtual bool IsValid() = 0;

		virtual bool OnBeginRender(I2dRenderer *pRender) = 0;
		virtual void OnEndRender(I2dRenderer *pRender) = 0;

		virtual void OnMatrixChanging(I2dRenderer *pRender, MatrixType type) = 0;
		virtual void OnMatrixChanged(I2dRenderer *pRender, MatrixType type) = 0;

		virtual void ApplyTextures(I2dRenderer *pRender, IGraphTexture **ppTextures, size_t nTextures) = 0;
		virtual bool Apply(I2dRenderer *pRender, DrawType2d type, ID3D11Buffer *pVertexes, ID3D11Buffer *pIndexes, float zOffset) = 0;

		virtual void PushDrawType(DrawType2d typeFlags) = 0;
		virtual void PopDrawType() = 0;
	};

	UTIL_API bool CreateDefault2dEffect(IGraphDevice *pDevice, I2dEffect **ppEffect);
}
