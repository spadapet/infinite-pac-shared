#include "pch.h"
#include "COM/ComObject.h"
#include "Data/Data.h"
#include "Data/DataWriterReader.h"
#include "Data/Stream.h"
#include "Globals/Log.h"

namespace ff
{
	class __declspec(uuid("6d01bbbf-9e9c-46d3-ba84-c7fba920486e"))
		CSavedDataStream : public ComBase, public IStream
	{
	public:
		DECLARE_HEADER(CSavedDataStream);

		bool Init(IData *pReadData, IDataVector *pWriteData, size_t nPos);

		// IStream functions

		COM_FUNC Read(void *pv, ULONG cb, ULONG *pcbRead) override;
		COM_FUNC Write(const void *pv, ULONG cb, ULONG *pcbWritten) override;
		COM_FUNC Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition) override;
		COM_FUNC SetSize(ULARGE_INTEGER libNewSize) override;
		COM_FUNC CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten) override;
		COM_FUNC Commit(DWORD grfCommitFlags) override;
		COM_FUNC Revert() override;
		COM_FUNC LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) override;
		COM_FUNC UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) override;
		COM_FUNC Stat(STATSTG *pstatstg, DWORD grfStatFlag) override;
		COM_FUNC Clone(IStream **ppstm) override;

	protected:
		const BYTE *Data();
		LPBYTE WritableData();
		size_t Size();

		ComPtr<IData> _readData;
		ComPtr<IDataVector> _writeData;
		size_t _pos;
	};

	class __declspec(uuid("1a446cf8-07a8-4913-adfe-7368c4c7d706"))
		DataReaderStream : public ComBase, public IStream
	{
	public:
		DECLARE_HEADER(DataReaderStream);

		bool Init(IDataReader *reader);

		// IStream functions

		COM_FUNC Read(void *pv, ULONG cb, ULONG *pcbRead) override;
		COM_FUNC Write(const void *pv, ULONG cb, ULONG *pcbWritten) override;
		COM_FUNC Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition) override;
		COM_FUNC SetSize(ULARGE_INTEGER libNewSize) override;
		COM_FUNC CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten) override;
		COM_FUNC Commit(DWORD grfCommitFlags) override;
		COM_FUNC Revert() override;
		COM_FUNC LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) override;
		COM_FUNC UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) override;
		COM_FUNC Stat(STATSTG *pstatstg, DWORD grfStatFlag) override;
		COM_FUNC Clone(IStream **ppstm) override;

	protected:
		ComPtr<IDataReader> _reader;
	};
}

BEGIN_INTERFACES(ff::CSavedDataStream)
	HAS_INTERFACE(IStream)
END_INTERFACES()

BEGIN_INTERFACES(ff::DataReaderStream)
	HAS_INTERFACE(IStream)
END_INTERFACES()

bool ff::CreateWriteStream(IDataVector **ppData, IStream **ppStream)
{
	ComPtr<IDataVector> pDataVector;
	assertRetVal(CreateDataVector(0, &pDataVector), false);

	if (ppData)
	{
		*ppData = GetAddRef<IDataVector>(pDataVector);
	}

	return CreateWriteStream(pDataVector, 0, ppStream);
}

bool ff::CreateWriteStream(IDataVector *pData, size_t nPos, IStream **ppStream)
{
	assertRetVal(ppStream, false);
	*ppStream = nullptr;

	ComPtr<CSavedDataStream> pStream = new ComObject<CSavedDataStream>;
	assertRetVal(pStream->Init(nullptr, pData, nPos), false);

	*ppStream = pStream.Detach();
	return true;
}

bool ff::CreateReadStream(IData *pData, size_t nPos, IStream **ppStream)
{
	assertRetVal(ppStream, false);
	*ppStream = nullptr;

	ComPtr<CSavedDataStream> pStream = new ComObject<CSavedDataStream>;
	assertRetVal(pStream->Init(pData, nullptr, nPos), false);

	*ppStream = pStream.Detach();
	return true;
}

bool ff::CreateReadStream(IDataReader *reader, IStream **obj)
{
	assertRetVal(obj, false);
	*obj = nullptr;

	ComPtr<DataReaderStream> myObj = new ComObject<DataReaderStream>;
	assertRetVal(myObj->Init(reader), false);

	*obj = myObj.Detach();
	return true;
}

