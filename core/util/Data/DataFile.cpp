#include "pch.h"
#include "COM/ComAlloc.h"
#include "COM/ComObject.h"
#include "Data/DataFile.h"
#include "Windows/FileUtil.h"

class __declspec(uuid("30bbbdcc-3ff1-4b2c-ab87-10a7f52f2156"))
	CDataFile : public ff::ComBase, public ff::IDataFile
{
public:
	DECLARE_HEADER(CDataFile);

	bool Init(ff::StringRef path, bool bTempFile);
	bool InitTempFile();

	// IDataFile functions

	virtual bool OpenWrite(ff::File &handle, bool bAppend) override;
	virtual bool OpenRead (ff::File &handle) override;

	virtual bool OpenReadMemMapped() override;
	virtual bool CloseMemMapped() override;
	virtual const BYTE *GetMem() const override;

	virtual ff::StringRef GetPath() const override;
	virtual size_t GetSize() const override;

private:
	ff::String _path;
	bool _temp;

	// For mem mapped files:
	long _open;
	ff::File _file;
	ff::MemMappedFile _mapping;
};

BEGIN_INTERFACES(CDataFile)
	HAS_INTERFACE(ff::IDataFile)
END_INTERFACES()

bool ff::CreateTempDataFile(ff::IDataFile **obj)
{
	ff::ComPtr<CDataFile, IDataFile> myObj;
	assertHrRetVal(ff::ComAllocator<CDataFile>::CreateInstance(&myObj), false);
	assertRetVal(myObj->InitTempFile(), false);	

	*obj = myObj.Detach();
	return true;
}

bool ff::CreateDataFile(StringRef path, bool bTempFile, ff::IDataFile **obj)
{
	ff::ComPtr<CDataFile, IDataFile> myObj;
	assertHrRetVal(ff::ComAllocator<CDataFile>::CreateInstance(&myObj), false);
	assertRetVal(myObj->Init(path, bTempFile), false);	

	*obj = myObj.Detach();
	return true;
}

CDataFile::CDataFile()
	: _temp(false)
	, _open(0)
{
}

CDataFile::~CDataFile()
{
	assert(!_open);

	_mapping.Close();
	_file.Close();

	if (_temp && ff::FileExists(_path))
	{
		verify(ff::DeleteFile(_path));
	}
}

bool CDataFile::InitTempFile()
{
	_path = ff::CreateTempFile();
	_temp = true;

	return !_path.empty();
}

bool CDataFile::Init(ff::StringRef path, bool bTempFile)
{
	assertRetVal(path.size(), false);

	_path = path;
	_temp = bTempFile;

	return true;
}

bool CDataFile::OpenWrite(ff::File &handle, bool bAppend)
{
	return handle.OpenWrite(_path, bAppend);
}

bool CDataFile::OpenRead(ff::File &handle)
{
	return handle.OpenRead(_path);
}

bool CDataFile::OpenReadMemMapped()
{
	if (InterlockedIncrement(&_open) == 1)
	{
		if (_file.OpenRead(_path))
		{
			if (!_mapping.OpenRead(_file))
			{
				_file.Close();
			}
		}
	}

	assertRetVal(_mapping && _file, false);

	return true;
}

bool CDataFile::CloseMemMapped()
{
	assertRetVal(_open > 0, false);

	if (!InterlockedDecrement(&_open))
	{
		_mapping.Close();
		_file.Close();
	}

	return true;
}

const BYTE *CDataFile::GetMem() const
{
	return _mapping ? _mapping.GetMem() : nullptr;
}

ff::StringRef CDataFile::GetPath() const
{
	return _path;
}

size_t CDataFile::GetSize() const
{
	return (_open && _file)
		? (_mapping ? _mapping.GetSize() : GetFileSize(_file))
		: GetFileSize(GetPath());
}
