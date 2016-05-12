#include "pch.h"
#include "Graph/GraphDevice.h"
#include "Graph/State/GraphStateCache.h"

ff::GraphStateCache::GraphStateCache()
{
}

ff::GraphStateCache::~GraphStateCache()
{
}

void ff::GraphStateCache::SetDevice(IGraphDevice *pDevice)
{
	_device = pDevice;
}

void ff::GraphStateCache::Reset()
{
	_blendStates.Clear();
	_depthStates.Clear();
	_rasterStates.Clear();
	_samplerStates.Clear();
}

ID3D11BlendState *ff::GraphStateCache::GetBlendState(const D3D11_BLEND_DESC &desc)
{
	BucketIter i = _blendStates.Get(desc);

	if (i == INVALID_ITER)
	{
		ComPtr<ID3D11BlendState> pState;
		assertHrRetVal(_device->Get3d()->CreateBlendState(&desc, &pState), nullptr);

		i = _blendStates.Insert(desc, pState);
	}

	return _blendStates.ValueAt(i);
}

ID3D11DepthStencilState *ff::GraphStateCache::GetDepthStencilState(const D3D11_DEPTH_STENCIL_DESC &desc)
{
	BucketIter i = _depthStates.Get(desc);

	if (i == INVALID_ITER)
	{
		ComPtr<ID3D11DepthStencilState> pState;
		assertHrRetVal(_device->Get3d()->CreateDepthStencilState(&desc, &pState), nullptr);

		i = _depthStates.Insert(desc, pState);
	}

	return _depthStates.ValueAt(i);
}

ID3D11RasterizerState *ff::GraphStateCache::GetRasterizerState(const D3D11_RASTERIZER_DESC &desc)
{
	BucketIter i = _rasterStates.Get(desc);

	if (i == INVALID_ITER)
	{
		ComPtr<ID3D11RasterizerState> pState;
		assertHrRetVal(_device->Get3d()->CreateRasterizerState(&desc, &pState), nullptr);

		i = _rasterStates.Insert(desc, pState);
	}

	return _rasterStates.ValueAt(i);
}

ID3D11SamplerState *ff::GraphStateCache::GetSamplerState(const D3D11_SAMPLER_DESC &desc)
{
	BucketIter i = _samplerStates.Get(desc);

	if (i == INVALID_ITER)
	{
		ComPtr<ID3D11SamplerState> pState;
		assertHrRetVal(_device->Get3d()->CreateSamplerState(&desc, &pState), nullptr);

		i = _samplerStates.Insert(desc, pState);
	}

	return _samplerStates.ValueAt(i);
}

void ff::GraphStateCache::GetDefault(D3D11_BLEND_DESC &desc)
{
	ZeroObject(desc);

	for (size_t i = 0; i < _countof(desc.RenderTarget); i++)
	{
		desc.RenderTarget[i].SrcBlend = D3D11_BLEND_ONE;
		desc.RenderTarget[i].DestBlend = D3D11_BLEND_ZERO;
		desc.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
		desc.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
		desc.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_ZERO;
		desc.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		desc.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	}
}

void ff::GraphStateCache::GetDefault(D3D11_DEPTH_STENCIL_DESC &desc)
{
	ZeroObject(desc);

	desc.DepthEnable = TRUE;
	desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	desc.DepthFunc = D3D11_COMPARISON_LESS;
	desc.StencilEnable = FALSE;
	desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

	desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
}

void ff::GraphStateCache::GetDefault(D3D11_RASTERIZER_DESC &desc)
{
	ZeroObject(desc);

	desc.FillMode = D3D11_FILL_SOLID;
	desc.CullMode = D3D11_CULL_BACK;
	desc.DepthClipEnable = TRUE;
}

void ff::GraphStateCache::GetDefault(D3D11_SAMPLER_DESC &desc)
{
	ZeroObject(desc);

	desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	desc.MinLOD = -FLT_MAX;
	desc.MaxLOD = FLT_MAX;
	desc.MaxAnisotropy = 16;
	desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
}

void ff::GraphStateCache::GetAlphaBlend(D3D11_RENDER_TARGET_BLEND_DESC &desc)
{
	desc.BlendEnable = TRUE;
	desc.SrcBlend = D3D11_BLEND_SRC_ALPHA;
	desc.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	desc.BlendOp = D3D11_BLEND_OP_ADD;
	desc.SrcBlendAlpha = D3D11_BLEND_ZERO;
	desc.DestBlendAlpha = D3D11_BLEND_ONE;
	desc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
}
