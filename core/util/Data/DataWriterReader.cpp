#include "pch.h"
#include "COM/ComObject.h"
#include "Data/Data.h"
#include "Data/DataFile.h"
#include "Data/DataWriterReader.h"
#include "Data/SavedData.h"
#include "Globals/ProcessGlobals.h"
#include "Thread/ThreadDispatch.h"
#include "Thread/ThreadUtil.h"
#include "Windows/FileUtil.h"

#if METRO_APP
#include <robuffer.h>
#endif

namespace ff
{
	class __declspec(uuid("d23231d2-1c6b-43ff-8a9b-9b7cc5d0b206"))
		FileDataWriter : public ComBase, public IDataWriter
	{
	public:
		DECLARE_HEADER(FileDataWriter);

		bool Init(IDataFile *pFile, size_t nPos);

		// IDataWriter functions
		virtual bool Write(LPCVOID pMem, size_t nBytes) override;

		// IDataStream functions
		virtual size_t GetSize() const override;
		virtual size_t GetPos() const override;
		virtual bool SetPos(size_t nPos) override;

		virtual bool CreateSavedData(
			size_t start,
			size_t savedSize,
			size_t fullSize,
			bool compressed,
			ISavedData **obj) override;

	protected:
		ComPtr<IDataFile> _file;
		File _fileHandle;
	};
}

BEGIN_INTERFACES(ff::FileDataWriter)
	HAS_INTERFACE(ff::IDataStream)
	HAS_INTERFACE(ff::IDataWriter)
END_INTERFACES()

namespace ff
{
	class __declspec(uuid("48c327ec-1c63-4f4a-a0b5-536630138f08"))
		VectorDataWriter : public ComBase, public IDataWriter
	{
	public:
		DECLARE_HEADER(VectorDataWriter);

		bool Init(IDataVector *pData, size_t nPos);

		// IDataWriter functions
		virtual bool Write(LPCVOID pMem, size_t nBytes) override;

		// IDataStream functions
		virtual size_t GetSize() const override;
		virtual size_t GetPos() const override;
		virtual bool SetPos(size_t nPos) override;

		virtual bool CreateSavedData(
			size_t start,
			size_t savedSize,
			size_t fullSize,
			bool compressed,
			ISavedData **obj) override;

	protected:
		ComPtr<IDataVector> _data;
		size_t _pos;
	};
}

BEGIN_INTERFACES(ff::VectorDataWriter)
	HAS_INTERFACE(ff::IDataStream)
	HAS_INTERFACE(ff::IDataWriter)
END_INTERFACES()

namespace ff
{
	class __declspec(uuid("a7a0da9e-7c9b-4008-91e7-4aa332d1ac0f"))
		FileDataReader : public ComBase, public IDataReader
	{
	public:
		DECLARE_HEADER(FileDataReader);

		bool Init(IDataFile *pFile, size_t nPos);

		// IDataReader functions
		virtual const BYTE *Read(size_t nBytes) override;
		virtual bool Read(size_t nBytes, IData **ppData) override;
		virtual bool Read(size_t nStart, size_t nBytes, IData **ppData) override;

		// IDataStream functions
		virtual size_t GetSize() const override;
		virtual size_t GetPos() const override;
		virtual bool SetPos(size_t nPos) override;

		virtual bool CreateSavedData(
			size_t start,
			size_t savedSize,
			size_t fullSize,
			bool compressed,
			ISavedData **obj) override;

	protected:
		ComPtr<IDataFile> _file;
		File _fileHandle;
		Vector<BYTE> _data;
	};
}

BEGIN_INTERFACES(ff::FileDataReader)
	HAS_INTERFACE(ff::IDataStream)
	HAS_INTERFACE(ff::IDataReader)
END_INTERFACES()

namespace ff
{
	class __declspec(uuid("13e696f3-ecbd-42a8-88ae-5c36552eda57"))
		CMemDataReader : public ComBase, public IDataReader
	{
	public:
		DECLARE_HEADER(CMemDataReader);

		bool Init(IData *pData, size_t nPos);

		// IDataReader functions
		virtual const BYTE *Read(size_t nBytes) override;
		virtual bool Read(size_t nBytes, IData **ppData) override;
		virtual bool Read(size_t nStart, size_t nBytes, IData **ppData) override;

		// IDataStream functions
		virtual size_t GetSize() const override;
		virtual size_t GetPos() const override;
		virtual bool SetPos(size_t nPos) override;

		virtual bool CreateSavedData(
			size_t start,
			size_t savedSize,
			size_t fullSize,
			bool compressed,
			ISavedData **obj) override;

	protected:
		ComPtr<IData> _data;
		size_t _pos;
	};
}

