#include "pch.h"
#include "COM/ComObject.h"
#include "Data/Data.h"
#include "Graph/DataBlob.h"

#include <DirectXTex.h>

namespace ff
{
	class __declspec(uuid("ea478997-3767-4438-8234-a3b2ab31ee89"))
		DataBlob : public ComBase, public IData
	{
	public:
		DECLARE_HEADER(DataBlob);

		bool Init(ID3DBlob *blob);

		// IData functions
		virtual const BYTE *GetMem() override;
		virtual size_t GetSize() override;
		virtual IDataFile *GetFile() override;
		virtual bool IsStatic() override;

	private:
		ComPtr<ID3DBlob> _blob;
	};

	class __declspec(uuid("8cd06f91-e7d2-4b3f-8624-33ccde129b5e"))
		DataTexBlob : public ComBase, public IData
	{
	public:
		DECLARE_HEADER(DataTexBlob);

		bool Init(DirectX::Blob &&blob);

		// IData functions
		virtual const BYTE *GetMem() override;
		virtual size_t GetSize() override;
		virtual IDataFile *GetFile() override;
		virtual bool IsStatic() override;

	private:
		DirectX::Blob _blob;
	};
}

BEGIN_INTERFACES(ff::DataBlob)
	HAS_INTERFACE(ff::IData)
END_INTERFACES()

BEGIN_INTERFACES(ff::DataTexBlob)
	HAS_INTERFACE(ff::IData)
END_INTERFACES()

bool ff::CreateDataFromBlob(ID3DBlob *blob, IData **obj)
{
	assertRetVal(blob && obj, false);
	*obj = nullptr;

	ComPtr<DataBlob> myObj = new ComObject<DataBlob>;
	assertRetVal(myObj->Init(blob), false);

	*obj = myObj.Detach();
	return *obj != nullptr;
}

bool ff::CreateDataFromBlob(DirectX::Blob &&blob, IData **obj)
{
	assertRetVal(obj, false);
	*obj = nullptr;

	ComPtr<DataTexBlob> myObj = new ComObject<DataTexBlob>;
	assertRetVal(myObj->Init(std::move(blob)), false);

	*obj = myObj.Detach();
	return *obj != nullptr;
}

ff::DataBlob::DataBlob()
{
}

ff::DataBlob::~DataBlob()
{
}

bool ff::DataBlob::Init(ID3DBlob *blob)
{
	assertRetVal(blob, false);

	_blob = blob;

	return true;
}

const BYTE *ff::DataBlob::GetMem()
{
	return _blob ? (const BYTE *)_blob->GetBufferPointer() : 0;
}

size_t ff::DataBlob::GetSize()
{
	return _blob ? _blob->GetBufferSize() : 0;
}

ff::IDataFile *ff::DataBlob::GetFile()
{
	return nullptr;
}

bool ff::DataBlob::IsStatic()
{
	return false;
}

ff::DataTexBlob::DataTexBlob()
{
}

ff::DataTexBlob::~DataTexBlob()
{
}

bool ff::DataTexBlob::Init(DirectX::Blob &&blob)
{
	assertRetVal(blob.GetBufferPointer(), false);
	_blob = std::move(blob);
	return true;
}

const BYTE *ff::DataTexBlob::GetMem()
{
	return (const BYTE *)_blob.GetBufferPointer();
}

size_t ff::DataTexBlob::GetSize()
{
	return _blob.GetBufferSize();
}

ff::IDataFile *ff::DataTexBlob::GetFile()
{
	return nullptr;
}

bool ff::DataTexBlob::IsStatic()
{
	return false;
}
