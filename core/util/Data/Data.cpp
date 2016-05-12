#include "pch.h"
#include "COM/ComObject.h"
#include "Data/Data.h"
#include "Data/DataFile.h"

#if METRO_APP
#include <robuffer.h>
#endif

namespace ff
{
	class __declspec(uuid("d16151be-46ad-46c8-9441-8e26cd6cf136"))
		CData : public ComBase, public IData
	{
	public:
		DECLARE_HEADER(CData);

		bool Init(const BYTE *pMem, size_t nSize, IDataFile *pFile);

		// IData functions
		virtual const BYTE *GetMem() override;
		virtual size_t GetSize() override;
		virtual IDataFile *GetFile() override;
		virtual bool IsStatic() override;

	private:
		ComPtr<IDataFile> _file;
		const BYTE *_mem;
		size_t _size;
	};
}

BEGIN_INTERFACES(ff::CData)
	HAS_INTERFACE(ff::IData)
END_INTERFACES()

namespace ff
{
	class __declspec(uuid("f6b6f0a6-c7b5-468f-9042-62e81a1eeba5"))
		CChildData : public ComBase, public IData
	{
	public:
		DECLARE_HEADER(CChildData);

		bool Init(IData *pParentData, size_t nPos, size_t nSize);

		// IData functions
		virtual const BYTE *GetMem() override;
		virtual size_t GetSize() override;
		virtual IDataFile *GetFile() override;
		virtual bool IsStatic() override;

	private:
		ComPtr<IData> _parentData;
		size_t _pos;
		size_t _size;
	};
}

BEGIN_INTERFACES(ff::CChildData)
	HAS_INTERFACE(ff::IData)
END_INTERFACES()

namespace ff
{
	class __declspec(uuid("630c0577-9d72-44ed-85e0-c16e3d1e7664"))
		CDataVector : public ComBase, public IDataVector
	{
	public:
		DECLARE_HEADER(CDataVector);

		bool Init(size_t nInitialSize);

		// IData functions
		virtual const BYTE *GetMem() override;
		virtual size_t GetSize() override;
		virtual IDataFile *GetFile() override;
		virtual bool IsStatic() override;

		// IDataVector functions

		virtual Vector<BYTE> &GetVector() override;

	private:
		Vector<BYTE> _data;
	};
}

BEGIN_INTERFACES(ff::CDataVector)
	HAS_INTERFACE(ff::IData)
	HAS_INTERFACE(ff::IDataVector)
END_INTERFACES()

#if METRO_APP

namespace ff
{
	class __declspec(uuid("3d8c0f33-4d02-46b8-80f8-ce87548349f8"))
		CDataBuffer : public ComBase, public IData
	{
	public:
		DECLARE_HEADER(CDataBuffer);

		bool Init(Windows::Storage::Streams::IBuffer ^buffer);

		// IData functions
		virtual const BYTE *GetMem() override;
		virtual size_t GetSize() override;
		virtual IDataFile *GetFile() override;
		virtual bool IsStatic() override;

	private:
		Windows::Storage::Streams::IBuffer ^_buffer;
		const BYTE *_data;
	};
}

BEGIN_INTERFACES(ff::CDataBuffer)
	HAS_INTERFACE(ff::IData)
END_INTERFACES()

#endif

bool ff::CreateDataInMemMappedFile(IDataFile *pFile, IData **ppData)
{
	return CreateDataInMemMappedFile(nullptr, INVALID_SIZE, pFile, ppData);
}

bool ff::CreateDataInMemMappedFile(const BYTE *pMem, size_t nSize, IDataFile *pFile, IData **ppData)
{
	assertRetVal(ppData, false);
	*ppData = nullptr;

	ComPtr<CData> pData = new ComObject<CData>;
	assertRetVal(pData->Init(pMem, nSize, pFile), false);

	*ppData = pData.Detach();

	return true;
}

bool ff::CreateDataInStaticMem(const BYTE *pMem, size_t nSize, IData **ppData)
{
	return CreateDataInMemMappedFile(pMem, nSize, nullptr, ppData);
}

#if !METRO_APP
bool ff::CreateDataInResource(HINSTANCE hInstance, UINT id, IData **ppData)
{
	return ff::CreateDataInResource(hInstance, MAKEINTRESOURCE(id), RT_RCDATA, ppData);
}

bool ff::CreateDataInResource(HINSTANCE hInstance, LPCWSTR type, LPCWSTR name, IData **ppData)
{
	assertRetVal(ppData, false);
	*ppData = nullptr;

	assertRetVal(hInstance, false);

	HRSRC hFound = ::FindResource(hInstance, name, type);
	HGLOBAL hRes = hFound ? ::LoadResource(hInstance, hFound) : nullptr;
	const BYTE *pMem = hRes ? (LPBYTE)::LockResource(hRes) : nullptr;
	size_t nSize = pMem ? ::SizeofResource(hInstance, hFound) : 0;

	assertRetVal(nSize, false);

	return CreateDataInMemMappedFile(pMem, nSize, nullptr, ppData);
}
#endif // !METRO_APP