BEGIN_INTERFACES(ff::CMemDataReader)
	HAS_INTERFACE(ff::IDataStream)
	HAS_INTERFACE(ff::IDataReader)
END_INTERFACES()

#if METRO_APP

namespace ff
{
	class __declspec(uuid("e22b624f-d39d-4ed3-9b04-385e059877f7"))
		RandomAccessStreamReader : public ComBase, public IDataReader
	{
	public:
		DECLARE_HEADER(RandomAccessStreamReader);

		bool Init(Windows::Storage::Streams::IRandomAccessStream ^stream);
		Windows::Storage::Streams::IRandomAccessStream ^GetStream() const;

		// IDataReader functions
		virtual const BYTE *Read(size_t nBytes) override;
		virtual bool Read(size_t nBytes, IData **ppData) override;
		virtual bool Read(size_t nStart, size_t nBytes, IData **ppData) override;

		// IDataStream functions
		virtual size_t GetSize() const override;
		virtual size_t GetPos() const override;
		virtual bool SetPos(size_t nPos) override;

		virtual bool CreateSavedData(
			size_t start,
			size_t savedSize,
			size_t fullSize,
			bool compressed,
			ISavedData **obj) override;

	protected:
		Windows::Storage::Streams::IRandomAccessStream ^_stream;
		Windows::Storage::Streams::IBuffer ^_buffer;
		ff::ComPtr<Windows::Storage::Streams::IBufferByteAccess> _bufferBytes;
	};
}

BEGIN_INTERFACES(ff::RandomAccessStreamReader)
	HAS_INTERFACE(ff::IDataStream)
	HAS_INTERFACE(ff::IDataReader)
END_INTERFACES()

#endif // METRO_APP

bool ff::CreateDataWriter(IDataVector *pData, size_t nPos, IDataWriter **ppWriter)
{
	assertRetVal(ppWriter, false);
	*ppWriter = nullptr;

	ComPtr<VectorDataWriter> pWriter = new ComObject<VectorDataWriter>;
	assertRetVal(pWriter->Init(pData, nPos), false);

	*ppWriter = pWriter.Detach();

	return true;
}

bool ff::CreateDataWriter(IDataVector **ppData, IDataWriter **ppWriter)
{
	ComPtr<IDataVector> pDataVector;
	assertRetVal(CreateDataVector(0, &pDataVector), false);

	if (ppData)
	{
		*ppData = ff::GetAddRef<IDataVector>(pDataVector);
	}

	return CreateDataWriter(pDataVector, 0, ppWriter);
}

bool ff::CreateDataWriter(IDataFile *pFile, size_t nPos, IDataWriter **ppWriter)
{
	assertRetVal(ppWriter, false);
	*ppWriter = nullptr;

	ComPtr<FileDataWriter> pWriter = new ComObject<FileDataWriter>;
	assertRetVal(pWriter->Init(pFile, nPos), false);

	*ppWriter = pWriter.Detach();

	return true;
}

bool ff::CreateDataReader(IData *pData, size_t nPos, IDataReader **ppReader)
{
	assertRetVal(ppReader, false);
	*ppReader = nullptr;

	ComPtr<CMemDataReader> pReader = new ComObject<CMemDataReader>;
	assertRetVal(pReader->Init(pData, nPos), false);

	*ppReader = pReader.Detach();

	return true;
}

bool ff::CreateDataReader(const BYTE *pMem, size_t nLen, size_t nPos, IDataReader **ppReader)
{
	ComPtr<IData> pData;
	assertRetVal(CreateDataInStaticMem(pMem, nLen, &pData), false);

	return CreateDataReader(pData, nPos, ppReader);
}

bool ff::CreateDataReader(IDataFile *pFile, size_t nPos, IDataReader **ppReader)
{
	assertRetVal(ppReader, false);
	*ppReader = nullptr;

	if (pFile && pFile->GetMem())
	{
		ComPtr<IData> pData;
		assertRetVal(CreateDataInMemMappedFile(pFile->GetMem(), pFile->GetSize(), pFile, &pData), false);

		return CreateDataReader(pData, nPos, ppReader);
	}
	else
	{
		ComPtr<FileDataReader> pReader = new ComObject<FileDataReader>;
		assertRetVal(pReader->Init(pFile, nPos), false);

		*ppReader = pReader.Detach();

		return true;
	}
}

#if METRO_APP

