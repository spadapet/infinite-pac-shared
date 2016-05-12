#pragma once

namespace ff
{
	// IData: A way to pass around a blob of raw data in a ref-counted manner.
	class IData;
	class IDataFile;
	class IDataVector;

	UTIL_API bool CreateDataInMemMappedFile(IDataFile *pFile, IData **ppData);
	UTIL_API bool CreateDataInMemMappedFile(const BYTE *pMem, size_t nSize, IDataFile *pFile, IData **ppData);
	UTIL_API bool CreateDataInStaticMem(const BYTE *pMem, size_t nSize, IData **ppData);
	UTIL_API bool CreateDataInData(IData *pParentData, size_t nPos, size_t nSize, IData **ppChildData);
	UTIL_API bool CreateDataVector(size_t nInitialSize, IDataVector **ppData);
#if METRO_APP
	UTIL_API bool CreateDataFromBuffer(Windows::Storage::Streams::IBuffer ^buffer, IData **ppData);
#else
	UTIL_API bool CreateDataInResource(HINSTANCE hInstance, UINT id, IData **ppData);
	UTIL_API bool CreateDataInResource(HINSTANCE hInstance, LPCWSTR type, LPCWSTR name, IData **ppData);
#endif

	class __declspec(uuid("599c522b-dcd2-4a85-ae7f-73f2d0ee1794")) __declspec(novtable)
		IData : public IUnknown
	{
	public:
		virtual const BYTE *GetMem() = 0;
		virtual size_t GetSize() = 0;
		virtual IDataFile *GetFile() = 0;
		virtual bool IsStatic() = 0;
	};

	class __declspec(uuid("4eca1dd7-b78e-4864-ae61-d4bff8027c4d")) __declspec(novtable)
		IDataVector : public IData
	{
	public:
		virtual Vector<BYTE> &GetVector() = 0;
	};
}
