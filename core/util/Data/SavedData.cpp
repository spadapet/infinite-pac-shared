#include "pch.h"
#include "COM/ComObject.h"
#include "Data/Compression.h"
#include "Data/Data.h"
#include "Data/DataFile.h"
#include "Data/DataWriterReader.h"
#include "Data/SavedData.h"

namespace ff
{
	class __declspec(uuid("98514703-8655-49f8-8c2e-13641af6dd9c"))
		CSavedData : public ComBase, public ISavedData
	{
	public:
		DECLARE_HEADER(CSavedData);

		bool CreateLoadedDataFromMemory(IData *pData, bool bCompress);
		bool CreateSavedDataFromMemory(IData *pData, size_t nFullSize, bool bCompressed);

		bool CreateSavedDataFromFile(
			IDataFile *pFile,
			size_t nStart,
			size_t nSavedSize,
			size_t nFullSize,
			bool bCompressed);

		// ISavedData functions

		virtual IData *Load() override;
		virtual bool Unload() override;
		virtual IData *SaveToMem() override;
		virtual bool SaveToFile() override;

		virtual size_t GetSavedSize() override;
		virtual size_t GetFullSize() override;
		virtual bool IsCompressed() override;
		virtual bool CreateSavedDataReader(IDataReader **ppReader) override;

		virtual bool Clone(ISavedData **ppSavedData) override;
		virtual bool Copy(ISavedData *pDataSource) override;

	private:
		ComPtr<IData> _fullData;
		ComPtr<IData> _origData;
		size_t _origDataFullSize;
		bool _compress;

		ComPtr<IDataFile> _origFile;
		size_t _origFileStart;
		size_t _origFileSize;
		size_t _origFileFullSize;
	};
}

BEGIN_INTERFACES(ff::CSavedData)
	HAS_INTERFACE(ff::ISavedData)
END_INTERFACES()

bool ff::CreateLoadedDataFromMemory(IData *pData, bool bCompress, ISavedData **ppSavedData)
{
	assertRetVal(ppSavedData, false);
	*ppSavedData = nullptr;

	ComPtr<CSavedData, ISavedData> pSavedData = new ComObject<CSavedData>;
	assertRetVal(pSavedData->CreateLoadedDataFromMemory(pData, bCompress), false);

	*ppSavedData = pSavedData.Detach();

	return true;
}

bool ff::CreateSavedDataFromMemory(IData *pData, size_t nFullSize, bool bCompressed, ISavedData **ppSavedData)
{
	assertRetVal(ppSavedData, false);
	*ppSavedData = nullptr;

	ComPtr<CSavedData, ISavedData> pSavedData = new ComObject<CSavedData>;
	if (bCompressed)
	{
		assertRetVal(pSavedData->CreateSavedDataFromMemory(pData, nFullSize, bCompressed), false);
	}
	else
	{
		assertRetVal(nFullSize == pData->GetSize(), false);
		assertRetVal(pSavedData->CreateLoadedDataFromMemory(pData, false), false);
	}

	*ppSavedData = pSavedData.Detach();

	return true;
}

bool ff::CreateSavedDataFromFile(
		IDataFile *pFile,
		size_t nStart,
		size_t nSavedSize,
		size_t nFullSize,
		bool bCompressed,
		ISavedData **ppSavedData)
{
	assertRetVal(ppSavedData, false);
	*ppSavedData = nullptr;

	if (pFile && pFile->GetMem())
	{
		ComPtr<IData> pData;

		if (!CreateDataInMemMappedFile(pFile->GetMem() + nStart, nSavedSize, pFile, &pData) ||
			!CreateSavedDataFromMemory(pData, nFullSize, bCompressed, ppSavedData))
		{
			assertRetVal(false, false);
		}
	}
	else
	{
		ComPtr<CSavedData, ISavedData> pSavedData = new ComObject<CSavedData>;

		assertRetVal(pSavedData->CreateSavedDataFromFile(
			pFile, nStart, nSavedSize, nFullSize, bCompressed), false);

		*ppSavedData = pSavedData.Detach();
	}

	return true;
}