bool ff::CreateDataReader(Windows::Storage::Streams::IRandomAccessStream ^stream, IDataReader **ppReader)
{
	assertRetVal(stream && ppReader, false);
	*ppReader = nullptr;

	ComPtr<RandomAccessStreamReader> pReader = new ComObject<RandomAccessStreamReader>;
	assertRetVal(pReader->Init(stream), false);

	*ppReader = pReader.Detach();
	return true;
}

Windows::Storage::Streams::IRandomAccessStream ^ff::GetRandomAccessStream(IDataReader *reader)
{
	ComPtr<RandomAccessStreamReader, IDataReader> randomReader;
	if (randomReader.QueryFrom(reader))
	{
		return randomReader->GetStream();
	}

	return nullptr;
}

#endif

bool ff::StreamCopyData(IDataReader *pReader, size_t nSize, IDataWriter *pWriter, size_t nChunkSize)
{
	assertRetVal(pReader && pWriter, false);

	nChunkSize = (nChunkSize == INVALID_SIZE) ? 1024 * 256 : nChunkSize;

	for (size_t nPos = 0; nPos < nSize; nPos += nChunkSize)
	{
		size_t nRead = std::min(nSize - nPos, nChunkSize);
		const BYTE *pChunk = pReader->Read(nRead);

		assertRetVal(pChunk, false);
		assertRetVal(pWriter->Write(pChunk, nRead), false);
	}

	return true;
}

ff::FileDataWriter::FileDataWriter()
{
}

ff::FileDataWriter::~FileDataWriter()
{
}

bool ff::FileDataWriter::Init(IDataFile *pFile, size_t nPos)
{
	assertRetVal(pFile, false);

	_file = pFile;
	assertRetVal(_file->OpenWrite(_fileHandle, nPos != INVALID_SIZE), false);

	if (nPos != INVALID_SIZE)
	{
		assertRetVal(SetFilePointer(_fileHandle, nPos) == nPos, false);
	}

	return true;
}

bool ff::FileDataWriter::Write(LPCVOID pMem, size_t nBytes)
{
	assertRetVal(pMem && _fileHandle, false);

	if (nBytes)
	{
		assertRetVal(WriteFile(_fileHandle, pMem, nBytes), false);
	}

	return true;
}

size_t ff::FileDataWriter::GetSize() const
{
	assertRetVal(_fileHandle, 0);

	return GetFileSize(_fileHandle);
}

size_t ff::FileDataWriter::GetPos() const
{
	assertRetVal(_fileHandle, 0);

	return GetFilePointer(_fileHandle);
}

bool ff::FileDataWriter::SetPos(size_t nPos)
{
	assertRetVal(_fileHandle, false);

	return SetFilePointer(_fileHandle, nPos) == nPos;
}

bool ff::FileDataWriter::CreateSavedData(
	size_t start,
	size_t savedSize,
	size_t fullSize,
	bool compressed,
	ISavedData **obj)
{
	return ff::CreateSavedDataFromFile(_file, start, savedSize, fullSize, compressed, obj);
}

ff::VectorDataWriter::VectorDataWriter()
	: _pos(0)
{
}

ff::VectorDataWriter::~VectorDataWriter()
{
}

bool ff::VectorDataWriter::Init(IDataVector *pData, size_t nPos)
{
	assertRetVal(pData && nPos >= 0 && nPos <= pData->GetVector().Size(), false);

	_data = pData;
	_pos = nPos;

	return true;
}

bool ff::VectorDataWriter::Write(LPCVOID pMem, size_t nBytes)
{
	assertRetVal(_data, false);

	if (nBytes)
	{
		if (_pos + nBytes > _data->GetVector().Size())
		{
			_data->GetVector().Resize(_pos + nBytes);
			assertRetVal(_data->GetVector().Size() == _pos + nBytes, false);
		}

		CopyMemory(_data->GetVector().Data() + _pos, pMem, nBytes);
		_pos += nBytes;
	}

	return true;
}

size_t ff::VectorDataWriter::GetSize() const
{
	assertRetVal(_data, false);

	return _data->GetSize();
}

size_t ff::VectorDataWriter::GetPos() const
{
	return _pos;
}

bool ff::VectorDataWriter::SetPos(size_t nPos)
{
	assertRetVal(_data && nPos >= 0 && nPos <= _data->GetVector().Size(), false);

	_pos = nPos;

	return true;
}

bool ff::VectorDataWriter::CreateSavedData(
	size_t start,
	size_t savedSize,
	size_t fullSize,
	bool compressed,
	ISavedData **obj)
{
	ff::ComPtr<ff::IData> data;
	assertRetVal(ff::CreateDataInData(_data, start, savedSize, &data), false);

	return ff::CreateSavedDataFromMemory(data, fullSize, compressed, obj);
}

