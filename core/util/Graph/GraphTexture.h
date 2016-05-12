#pragma once

#include "Graph/GraphDeviceChild.h"

namespace DirectX
{
	class ScratchImage;
}

namespace ff
{
	class ISprite;

	class __declspec(uuid("8d9fab28-83b4-4327-8bf1-87b75eb9235e")) __declspec(novtable)
		IGraphTexture : public IGraphDeviceChild
	{
	public:
		virtual PointInt GetSize() = 0;
		virtual ID3D11Texture2D *GetTexture() = 0;
		virtual ID3D11ShaderResourceView *GetShaderResource() = 0;
		virtual bool Convert(DXGI_FORMAT format, size_t nMipMapLevels, IGraphTexture **ppNewTexture) = 0;
	};

	UTIL_API DXGI_FORMAT ParseTextureFormat(StringRef szFormat);

	UTIL_API bool CreateGraphTexture(
		IGraphDevice *pDevice,
		StringRef path,
		DXGI_FORMAT format,
		size_t nMipMapLevels, // 0=all, 1, 2, 3...
		IGraphTexture **ppTexture);

#if !METRO_APP
	UTIL_API bool CreateGraphTexture(
		IGraphDevice *pDevice,
		HBITMAP bitmap,
		DXGI_FORMAT format,
		size_t nMipMapLevels, // 0=all, 1, 2, 3...
		IGraphTexture **ppTexture);
#endif

	UTIL_API bool CreateGraphTexture(
		IGraphDevice *pDevice,
		PointInt size,
		DXGI_FORMAT format,
		size_t nMipMapLevels, // 0=all, 1, 2, 3...
		size_t nArraySize,
		size_t nMultiSamples,
		IGraphTexture **ppTexture);

	UTIL_API bool CreateGraphTexture(
		ISprite *sprite,
		IGraphTexture **texture);

	UTIL_API bool CreateStagingTexture(
		IGraphDevice *pDevice,
		PointInt size,
		DXGI_FORMAT format,
		bool bReadable,
		bool bWritable,
		IGraphTexture **ppTexture);

	// uses all mipmaps and all array entries
	UTIL_API bool CreateGraphTexture(
		IGraphDevice *pDevice,
		ID3D11Texture2D *pData,
		IGraphTexture **ppTexture);

	UTIL_API bool CreateGraphTexture(
		IGraphDevice *pDevice,
		ID3D11Texture2D *pData,
		size_t nMipMapStart,
		size_t nMipMapCount, // 0=all
		size_t nArrayStart,
		size_t nArrayCount, // 0=all
		IGraphTexture **ppTexture);

#if !METRO_APP
	UTIL_API std::shared_ptr<DirectX::ScratchImage> CreateScratchImage(HBITMAP bitmap);
#endif
}