ff::CSavedData::CSavedData()
	: _compress(false)
	, _origDataFullSize(0)
	, _origFileStart(0)
	, _origFileSize(0)
	, _origFileFullSize(0)
{
}

ff::CSavedData::~CSavedData()
{
}

bool ff::CSavedData::CreateLoadedDataFromMemory(IData *pData, bool bCompress)
{
	assertRetVal(pData, false);

	_fullData = pData;
	_compress = bCompress;

	if (!bCompress)
	{
		_origData = pData;
		_origDataFullSize = pData->GetSize();
	}

	return true;
}

bool ff::CSavedData::CreateSavedDataFromMemory(IData *pData, size_t nFullSize, bool bCompressed)
{
	assertRetVal(pData, false);

	if (!bCompressed)
	{
		assertRetVal(nFullSize == pData->GetSize(), false);
		return CreateLoadedDataFromMemory(pData, bCompressed);
	}
	else
	{
		_origData = pData;
		_origDataFullSize = nFullSize;
		_compress = bCompressed;
	}

	return true;
}

bool ff::CSavedData::CreateSavedDataFromFile(
		IDataFile *pFile,
		size_t nStart,
		size_t nSavedSize,
		size_t nFullSize,
		bool bCompressed)
{
	assertRetVal(pFile, false);
	assertSz(!pFile->GetMem(), L"Can't use a mem mapped file in CSavedData::CreateSavedDataFromFile");

	_origFile = pFile;
	_origFileStart = nStart;
	_origFileSize = nSavedSize;
	_origFileFullSize = nFullSize;
	_compress = bCompressed;

	return true;
}

ff::IData *ff::CSavedData::Load()
{
	if (!_fullData && !_compress && _origData)
	{
		_fullData = _origData;

		if (!_origData->IsStatic())
		{
			_origData = nullptr;
		}
	}

	if (!_fullData)
	{
		size_t nSavedSize = GetSavedSize();
		size_t nFullSize = GetFullSize();
		bool bSuccess = false;

		ComPtr<IDataReader> pReader;
		ComPtr<IDataWriter> pWriter;
		ComPtr<IDataVector> pDataVector;

		if (CreateDataVector(nFullSize, &pDataVector) && CreateDataWriter(pDataVector, 0, &pWriter))
		{
			if (_origFile)
			{
				bSuccess = CreateDataReader(_origFile, _origFileStart, &pReader);
			}
			else if (_origData)
			{
				bSuccess = CreateDataReader(_origData, 0, &pReader);
			}
		}

		if (bSuccess)
		{
			bSuccess = _compress
				? UncompressData(pReader, nSavedSize, pWriter)
				: StreamCopyData(pReader, nSavedSize, pWriter);

			assert(pWriter->GetPos() == nFullSize && pDataVector->GetVector().Size() == nFullSize);

			bSuccess = bSuccess && pWriter->GetPos() == nFullSize;
		}

		if (bSuccess)
		{
			_fullData = pDataVector;

			if (_origData && !_origData->IsStatic())
			{
				_origData = nullptr;
			}
		}
	}

	assert(_fullData);
	return _fullData;
}

bool ff::CSavedData::Unload()
{
	if (_origFile)
	{
		// still have the original file data, get rid of anything in memory

		if (_origData && !_origData->IsStatic())
		{
			_origData = nullptr;
		}
	}
	else if (_origData)
	{
		// still have the original saved data
	}
	else if (_fullData)
	{
		if (_compress)
		{
			CompressData(_fullData, &_origData);
		}
		else
		{
			_origData = _fullData;
			
		}
		
		_origDataFullSize = _fullData->GetSize();
	}

	_fullData = nullptr;

	return true;
}

ff::IData *ff::CSavedData::SaveToMem()
{
	if (!_origData)
	{
		if (_fullData)
		{
			if (_compress)
			{
				CompressData(_fullData, &_origData);
			}
			else
			{
				_origData = _fullData;
			}
		
			_origDataFullSize = _fullData->GetSize();
		}
		else if (_origFile)
		{
			// load a chunk of the file into memory

			ComPtr<IDataReader> pReader;
			ComPtr<IDataWriter> pWriter;
			ComPtr<IDataVector> pDataVector;

			if (CreateDataVector(_origFileSize, &pDataVector) &&
				CreateDataWriter(pDataVector, 0, &pWriter) &&
				CreateDataReader(_origFile, _origFileStart, &pReader) &&
				StreamCopyData(pReader, _origFileSize, pWriter))
			{
				_origData = pDataVector;
				_origDataFullSize = _origFileFullSize;
			}
		}
	}

	assertRetVal(_origData, nullptr);

	_fullData = nullptr;

	return _origData;
}