ff::FileDataReader::FileDataReader()
{
}

ff::FileDataReader::~FileDataReader()
{
}

bool ff::FileDataReader::Init(IDataFile *pFile, size_t nPos)
{
	assertRetVal(pFile, false);
	_file = pFile;

	assertRetVal(_file->OpenRead(_fileHandle), false);
	assertRetVal(SetFilePointer(_fileHandle, nPos) == nPos, false);

	return true;
}

const BYTE *ff::FileDataReader::Read(size_t nBytes)
{
	assertRetVal(_fileHandle, nullptr);

	_data.Resize(nBytes);

	if (nBytes)
	{
		assertRetVal(ReadFile(_fileHandle, nBytes, _data.Data()), false);
	}

	return _data.Data();
}

bool ff::FileDataReader::Read(size_t nBytes, IData **ppData)
{
	assertRetVal(_fileHandle, false);

	if (ppData && _file->GetMem() != nullptr)
	{
		assertRetVal(CreateDataInMemMappedFile(_file->GetMem() + GetPos(), nBytes, _file, ppData), false);
		assertRetVal(SetPos(GetPos() + nBytes), false);
	}
	else if (ppData)
	{
		ComPtr<IDataVector> pData;
		assertRetVal(CreateDataVector(nBytes, &pData), false);
		assertRetVal(ReadFile(_fileHandle, nBytes, pData->GetVector().Data()), false);
		*ppData = pData.Detach();
	}
	else
	{
		assertRetVal(SetPos(GetPos() + nBytes), false);
	}

	return true;
}

bool ff::FileDataReader::Read(size_t nStart, size_t nBytes, IData **ppData)
{
	assertRetVal(_fileHandle && ppData, false);

	if (_file->GetMem() != nullptr)
	{
		assertRetVal(CreateDataInMemMappedFile(_file->GetMem() + nStart, nBytes, _file, ppData), false);
	}
	else
	{
		size_t oldPos = GetPos();

		ComPtr<IDataVector> pData;
		assertRetVal(CreateDataVector(nBytes, &pData), false);
		assertRetVal(ReadFile(_fileHandle, nStart, nBytes, pData->GetVector().Data()), false);
		assertRetVal(SetPos(oldPos), false);
		*ppData = pData.Detach();
	}

	return true;
}

size_t ff::FileDataReader::GetSize() const
{
	assertRetVal(_fileHandle, 0);

	return GetFileSize(_fileHandle);
}

size_t ff::FileDataReader::GetPos() const
{
	assertRetVal(_fileHandle, 0);

	return GetFilePointer(_fileHandle);
}

bool ff::FileDataReader::SetPos(size_t nPos)
{
	assertRetVal(_fileHandle, false);

	return SetFilePointer(_fileHandle, nPos) == nPos;
}

bool ff::FileDataReader::CreateSavedData(
	size_t start,
	size_t savedSize,
	size_t fullSize,
	bool compressed,
	ISavedData **obj)
{
	return ff::CreateSavedDataFromFile(_file, start, savedSize, fullSize, compressed, obj);
}

ff::CMemDataReader::CMemDataReader()
{
}

ff::CMemDataReader::~CMemDataReader()
{
}

bool ff::CMemDataReader::Init(IData *pData, size_t nPos)
{
	assertRetVal(pData && nPos >= 0 && nPos <= pData->GetSize(), false);

	_data = pData;
	_pos = nPos;

	return true;
}

const BYTE *ff::CMemDataReader::Read(size_t nBytes)
{
	assertRetVal(_data && _pos + nBytes <= _data->GetSize(), nullptr);

	const BYTE *pMem = _data->GetMem() + _pos;
	_pos += nBytes;

	return pMem;
}

bool ff::CMemDataReader::Read(size_t nBytes, IData **ppData)
{
	assertRetVal(_data && _pos + nBytes <= _data->GetSize(), false);

	if (ppData)
	{
		assertRetVal(CreateDataInData(_data, _pos, nBytes, ppData), false);
	}

	_pos += nBytes;

	return true;
}

bool ff::CMemDataReader::Read(size_t nStart, size_t nBytes, IData **ppData)
{
	assertRetVal(ppData && _data && nStart + nBytes <= _data->GetSize(), false);
	assertRetVal(CreateDataInData(_data, nStart, nBytes, ppData), false);

	return true;
}

size_t ff::CMemDataReader::GetSize() const
{
	assertRetVal(_data, 0);

	return _data->GetSize();
}

