#pragma once

namespace ff
{
	class IData;
	class IDataReader;
	class IDataWriter;
	class String;

	UTIL_API bool SaveBytes(IDataWriter *pWriter, const void *pMem, size_t nBytes);
	UTIL_API bool SaveBytes(IDataWriter *pWriter, IData *pData);
	UTIL_API bool LoadBytes(IDataReader *pReader, void *pMem, size_t nBytes);
	UTIL_API bool LoadBytes(IDataReader *pReader, size_t nBytes, IData **ppData);
	UTIL_API const BYTE *LoadBytes(IDataReader *pReader, size_t nBytes);

	template<typename T>
	bool SaveData(IDataWriter *pWriter, const T &data)
	{
		return SaveBytes(pWriter, &data, sizeof(T));
	}

	template<typename T>
	bool LoadData(IDataReader *pReader, T &data)
	{
		return LoadBytes(pReader, &data, sizeof(T));
	}

	template<>
	UTIL_API bool SaveData<String>(IDataWriter *pWriter, StringRef data);

	template<>
	UTIL_API bool LoadData<String>(IDataReader *pReader, StringOut data);
}
