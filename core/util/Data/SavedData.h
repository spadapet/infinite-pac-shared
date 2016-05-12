#pragma once

namespace ff
{
	// ISavedData: A way to load or save data to or from any location.
	// The data always exists in one of two states:
	// LOADED: The data is fully loaded into memory (uncompressed).
	// SAVED: The data exists either on disk, in a resource, or just in memory.
	// When saved, the data is optionally compressed.
	// You can change the state of the data at any time.

	class IData;
	class IDataFile;
	class IDataReader;
	class IDataVector;
	class ISavedData;

	UTIL_API bool CreateLoadedDataFromMemory(
		IData *pData,
		bool bCompress,
		ISavedData **ppSavedData);

	UTIL_API bool CreateSavedDataFromMemory(
		IData *pData,
		size_t nFullSize,
		bool bCompressed,
		ISavedData **ppSavedData);

	UTIL_API bool CreateSavedDataFromFile(
		IDataFile *pFile,
		size_t nStart,
		size_t nSavedSize,
		size_t nFullSize,
		bool bCompressed,
		ISavedData **ppSavedData);

	class __declspec(uuid("04f35c9d-b5bb-4e42-8df5-3b13630e6516")) __declspec(novtable)
		ISavedData : public IUnknown
	{
	public:
		virtual IData *Load() = 0; // fully load into memory
		virtual bool Unload() = 0; // revert back to the original saved state
		virtual IData *SaveToMem() = 0; // copy into memory
		virtual bool SaveToFile() = 0; // copy any allocated memory to a file

		virtual size_t GetSavedSize() = 0;
		virtual size_t GetFullSize() = 0;
		virtual bool IsCompressed() = 0;
		virtual bool CreateSavedDataReader(IDataReader **ppReader) = 0;

		virtual bool Clone(ISavedData **ppSavedData) = 0;
		virtual bool Copy(ISavedData *pDataSource) = 0;
	};
}