bool ff::CSavedData::SaveToFile()
{
	if (!_origFile && (!_origData || !_origData->IsStatic()))
	{
		ComPtr<IDataFile> pFile;
		ComPtr<IDataWriter> pWriter;
		ComPtr<IDataReader> pReader;

		assertRetVal(CreateTempDataFile(&pFile), false);
		assertRetVal(CreateDataWriter(pFile, 0, &pWriter), false);

		if (_origData)
		{
			assertRetVal(CreateDataReader(_origData, 0, &pReader), false);
			assertRetVal(StreamCopyData(pReader, _origData->GetSize(), pWriter), false);

			_origFile = pFile;
			_origFileStart = 0;
			_origFileSize = _origData->GetSize();
			_origFileFullSize = _origDataFullSize;
			_origData = nullptr;
		}
		else if (_fullData)
		{
			assertRetVal(CreateDataReader(_fullData, 0, &pReader), false);

			if (_compress)
			{
				assertRetVal(CompressData(pReader, _fullData->GetSize(), pWriter), false);
			}
			else
			{
				assertRetVal(StreamCopyData(pReader, _fullData->GetSize(), pWriter), false);
			}

			_origFile = pFile;
			_origFileStart = 0;
			_origFileSize = pWriter->GetPos();
			_origFileFullSize = _fullData->GetSize();
			_origData = nullptr;
		}
	}

	assertRetVal(_origFile || _origData, false);

	_fullData = nullptr;

	return true;
}

size_t ff::CSavedData::GetSavedSize()
{
	if (_origData)
	{
		return _origData->GetSize();
	}
	else if (_origFile)
	{
		return _origFileSize;
	}
	else if (SaveToMem())
	{
		return SaveToMem()->GetSize();
	}

	assertRetVal(false, 0);
}

size_t ff::CSavedData::GetFullSize()
{
	if (_fullData)
	{
		return _fullData->GetSize();
	}
	else if (_origData)
	{
		return _origDataFullSize;
	}
	else if (_origFile)
	{
		return _origFileFullSize;
	}

	assertRetVal(false, 0);
}

bool ff::CSavedData::IsCompressed()
{
	return _compress;
}

bool ff::CSavedData::CreateSavedDataReader(IDataReader **ppReader)
{
	assertRetVal(ppReader, false);
	*ppReader = nullptr;

	if (!_origFile && !_origData)
	{
		assertRetVal(Unload(), false);
	}

	if (_origData)
	{
		assertRetVal(CreateDataReader(_origData, 0, ppReader), false);
	}
	else if (_origFile)
	{
		assertRetVal(CreateDataReader(_origFile, _origFileStart, ppReader), false);
	}

	assert(*ppReader);
	return *ppReader != nullptr;
}

bool ff::CSavedData::Clone(ISavedData **ppSavedData)
{
	assertRetVal(ppSavedData, false);
	*ppSavedData = nullptr;

	ComPtr<CSavedData, ISavedData> pSavedData = new ComObject<CSavedData>;
	assertRetVal(pSavedData->Copy(this), false);

	*ppSavedData = pSavedData.Detach();

	return true;
}

bool ff::CSavedData::Copy(ISavedData *pDataSource)
{
	ComPtr<CSavedData, ISavedData> pRealSource;
	assertRetVal(pRealSource.QueryFrom(pDataSource), false);

	_fullData = pRealSource->_fullData;
	_compress = pRealSource->_compress;

	_origData = pRealSource->_origData;
	_origDataFullSize = pRealSource->_origDataFullSize;

	_origFile = pRealSource->_origFile;
	_origFileStart = pRealSource->_origFileStart;
	_origFileSize = pRealSource->_origFileSize;
	_origFileFullSize = pRealSource->_origFileFullSize;

	return true;
}