bool ff::CreateReadStream(IDataReader *reader, IMFByteStream **obj)
{
	assertRetVal(obj, false);
	*obj = nullptr;

#if METRO_APP
	// Use the Windows stream implementation when possible
	{
		Windows::Storage::Streams::IRandomAccessStream ^stream = ff::GetRandomAccessStream(reader);
		if (stream && SUCCEEDED(MFCreateMFByteStreamOnStreamEx((IUnknown *) stream, obj)))
		{
			return true;
		}
	}

	// Use IStream
	{
		ff::ComPtr<IStream> stream;
		assertRetVal(ff::CreateReadStream(reader, &stream), false);
		assertHrRetVal(MFCreateMFByteStreamOnStreamEx(stream, obj), false);
	}
#else
	// Use IStream
	{
		ff::ComPtr<IStream> stream;
		assertRetVal(ff::CreateReadStream(reader, &stream), false);
		assertHrRetVal(MFCreateMFByteStreamOnStream(stream, obj), false);
	}
#endif

	return true;
}

ff::CSavedDataStream::CSavedDataStream()
	: _pos(0)
{
}

ff::CSavedDataStream::~CSavedDataStream()
{
}

bool ff::CSavedDataStream::Init(IData *pReadData, IDataVector *pWriteData, size_t nPos)
{
	_readData = pReadData;
	_writeData = pWriteData;
	_pos = nPos;

	if (!_readData && !_writeData)
	{
		assertRetVal(CreateDataVector(0, &_writeData), false);
	}
	
	return true;
}

const BYTE *ff::CSavedDataStream::Data()
{
	if (_readData)
	{
		return _readData->GetMem();
	}
	else if (_writeData)
	{
		return _writeData->GetMem();
	}
	else
	{
		assertRetVal(false, nullptr);
	}
}

LPBYTE ff::CSavedDataStream::WritableData()
{
	if (_writeData)
	{
		return _writeData->GetVector().Data();
	}
	else
	{
		assertRetVal(false, nullptr);
	}
}

size_t ff::CSavedDataStream::Size()
{
	if (_readData)
	{
		return _readData->GetSize();
	}
	else if (_writeData)
	{
		return _writeData->GetSize();
	}
	else
	{
		assertRetVal(false, 0);
	}
}

HRESULT ff::CSavedDataStream::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
	assertRetVal(Data(), E_FAIL);

	size_t nRead = 0;
	
	if (_pos < Size())
	{
		nRead = std::min((size_t)cb, Size() - _pos);
	}

	if (pcbRead)
	{
		*pcbRead = (ULONG)nRead;
	}

	if (nRead)
	{
		CopyMemory(pv, Data() + _pos, nRead);
		_pos += nRead;
	}

	return (nRead == (size_t)cb) ? S_OK : S_FALSE;
}

HRESULT ff::CSavedDataStream::Write(const void *pv, ULONG cb, ULONG *pcbWritten)
{
	assertRetVal(_writeData, STG_E_CANTSAVE);
	assertRetVal(pv, STG_E_INVALIDPOINTER);

	size_t nWrite = (size_t)cb;

	if (_pos + nWrite > Size())
	{
		_writeData->GetVector().Resize(_pos + nWrite);
	}

	assertRetVal(Size() >= _pos + nWrite, STG_E_MEDIUMFULL);

	if (pcbWritten)
	{
		*pcbWritten = (ULONG)nWrite;
	}

	if (nWrite)
	{
		CopyMemory(WritableData() + _pos, pv, nWrite);
		_pos += nWrite;
	}

	return S_OK;
}

HRESULT ff::CSavedDataStream::Seek(
	LARGE_INTEGER dlibMove,
	DWORD dwOrigin,
	ULARGE_INTEGER *plibNewPosition)
{
	assertRetVal(Data(), STG_E_INVALIDFUNCTION);

	LARGE_INTEGER nSize;
	nSize.QuadPart = (LONGLONG)Size();

	LARGE_INTEGER nNewPos;
	nNewPos.QuadPart = (LONGLONG)_pos;

	if (nNewPos.QuadPart < 0 || nSize.QuadPart < 0)
	{
		assertRetVal(false, STG_E_INVALIDFUNCTION);
	}

	if (plibNewPosition)
	{
		*plibNewPosition = *(ULARGE_INTEGER*)&nNewPos;
	}

	switch (dwOrigin)
	{
		case STREAM_SEEK_SET:
			nNewPos = dlibMove;
			break;

		case STREAM_SEEK_CUR:
			nNewPos.QuadPart += dlibMove.QuadPart;
			break;

		case STREAM_SEEK_END:
			nNewPos.QuadPart = nSize.QuadPart + dlibMove.QuadPart;
			break;

		default:
			return STG_E_INVALIDFUNCTION;
	}

	if (nNewPos.QuadPart < 0)
	{
		assertRetVal(false, STG_E_INVALIDFUNCTION);
	}

	if (plibNewPosition)
	{
		*plibNewPosition = *(ULARGE_INTEGER*)&nNewPos;
	}

	_pos = (size_t)nNewPos.QuadPart;

	return S_OK;
}

