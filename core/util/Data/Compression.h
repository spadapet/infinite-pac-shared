#pragma once

namespace ff
{
	class IData;
	class IDataReader;
	class IDataWriter;
	class IChunkListener;

	UTIL_API bool CompressData(const BYTE *pFullData, size_t nFullSize, IData **ppCompData, IChunkListener *pListener = nullptr);
	UTIL_API bool UncompressData(const BYTE *pCompData, size_t nCompSize, size_t nFullSize, IData **ppFullData, IChunkListener *pListener = nullptr);

	UTIL_API bool CompressData(IData *pData, IData **ppCompData, IChunkListener *pListener = nullptr);
	UTIL_API bool UncompressData(IData *pData, size_t nFullSize, IData **ppFullData, IChunkListener *pListener = nullptr);

	UTIL_API bool CompressData(IDataReader *pInput, size_t nFullSize, IDataWriter *pOutput, IChunkListener *pListener = nullptr);
	UTIL_API bool UncompressData(IDataReader *pInput, size_t nCompSize, IDataWriter *pOutput, IChunkListener *pListener = nullptr);

	class IChunkListener
	{
	public:
		virtual bool OnChunk(size_t nChunkSize, size_t nAllChunks, size_t nFullSize) = 0;
		virtual void OnChunkSuccess(size_t nFullSize) = 0;
		virtual void OnChunkFailure(size_t nAllChunks, size_t nFullSize) = 0;
	};
}
