#pragma once

namespace ff
{
	class IGraphDevice;

	class GraphStateCache
	{
	public:
		GraphStateCache();
		~GraphStateCache();

		void SetDevice(IGraphDevice *pDevice);
		void Reset();

		UTIL_API ID3D11BlendState *GetBlendState(const D3D11_BLEND_DESC &desc);
		UTIL_API ID3D11DepthStencilState *GetDepthStencilState(const D3D11_DEPTH_STENCIL_DESC &desc);
		UTIL_API ID3D11RasterizerState *GetRasterizerState(const D3D11_RASTERIZER_DESC &desc);
		UTIL_API ID3D11SamplerState *GetSamplerState(const D3D11_SAMPLER_DESC &desc);

		UTIL_API void GetDefault(D3D11_BLEND_DESC &desc);
		UTIL_API void GetDefault(D3D11_DEPTH_STENCIL_DESC &desc);
		UTIL_API void GetDefault(D3D11_RASTERIZER_DESC &desc);
		UTIL_API void GetDefault(D3D11_SAMPLER_DESC &desc);

		UTIL_API void GetAlphaBlend(D3D11_RENDER_TARGET_BLEND_DESC &desc);

	private:
		IGraphDevice *_device;

		Map<D3D11_BLEND_DESC, ComPtr<ID3D11BlendState>> _blendStates;
		Map<D3D11_DEPTH_STENCIL_DESC, ComPtr<ID3D11DepthStencilState>> _depthStates;
		Map<D3D11_RASTERIZER_DESC, ComPtr<ID3D11RasterizerState>> _rasterStates;
		Map<D3D11_SAMPLER_DESC, ComPtr<ID3D11SamplerState>> _samplerStates;
	};
}
