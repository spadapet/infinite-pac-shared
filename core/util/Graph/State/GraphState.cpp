#include "pch.h"
#include "Graph/GraphDevice.h"
#include "Graph/State/GraphState.h"
#include "Graph/VertexFormat.h"

ff::GraphState::GraphState()
	: _vertexType(ff::VERTEX_TYPE_UNKNOWN)
	, _topology(D3D_PRIMITIVE_TOPOLOGY_UNDEFINED)
	, _stride(0)
	, _stencil(0)
	, _sampleMask(0)
{
	ZeroObject(_blendFactors);
}

bool ff::GraphState::Apply(IGraphDevice *device, ID3D11Buffer *vertexes, ID3D11Buffer *indexes) const
{
	assertRetVal(device && device->GetContext(), false);
	ID3D11DeviceContextX *context = device->GetContext();

	// Input state

	if (indexes)
	{
		context->IASetIndexBuffer(indexes, DXGI_FORMAT_R16_UINT, 0);
	}

	if (vertexes)
	{
		UINT offset = 0;
		context->IASetVertexBuffers(0, 1, &vertexes, &_stride, &offset);
	}

	if (_layout)
	{
		context->IASetPrimitiveTopology(_topology);
		context->IASetInputLayout(_layout);
	}

	// Vertex stage

	if (!_vertexConstants.IsEmpty())
	{
		context->VSSetConstantBuffers(0, (UINT)_vertexConstants.Size(), _vertexConstants[0].Address());
	}

	if (_vertex)
	{
		context->VSSetShader(_vertex, nullptr, 0);
	}

	// Raster stage

	if (_raster)
	{
		context->RSSetState(_raster);
	}

	// Pixel stage

	if (!_pixelConstants.IsEmpty())
	{
		context->PSSetConstantBuffers(0, (UINT)_pixelConstants.Size(), _pixelConstants[0].Address());
	}

	if (_pixel)
	{
		context->PSSetShader(_pixel, nullptr, 0);
	}

	if (!_samplers.IsEmpty())
	{
		context->PSSetSamplers(0, (UINT)_samplers.Size(), _samplers[0].Address());
	}

	// Output stage

	if (_depth)
	{
		context->OMSetDepthStencilState(_depth, _stencil);
	}

	if (_blend)
	{
		context->OMSetBlendState(_blend, _blendFactors, _sampleMask);
	}

	return true;
}