HRESULT ff::CSavedDataStream::SetSize(ULARGE_INTEGER libNewSize)
{
	assertRetVal(_writeData, STG_E_INVALIDFUNCTION);

	size_t nNewSize = (size_t)libNewSize.QuadPart;

	_writeData->GetVector().Resize(nNewSize);

	return S_OK;
}

HRESULT ff::CSavedDataStream::CopyTo(
	IStream *pstm,
	ULARGE_INTEGER cb,
	ULARGE_INTEGER *pcbRead,
	ULARGE_INTEGER *pcbWritten)
{
	assertRetVal(pstm && pstm != this && Data(), STG_E_INVALIDPOINTER);

	size_t nRead = 0;

	if (_pos < Size())
	{
		nRead = std::min((size_t)cb.QuadPart, Size() - _pos);
	}

	if (pcbRead)
	{
		pcbRead->QuadPart = (ULONGLONG)nRead;
	}

	if (nRead)
	{
		ComPtr<CSavedDataStream> pMyDest;

		if (pMyDest.QueryFrom(pstm) && pMyDest->Data() == Data())
		{
			// copying within myself, so be safe and make a copy of the data first

			Vector<BYTE> dataCopy;
			dataCopy.Resize(nRead);
			CopyMemory(dataCopy.Data(), Data() + _pos, nRead);

			ULONG nWritten = 0;
			HRESULT hr = pstm->Write(dataCopy.Data(), (ULONG)nRead, &nWritten);
			_pos += nRead;

			if (pcbWritten)
			{
				pcbWritten->QuadPart = (ULONGLONG)nWritten;
			}

			return hr;
		}
		else
		{
			ULONG nWritten = 0;
			HRESULT hr = pstm->Write(Data() + _pos, (ULONG)nRead, &nWritten);
			_pos += nRead;

			if (pcbWritten)
			{
				pcbWritten->QuadPart = (ULONGLONG)nWritten;
			}

			return hr;
		}
	}
	else
	{
		if (pcbWritten)
		{
			pcbWritten->QuadPart = 0;
		}

		return S_OK;
	}
}

HRESULT ff::CSavedDataStream::Commit(DWORD grfCommitFlags)
{
	return S_OK;
}

HRESULT ff::CSavedDataStream::Revert()
{
	return S_OK;
}

HRESULT ff::CSavedDataStream::LockRegion(
	ULARGE_INTEGER libOffset,
	ULARGE_INTEGER cb,
	DWORD dwLockType)
{
	return STG_E_INVALIDFUNCTION;
}

HRESULT ff::CSavedDataStream::UnlockRegion(
	ULARGE_INTEGER libOffset,
	ULARGE_INTEGER cb,
	DWORD dwLockType)
{
	return STG_E_INVALIDFUNCTION;
}

HRESULT ff::CSavedDataStream::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
	assertRetVal(pstatstg && Data(), STG_E_INVALIDPOINTER);
	ff::ZeroObject(*pstatstg);

	pstatstg->type = STGTY_STREAM;
	pstatstg->cbSize.QuadPart = (ULONGLONG)Size();

	return S_OK;
}

HRESULT ff::CSavedDataStream::Clone(IStream **ppstm)
{
	assertRetVal(ppstm, STG_E_INVALIDPOINTER);
	*ppstm = nullptr;

	ComPtr<CSavedDataStream> pClone = new ComObject<CSavedDataStream>;
	assertRetVal(pClone->Init(_readData, _writeData, _pos), E_FAIL);

	*ppstm = pClone.Detach();

	return S_OK;
}

ff::DataReaderStream::DataReaderStream()
{
}

ff::DataReaderStream::~DataReaderStream()
{
}

bool ff::DataReaderStream::Init(IDataReader *reader)
{
	assertRetVal(reader, false);
	_reader = reader;
	return true;
}

