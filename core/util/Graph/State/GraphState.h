#pragma once

namespace ff
{
	class IGraphDevice;
	enum VertexType;

	struct GraphState
	{
		UTIL_API GraphState();
		UTIL_API bool Apply(IGraphDevice *device, ID3D11Buffer *vertexes = nullptr, ID3D11Buffer *indexes = nullptr) const;

		VertexType _vertexType;
		D3D_PRIMITIVE_TOPOLOGY _topology;

		UINT _stride;
		UINT _stencil;
		UINT _sampleMask;
		float _blendFactors[4];

		ComPtr<ID3D11BlendState> _blend;
		Vector<ComPtr<ID3D11Buffer>> _vertexConstants;
		Vector<ComPtr<ID3D11Buffer>> _pixelConstants;
		ComPtr<ID3D11DepthStencilState> _depth;
		ComPtr<ID3D11InputLayout> _layout;
		ComPtr<ID3D11PixelShader> _pixel;
		ComPtr<ID3D11RasterizerState> _raster;
		Vector<ComPtr<ID3D11SamplerState>> _samplers;
		ComPtr<ID3D11VertexShader> _vertex;
	};
}