bool ff::CreateDataInData(IData *pParentData, size_t nPos, size_t nSize, IData **ppChildData)
{
	assertRetVal(ppChildData, false);
	*ppChildData = nullptr;

	ComPtr<CChildData> pChildData = new ComObject<CChildData>;
	assertRetVal(pChildData->Init(pParentData, nPos, nSize), false);

	*ppChildData = pChildData.Detach();

	return true;
}

bool ff::CreateDataVector(size_t nInitialSize, IDataVector **ppData)
{
	assertRetVal(ppData, false);
	*ppData = nullptr;

	ComPtr<CDataVector> pData = new ComObject<CDataVector>;
	assertRetVal(pData->Init(nInitialSize), false);

	*ppData = pData.Detach();
	return true;
}

#if METRO_APP
bool ff::CreateDataFromBuffer(Windows::Storage::Streams::IBuffer ^buffer, IData **ppData)
{
	assertRetVal(ppData, false);
	*ppData = nullptr;

	ComPtr<CDataBuffer> pData = new ComObject<CDataBuffer>;
	assertRetVal(pData->Init(buffer), false);

	*ppData = pData.Detach();
	return true;
}
#endif

ff::CData::CData()
{
}

ff::CData::~CData()
{
	if (_file)
	{
		_file->CloseMemMapped();
	}
}

bool ff::CData::Init(const BYTE *pMem, size_t nSize, IDataFile *pFile)
{
	_file = pFile;
	_mem = pMem;
	_size = nSize;

	if (_file)
	{
		assertRetVal(_file->OpenReadMemMapped(), false);

		if (!_mem)
		{
			assert(_size == INVALID_SIZE);
			_mem = _file->GetMem();
			_size = _file->GetSize();
		}
	}

	return _mem != nullptr;
}

const BYTE *ff::CData::GetMem()
{
	return _mem;
}

size_t ff::CData::GetSize()
{
	assertRetVal(_mem, 0);

	return _size;
}

ff::IDataFile *ff::CData::GetFile()
{
	return _file;
}

bool ff::CData::IsStatic()
{
	return true;
}

ff::CChildData::CChildData()
{
}

ff::CChildData::~CChildData()
{
}

bool ff::CChildData::Init(IData *pParentData, size_t nPos, size_t nSize)
{
	assertRetVal(
		pParentData &&
		nPos >= 0 &&
		nPos <= pParentData->GetSize() &&
		nSize >= 0 &&
		nPos + nSize <= pParentData->GetSize(),
		false);

	_parentData = pParentData;
	_pos = nPos;
	_size = nSize;

	return true;
}

const BYTE *ff::CChildData::GetMem()
{
	return _parentData->GetMem() + _pos;
}

size_t ff::CChildData::GetSize()
{
	return _size;
}

ff::IDataFile *ff::CChildData::GetFile()
{
	return _parentData->GetFile();
}

bool ff::CChildData::IsStatic()
{
	return _parentData->IsStatic();
}

ff::CDataVector::CDataVector()
{
}

ff::CDataVector::~CDataVector()
{
}

bool ff::CDataVector::Init(size_t nInitialSize)
{
	_data.Resize(nInitialSize);

	return _data.Size() == nInitialSize;
}

const BYTE *ff::CDataVector::GetMem()
{
	return _data.Data();
}

size_t ff::CDataVector::GetSize()
{
	return _data.Size();
}

ff::IDataFile *ff::CDataVector::GetFile()
{
	return nullptr;
}

bool ff::CDataVector::IsStatic()
{
	return false;
}

ff::Vector<BYTE> &ff::CDataVector::GetVector()
{
	return _data;
}

#if METRO_APP

ff::CDataBuffer::CDataBuffer()
	: _data(nullptr)
{
}

ff::CDataBuffer::~CDataBuffer()
{
}

bool ff::CDataBuffer::Init(Windows::Storage::Streams::IBuffer ^buffer)
{
	assertRetVal(buffer, false);

	ComPtr<Windows::Storage::Streams::IBufferByteAccess> bytes;
	assertRetVal(bytes.QueryFrom(buffer), false);
	assertHrRetVal(bytes->Buffer((byte **)&_data), false);

	_buffer = buffer;
	return true;
}

const BYTE *ff::CDataBuffer::GetMem()
{
	return _data;
}

size_t ff::CDataBuffer::GetSize()
{
	return _buffer->Length;
}

ff::IDataFile *ff::CDataBuffer::GetFile()
{
	return nullptr;
}

bool ff::CDataBuffer::IsStatic()
{
	return false;
}

#endif