HRESULT ff::DataReaderStream::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
	size_t bytesLeft = _reader->GetSize() - _reader->GetPos();
	size_t nRead = std::min((size_t)cb, bytesLeft);

	if (pcbRead)
	{
		*pcbRead = (ULONG)nRead;
	}

	if (nRead)
	{
		::CopyMemory(pv, _reader->Read(nRead), nRead);
	}

	return (nRead == (size_t)cb) ? S_OK : S_FALSE;
}

HRESULT ff::DataReaderStream::Write(const void *pv, ULONG cb, ULONG *pcbWritten)
{
	assertRetVal(false, STG_E_CANTSAVE);
}

HRESULT ff::DataReaderStream::Seek(
	LARGE_INTEGER dlibMove,
	DWORD dwOrigin,
	ULARGE_INTEGER *plibNewPosition)
{
	LARGE_INTEGER nSize;
	nSize.QuadPart = (LONGLONG)_reader->GetSize();

	LARGE_INTEGER nNewPos;
	nNewPos.QuadPart = (LONGLONG)_reader->GetPos();

	if (nNewPos.QuadPart < 0 || nSize.QuadPart < 0)
	{
		assertRetVal(false, STG_E_INVALIDFUNCTION);
	}

	if (plibNewPosition)
	{
		*plibNewPosition = *(ULARGE_INTEGER*)&nNewPos;
	}

	switch (dwOrigin)
	{
		case STREAM_SEEK_SET:
			nNewPos = dlibMove;
			break;

		case STREAM_SEEK_CUR:
			nNewPos.QuadPart += dlibMove.QuadPart;
			break;

		case STREAM_SEEK_END:
			nNewPos.QuadPart = nSize.QuadPart + dlibMove.QuadPart;
			break;

		default:
			return STG_E_INVALIDFUNCTION;
	}

	if (nNewPos.QuadPart < 0)
	{
		assertRetVal(false, STG_E_INVALIDFUNCTION);
	}

	if (plibNewPosition)
	{
		*plibNewPosition = *(ULARGE_INTEGER*)&nNewPos;
	}

	assertRetVal(_reader->SetPos((size_t)nNewPos.QuadPart), STG_E_INVALIDFUNCTION);

	return S_OK;
}

HRESULT ff::DataReaderStream::SetSize(ULARGE_INTEGER libNewSize)
{
	return STG_E_INVALIDFUNCTION;
}

HRESULT ff::DataReaderStream::CopyTo(
	IStream *pstm,
	ULARGE_INTEGER cb,
	ULARGE_INTEGER *pcbRead,
	ULARGE_INTEGER *pcbWritten)
{
	assertRetVal(pstm && pstm != this, STG_E_INVALIDPOINTER);

	size_t bytesLeft = _reader->GetSize() - _reader->GetPos();
	size_t nRead = std::min((size_t)cb.QuadPart, bytesLeft);

	if (pcbRead)
	{
		pcbRead->QuadPart = (ULONGLONG)nRead;
	}

	if (nRead)
	{
		ULONG nWritten = 0;
		HRESULT hr = pstm->Write(_reader->Read(nRead), (ULONG)nRead, &nWritten);

		if (pcbWritten)
		{
			pcbWritten->QuadPart = (ULONGLONG)nWritten;
		}

		return hr;
	}
	else
	{
		if (pcbWritten)
		{
			pcbWritten->QuadPart = 0;
		}

		return S_OK;
	}
}

HRESULT ff::DataReaderStream::Commit(DWORD grfCommitFlags)
{
	return S_OK;
}

HRESULT ff::DataReaderStream::Revert()
{
	return S_OK;
}

HRESULT ff::DataReaderStream::LockRegion(
	ULARGE_INTEGER libOffset,
	ULARGE_INTEGER cb,
	DWORD dwLockType)
{
	return STG_E_INVALIDFUNCTION;
}

HRESULT ff::DataReaderStream::UnlockRegion(
	ULARGE_INTEGER libOffset,
	ULARGE_INTEGER cb,
	DWORD dwLockType)
{
	return STG_E_INVALIDFUNCTION;
}

HRESULT ff::DataReaderStream::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
	assertRetVal(pstatstg, STG_E_INVALIDPOINTER);
	ZeroMemory(pstatstg, sizeof(*pstatstg));

	pstatstg->type = STGTY_STREAM;
	pstatstg->cbSize.QuadPart = (ULONGLONG)_reader->GetSize();

	return S_OK;
}

HRESULT ff::DataReaderStream::Clone(IStream **ppstm)
{
	assertRetVal(false, STG_E_INVALIDFUNCTION);
}