size_t ff::CMemDataReader::GetPos() const
{
	assertRetVal(_data, 0);

	return _pos;
}

bool ff::CMemDataReader::SetPos(size_t nPos)
{
	assertRetVal(_data && nPos >= 0 && nPos <= _data->GetSize(), false);

	_pos = nPos;
	
	return true;
}

bool ff::CMemDataReader::CreateSavedData(
	size_t start,
	size_t savedSize,
	size_t fullSize,
	bool compressed,
	ISavedData **obj)
{
	ff::ComPtr<ff::IData> data;
	assertRetVal(ff::CreateDataInData(_data, start, savedSize, &data), false);

	return ff::CreateSavedDataFromMemory(data, fullSize, compressed, obj);
}

#if METRO_APP

ff::RandomAccessStreamReader::RandomAccessStreamReader()
{
}

ff::RandomAccessStreamReader::~RandomAccessStreamReader()
{
}

bool ff::RandomAccessStreamReader::Init(Windows::Storage::Streams::IRandomAccessStream ^stream)
{
	assertRetVal(stream, false);
	_stream = stream;
	return true;
}

Windows::Storage::Streams::IRandomAccessStream ^ff::RandomAccessStreamReader::GetStream() const
{
	return _stream;
}

const BYTE *ff::RandomAccessStreamReader::Read(size_t nBytes)
{
	// Can't block the UI thread
	assertRetVal(!ff::GetMainThreadDispatch()->IsCurrentThread(), nullptr);
	assertRetVal(GetPos() + nBytes <= GetSize() && _stream->CanRead, nullptr);

	if (!_buffer || _buffer->Length < nBytes)
	{
		_buffer = ref new Windows::Storage::Streams::Buffer((unsigned int)nBytes);
		assertRetVal(_bufferBytes.QueryFrom(_buffer), nullptr);
	}

	try
	{
		auto task = concurrency::create_task(_stream->ReadAsync(
			_buffer,
			(unsigned int)nBytes,
			Windows::Storage::Streams::InputStreamOptions::None));

		Windows::Storage::Streams::IBuffer ^buffer = task.get();
		assert(buffer == _buffer);
	}
	catch (Platform::Exception ^exception)
	{
		assertSz(false, exception->Message->Data());
		return nullptr;
	}

	BYTE *data = nullptr;
	assertHrRetVal(_bufferBytes->Buffer(&data), nullptr);
	return data;
}

bool ff::RandomAccessStreamReader::Read(size_t nBytes, IData **ppData)
{
	// Can't block the UI thread
	assertRetVal(!ff::GetMainThreadDispatch()->IsCurrentThread(), false);
	assertRetVal(ppData && GetPos() + nBytes <= GetSize() && _stream->CanRead, false);

	ComPtr<IData> pData;
	Windows::Storage::Streams::Buffer ^buffer = ref new Windows::Storage::Streams::Buffer((unsigned int)nBytes);
	assertRetVal(ff::CreateDataFromBuffer(buffer, &pData), false);

	try
	{
		auto task = concurrency::create_task(_stream->ReadAsync(
			buffer,
			(unsigned int)nBytes,
			Windows::Storage::Streams::InputStreamOptions::None));

		Windows::Storage::Streams::IBuffer ^buffer2 = task.get();
		assert(buffer2 == buffer);
	}
	catch (Platform::Exception ^exception)
	{
		assertSz(false, exception->Message->Data());
		return false;
	}

	*ppData = pData.Detach();
	return true;
}

bool ff::RandomAccessStreamReader::Read(size_t nStart, size_t nBytes, IData **ppData)
{
	size_t oldPos = GetPos();
	assertRetVal(SetPos(nStart), false);
	assertRetVal(Read(nBytes, ppData), false);
	assertRetVal(SetPos(oldPos), false);
	return true;
}

size_t ff::RandomAccessStreamReader::GetSize() const
{
	return (size_t)_stream->Size;
}

size_t ff::RandomAccessStreamReader::GetPos() const
{
	return (size_t)_stream->Position;
}

bool ff::RandomAccessStreamReader::SetPos(size_t nPos)
{
	assertRetVal(nPos <= GetSize(), false);
	_stream->Seek(nPos);
	return true;
}

bool ff::RandomAccessStreamReader::CreateSavedData(
	size_t start,
	size_t savedSize,
	size_t fullSize,
	bool compressed,
	ISavedData **obj)
{
	ComPtr<IData> data;
	assertRetVal(Read(start, savedSize, &data), false);
	return ff::CreateSavedDataFromMemory(data, fullSize, compressed, obj);
}

#endif // METRO_APP
