#include "pch.h"
#include "Data/Compression.h"
#include "Data/Data.h"
#include "Data/DataFile.h"
#include "Data/DataWriterReader.h"

#include <zlib.h>

static size_t GetChunkSizeForDataSize(size_t nDataSize)
{
	static const size_t s_nMaxChunkSize = 1024 * 256;

	return std::min<size_t>(nDataSize, s_nMaxChunkSize);
}

bool ff::CompressData(const BYTE *pFullData, size_t nFullSize, IData **ppCompData, IChunkListener *pListener)
{
	assertRetVal(pFullData && ppCompData, false);

	ComPtr<IData> pData;
	assertRetVal(CreateDataInStaticMem(pFullData, nFullSize, &pData), false);

	return CompressData(pData, ppCompData, pListener);
}

bool ff::UncompressData(const BYTE *pCompData, size_t nCompSize, size_t nFullSize, IData **ppFullData, IChunkListener *pListener)
{
	assertRetVal(pCompData && ppFullData, false);

	ComPtr<IData> pData;
	assertRetVal(CreateDataInStaticMem(pCompData, nCompSize, &pData), false);

	return UncompressData(pData, nFullSize, ppFullData, pListener);
}

bool ff::CompressData(IData *pData, IData **ppCompData, IChunkListener *pListener)
{
	assertRetVal(pData && ppCompData, false);

	ComPtr<IDataReader> pReader;
	assertRetVal(CreateDataReader(pData, 0, &pReader), false);

	ComPtr<IDataVector> pCompData;
	ComPtr<IDataWriter> pWriter;
	assertRetVal(CreateDataWriter(&pCompData, &pWriter), false);

	assertRetVal(CompressData(pReader, pData->GetSize(), pWriter, pListener), false);

	*ppCompData = pCompData.Detach();
	return true;
}

bool ff::UncompressData(IData *pData, size_t nFullSize, IData **ppFullData, IChunkListener *pListener)
{
	assertRetVal(pData && ppFullData, false);

	ComPtr<IDataReader> pReader;
	assertRetVal(CreateDataReader(pData, 0, &pReader), false);

	ComPtr<IDataVector> pFullData;
	ComPtr<IDataWriter> pWriter;
	assertRetVal(CreateDataWriter(&pFullData, &pWriter), false);

	assertRetVal(UncompressData(pReader, pData->GetSize(), pWriter, pListener), false);
	assertRetVal(nFullSize == INVALID_SIZE || nFullSize == pFullData->GetSize(), false);

	*ppFullData = pFullData.Detach();
	return true;
}

bool ff::CompressData(IDataReader *pInput, size_t nFullSize, IDataWriter *pOutput, IChunkListener *pListener)
{
	assertRetVal(pInput && pOutput, false);

	// Init zlib's buffer
	z_stream zlibData;
	ZeroObject(zlibData);
	deflateInit(&zlibData, Z_BEST_COMPRESSION);

	// Init my output buffer
	Vector<BYTE> outputChunk;
	outputChunk.Resize(GetChunkSizeForDataSize(nFullSize));

	bool bStatus = true;
	size_t nProgress = 0;

	for (size_t nPos = 0, nInputChunkSize = GetChunkSizeForDataSize(nFullSize);
		bStatus && nPos < nFullSize; nPos += nInputChunkSize)
	{
		// Read a chunk of input and get ready to pass it to zlib

		size_t nRead = std::min(nFullSize - nPos, nInputChunkSize);
		const BYTE *pChunk = pInput->Read(nRead);
		bool bLastChunk = nPos + nInputChunkSize >= nFullSize;

		if (pChunk)
		{
			zlibData.avail_in = (uInt)nRead;
			zlibData.next_in = (Bytef*)pChunk;

			do
			{
				zlibData.avail_out = (uInt)outputChunk.Size();
				zlibData.next_out = outputChunk.Data();
				deflate(&zlibData, bLastChunk ? Z_FINISH : Z_NO_FLUSH);

				size_t nWrite = outputChunk.Size() - zlibData.avail_out;
				if (nWrite)
				{
					bStatus = pOutput->Write(outputChunk.Data(), nWrite);

					if (bStatus && pListener)
					{
						nProgress += nRead;

						if (!pListener->OnChunk(nRead, nProgress, nFullSize))
						{
							bStatus = false;
							break;
						}
					}
				}
			}
			while (bStatus && !zlibData.avail_out);
			assert(!zlibData.avail_in);
		}
		else
		{
			bStatus = false;
		}
	}

	deflateEnd(&zlibData);

	if (pListener)
	{
		if (bStatus)
		{	
			assert(nProgress == nFullSize);
			pListener->OnChunkSuccess(nFullSize);
		}
		else
		{
			pListener->OnChunkFailure(nProgress, nFullSize);
		}
	}

	assert(bStatus);
	return bStatus;
}

bool ff::UncompressData(IDataReader *pInput, size_t nCompSize, IDataWriter *pOutput, IChunkListener *pListener)
{
	assertRetVal(pInput && pOutput, false);

	if (!nCompSize)
	{
		return true;
	}

	// Init zlib's buffer
	z_stream zlibData;
	ZeroObject(zlibData);
	inflateInit(&zlibData);

	// Init my output buffer
	Vector<BYTE> outputChunk;
	outputChunk.Resize(GetChunkSizeForDataSize(nCompSize * 2));

	bool bStatus = true;
	int nInflateStatus = Z_OK;
	size_t nProgress = 0;
	size_t nPos = 0;

	for (size_t nInputChunkSize = GetChunkSizeForDataSize(nCompSize);
		bStatus && nPos < nCompSize; nPos = std::min(nPos + nInputChunkSize, nCompSize))
	{
		// Read a chunk of input and get ready to pass it to zlib

		size_t nRead = std::min(nCompSize - nPos, nInputChunkSize);
		const BYTE *pChunk = pInput->Read(nRead);

		if (pChunk)
		{
			zlibData.avail_in = (uInt)nRead;
			zlibData.next_in = (Bytef *)pChunk;

			do
			{
				zlibData.avail_out = (uInt)outputChunk.Size();
				zlibData.next_out = outputChunk.Data();
				nInflateStatus = inflate(&zlibData, Z_NO_FLUSH);

				bStatus =
					nInflateStatus != Z_NEED_DICT &&
					nInflateStatus != Z_DATA_ERROR &&
					nInflateStatus != Z_MEM_ERROR;

				size_t nWrite = outputChunk.Size() - zlibData.avail_out;

				if (nWrite)
				{
					bStatus = pOutput->Write(outputChunk.Data(), nWrite);

					if (pListener && bStatus)
					{
						nProgress += nRead;

						if (!pListener->OnChunk(nRead, nProgress, nCompSize))
						{
							bStatus = false;
							break;
						}
					}
				}
			}
			while (bStatus && !zlibData.avail_out);
			assert(!zlibData.avail_in);
		}
		else
		{
			bStatus = false;
		}
	}

	bStatus = (nPos == nCompSize && nInflateStatus == Z_STREAM_END);

	inflateEnd(&zlibData);

	if (pListener)
	{
		if (bStatus)
		{	
			assert(nProgress == nCompSize);
			pListener->OnChunkSuccess(nCompSize);
		}
		else
		{
			pListener->OnChunkFailure(nProgress, nCompSize);
		}
	}

	assert(bStatus);
	return bStatus;
}
