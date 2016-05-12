#pragma once

namespace ff
{
	class IData;
	class IDataFile;
	class IDataReader;
	class IDataVector;
	class IDataWriter;
	class ISavedData;

	UTIL_API bool CreateDataWriter(IDataVector *pData, size_t nPos, IDataWriter **ppWriter);
	UTIL_API bool CreateDataWriter(IDataVector **ppData, IDataWriter **ppWriter); // shortcut
	UTIL_API bool CreateDataWriter(IDataFile *pFile, size_t nPos, IDataWriter **ppWriter); // Use INVALID_SIZE to create a new file

	UTIL_API bool CreateDataReader(IData *pData, size_t nPos, IDataReader **ppReader);
	UTIL_API bool CreateDataReader(const BYTE *pMem, size_t nLen, size_t nPos, IDataReader **ppReader); // shortcut
	UTIL_API bool CreateDataReader(IDataFile *pFile, size_t nPos, IDataReader **ppReader);
#if METRO_APP
	UTIL_API bool CreateDataReader(Windows::Storage::Streams::IRandomAccessStream ^stream, IDataReader **ppReader);
	UTIL_API Windows::Storage::Streams::IRandomAccessStream ^GetRandomAccessStream(IDataReader *reader);
#endif

	UTIL_API bool StreamCopyData(IDataReader *pReader, size_t nSize, IDataWriter *pWriter, size_t nChunkSize = INVALID_SIZE);

	class __declspec(uuid("9eed88fb-f90c-4ab7-883e-20425e4e6345")) __declspec(novtable)
		IDataStream : public IUnknown
	{
	public:
		virtual size_t GetSize() const = 0;
		virtual size_t GetPos() const = 0;
		virtual bool SetPos(size_t nPos) = 0;

		virtual bool CreateSavedData(
			size_t start,
			size_t savedSize,
			size_t fullSize,
			bool compressed,
			ISavedData **obj) = 0;
	};

	class __declspec(uuid("55bb2747-eb40-47be-8523-75488d050d99")) __declspec(novtable)
		IDataWriter : public IDataStream
	{
	public:
		virtual bool Write(LPCVOID pMem, size_t nBytes) = 0;
	};

	class __declspec(uuid("ac9c2fd0-3e8d-4e0c-acdb-9b47487e85d8")) __declspec(novtable)
		IDataReader : public IDataStream
	{
	public:
		virtual const BYTE *Read(size_t nBytes) = 0;
		virtual bool Read(size_t nBytes, IData **ppData) = 0;
		virtual bool Read(size_t nStart, size_t nBytes, IData **ppData) = 0;
	};
}
