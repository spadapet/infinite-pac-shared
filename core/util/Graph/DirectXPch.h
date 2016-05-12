#pragma once

// DirectX includes
// Precompiled header only

#ifdef DIRECTINPUT_VERSION
#undef DIRECTINPUT_VERSION
#endif
#define DIRECTINPUT_VERSION 0x0800

#include <DInput.h>
#undef COM_NO_WINDOWS_H

#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <XAudio2.h>
#include <XInput.h>

#if METRO_APP
#include <dxgi1_4.h>
#include <d3d11_3.h>
#include <d2d1_3.h>
#define ID2D1DeviceX ID2D1Device2
#define ID2D1DeviceContextX ID2D1DeviceContext2
#define ID2D1FactoryX ID2D1Factory3
#define IDXGIAdapterX IDXGIAdapter3
#define IDXGIDeviceX IDXGIDevice3
#define IDXGIFactoryX IDXGIFactory4
#define IDXGIOutputX IDXGIOutput4
#define IDXGIResourceX IDXGIResource1
#define IDXGISurfaceX IDXGISurface2
#define IDXGISwapChainX IDXGISwapChain3
#define ID3D11DeviceX ID3D11Device3
#define ID3D11DeviceContextX ID3D11DeviceContext3
#define ID3D11BlendStateX ID3D11BlendState1
#define ID3D11RasterizerStateX ID3D11RasterizerState2
#else
#include <d3d11.h>
#include <d2d1.h>
#define ID2D1DeviceX IUnknown
#define ID2D1FactoryX ID2D1Factory
#define IDXGIAdapterX IDXGIAdapter1
#define IDXGIDeviceX IDXGIDevice1
#define IDXGIFactoryX IDXGIFactory1
#define IDXGIOutputX IDXGIOutput
#define IDXGIResourceX IDXGIResource
#define IDXGISurfaceX IDXGISurface1
#define IDXGISwapChainX IDXGISwapChain
#define ID3D11DeviceX ID3D11Device
#define ID3D11DeviceContextX ID3D11DeviceContext
#define ID3D11BlendStateX ID3D11BlendState
#define ID3D11RasterizerStateX ID3D11RasterizerState
#endif

inline bool operator<(const D3D11_BLEND_DESC &lhs, const D3D11_BLEND_DESC &rhs)
{
	return memcmp(&lhs, &rhs, sizeof(lhs)) < 0;
}

inline bool operator<(const D3D11_DEPTH_STENCIL_DESC &lhs, const D3D11_DEPTH_STENCIL_DESC &rhs)
{
	return memcmp(&lhs, &rhs, sizeof(lhs)) < 0;
}

inline bool operator<(const D3D11_RASTERIZER_DESC &lhs, const D3D11_RASTERIZER_DESC &rhs)
{
	return memcmp(&lhs, &rhs, sizeof(lhs)) < 0;
}

inline bool operator<(const D3D11_SAMPLER_DESC &lhs, const D3D11_SAMPLER_DESC &rhs)
{
	return memcmp(&lhs, &rhs, sizeof(lhs)) < 0;
}

inline bool operator==(DirectX::FXMVECTOR lhs, DirectX::FXMVECTOR rhs)
{
	UINT record;
	DirectX::XMVectorEqualR(&record, lhs, rhs);
	return DirectX::XMComparisonAllTrue(record);
}

inline bool operator!=(DirectX::FXMVECTOR lhs, DirectX::FXMVECTOR rhs)
{
	UINT record;
	DirectX::XMVectorEqualR(&record, lhs, rhs);
	return DirectX::XMComparisonAnyFalse(record);
}

inline bool operator==(const DirectX::XMFLOAT4 &lhs, const DirectX::XMFLOAT4 &rhs)
{
	return DirectX::XMLoadFloat4(&lhs) == DirectX::XMLoadFloat4(&rhs);
}

inline bool operator!=(const DirectX::XMFLOAT4 &lhs, const DirectX::XMFLOAT4 &rhs)
{
	return DirectX::XMLoadFloat4(&lhs) != DirectX::XMLoadFloat4(&rhs);
}

MAKE_POD(DirectX::XMFLOAT2);
MAKE_POD(DirectX::XMFLOAT2A);
MAKE_POD(DirectX::XMFLOAT3);
MAKE_POD(DirectX::XMFLOAT3A);
MAKE_POD(DirectX::XMFLOAT4);
MAKE_POD(DirectX::XMFLOAT4A);
MAKE_POD(DirectX::XMFLOAT3X3);
MAKE_POD(DirectX::XMFLOAT4X3);
MAKE_POD(DirectX::XMFLOAT4X3A);
MAKE_POD(DirectX::XMFLOAT4X4);
MAKE_POD(DirectX::XMFLOAT4X4A);
MAKE_POD(DirectX::XMINT2);
MAKE_POD(DirectX::XMINT3);
MAKE_POD(DirectX::XMINT4);
MAKE_POD(DirectX::XMUINT2);
MAKE_POD(DirectX::XMUINT3);
MAKE_POD(DirectX::XMUINT4);
MAKE_POD(DirectX::XMMATRIX);
MAKE_POD(DirectX::XMVECTORF32);
MAKE_POD(DirectX::XMVECTORI32);
MAKE_POD(DirectX::XMVECTORU32);
MAKE_POD(DirectX::XMVECTORU8);
