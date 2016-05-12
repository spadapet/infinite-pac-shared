#pragma once

namespace ff
{
	class IGraphDevice;

	enum VertexType
	{
		VERTEX_TYPE_UNKNOWN,
		VERTEX_TYPE_LINE_ART,
		VERTEX_TYPE_SPRITE,
		VERTEX_TYPE_MULTI_SPRITE,
	};

	struct LineArtVertex
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT4 color;
	};

	struct SpriteVertex
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT4 color;
		DirectX::XMFLOAT2 tex;
		UINT ntex;
	};

	struct MultiSpriteVertex
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT4 color0;
		DirectX::XMFLOAT4 color1;
		DirectX::XMFLOAT4 color2;
		DirectX::XMFLOAT4 color3;
		DirectX::XMFLOAT2 tex0;
		DirectX::XMFLOAT2 tex1;
		DirectX::XMFLOAT2 tex2;
		DirectX::XMFLOAT2 tex3;
		UINT ntex[4];
	};

	typedef Vector<LineArtVertex> LineArtVertexes;
	typedef Vector<SpriteVertex> SpriteVertexes;
	typedef Vector<MultiSpriteVertex> MultiSpriteVertexes;

	UTIL_API size_t GetVertexStride(VertexType type);

	bool CreateVertexLayout(
		IGraphDevice *pDevice,
		const void *pShaderBytes,
		size_t nShaderSize,
		VertexType type,
		ID3D11InputLayout **ppLayout);
}

MAKE_POD(ff::LineArtVertex);
MAKE_POD(ff::SpriteVertex);
MAKE_POD(ff::MultiSpriteVertex);
