#include "pch.h"
#include "Graph/GraphDevice.h"
#include "Graph/VertexFormat.h"

// Must match the LineArtVertex structure
static D3D11_INPUT_ELEMENT_DESC s_layoutLineArtVertex[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

// Must match the SpriteVertex structure
static D3D11_INPUT_ELEMENT_DESC s_layoutSpriteVertex[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXINDEX", 0, DXGI_FORMAT_R32_UINT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

// Must match the MultiSpriteVertex structure
static D3D11_INPUT_ELEMENT_DESC s_layoutMultiSpriteVertex[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 60, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 76, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 84, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 2, DXGI_FORMAT_R32G32_FLOAT, 0, 92, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 3, DXGI_FORMAT_R32G32_FLOAT, 0, 100, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXINDEX", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, 108, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

static const D3D11_INPUT_ELEMENT_DESC *GetVertexDescription(ff::VertexType type)
{
	switch (type)
	{
		default: assert(false); return nullptr;
		case ff::VERTEX_TYPE_LINE_ART: return s_layoutLineArtVertex;
		case ff::VERTEX_TYPE_SPRITE: return s_layoutSpriteVertex;
		case ff::VERTEX_TYPE_MULTI_SPRITE: return s_layoutMultiSpriteVertex;
	}
}

static size_t GetVertexDescriptionCount(ff::VertexType type)
{
	switch (type)
	{
		default: assert(false); return 0;
		case ff::VERTEX_TYPE_LINE_ART: return _countof(s_layoutLineArtVertex);
		case ff::VERTEX_TYPE_SPRITE: return _countof(s_layoutSpriteVertex);
		case ff::VERTEX_TYPE_MULTI_SPRITE: return _countof(s_layoutMultiSpriteVertex);
	}
}

size_t ff::GetVertexStride(ff::VertexType type)
{
	switch (type)
	{
		default: assert(false); return 0;
		case VERTEX_TYPE_LINE_ART: return sizeof(LineArtVertex);
		case VERTEX_TYPE_SPRITE: return sizeof(SpriteVertex);
		case VERTEX_TYPE_MULTI_SPRITE: return sizeof(MultiSpriteVertex);
	}
}

bool ff::CreateVertexLayout(
	IGraphDevice *pDevice,
	const void *pShaderBytes,
	size_t nShaderSize,
	VertexType type,
	ID3D11InputLayout **ppLayout)
{
	assertRetVal(pDevice && pDevice->Get3d() && ppLayout, false);
	assertRetVal(pShaderBytes && nShaderSize, false);

	ComPtr<ID3D11InputLayout> pLayout;
	assertHrRetVal(pDevice->Get3d()->CreateInputLayout(
		GetVertexDescription(type),
		(UINT)GetVertexDescriptionCount(type),
		pShaderBytes,
		nShaderSize,
		&pLayout), false);

	*ppLayout = pLayout.Detach();
	return true;
}
