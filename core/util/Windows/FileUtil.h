#pragma once

namespace ff
{
	class IData;
	class IDataWriter;

	class File
	{
	public:
		UTIL_API File();
		UTIL_API ~File();

		UTIL_API bool Open(StringRef path, bool bReadOnly, bool bAppend = false);
		UTIL_API bool OpenWrite(StringRef path, bool bAppend = false);
		UTIL_API bool OpenRead(StringRef path);
		UTIL_API void Close();

		UTIL_API operator HANDLE() const;

	private:
		HANDLE _file;
	};

	class MemMappedFile
	{
	public:
		UTIL_API MemMappedFile();
		UTIL_API ~MemMappedFile();

		UTIL_API bool Open(HANDLE hFile, bool bReadOnly);
		UTIL_API bool OpenWrite(HANDLE hFile, size_t nNewSize = INVALID_SIZE);
		UTIL_API bool OpenRead(HANDLE hFile);
		UTIL_API void Close();

		UTIL_API operator HANDLE() const;

		UTIL_API LPBYTE GetMem() const;
		UTIL_API size_t GetSize() const;

	private:
		HANDLE _mapping;
		LPBYTE _mem;
		size_t _size;
	};

#if !METRO_APP
	UTIL_API bool PlaceFile(StringRef pathFrom, StringRef pathTo, bool onlyNewer = true, bool allowCopyFallback = false);
	UTIL_API String GetShortPath(StringRef path);
	UTIL_API String GetLongPath(StringRef path);
	UTIL_API String GetOriginalCasePath(StringRef path);
	UTIL_API bool SetCurrentDirectory(StringRef path);
	UTIL_API String GetKnownDirectory(REFGUID id, bool bCreate = false);
#endif

	UTIL_API String GetLocalUserDirectory();
	UTIL_API String GetRoamingUserDirectory();
	UTIL_API String GetCurrentDirectory();
	UTIL_API bool CreateDirectory(StringRef path);
	UTIL_API bool DeleteDirectory(StringRef path, bool bMustBeEmpty = true);
	UTIL_API bool DirectoryExists(StringRef path, WIN32_FIND_DATA *pFindData = nullptr);
	UTIL_API bool GetDirectoryContents(StringRef path, Vector<String> &dirs, Vector<String> &files, Vector<FILETIME> *fileTimes = nullptr);

	UTIL_API String GetTempDirectory();
	UTIL_API String GetTempSubDirectory();
	UTIL_API void SetTempSubDirectory(StringRef name);
	UTIL_API String CreateTempFile(StringRef base = GetEmptyString(), StringRef extension = GetEmptyString());
	UTIL_API bool IsTempFile(StringRef path);
	UTIL_API void ClearTempSubDirectory();

	UTIL_API bool PathsEqual(StringRef path1, StringRef path2, bool bCheckShortPath = false);
	UTIL_API bool PathInPath(StringRef parent, StringRef child, bool bCheckShortPath = false);
	UTIL_API bool IsAbsolutePath(StringRef path);
	UTIL_API bool IsDriveRelativePath(StringRef path);
	UTIL_API wchar_t GetDriveLetter(StringRef path);

	UTIL_API bool FileExists(StringRef path, WIN32_FIND_DATA *pFindData = nullptr);
	UTIL_API bool FileReadable(StringRef path);
	UTIL_API bool DeleteFile(StringRef path);
	UTIL_API bool CopyFile(StringRef pathFrom, StringRef pathTo, bool bAllowOverwrite = true);
	UTIL_API bool MoveFile(StringRef pathFrom, StringRef pathTo, bool bAllowOverwrite = true);
	UTIL_API bool GetFileAttributes(StringRef path, DWORD &attribs);
	UTIL_API bool SetFileAttributes(StringRef path, DWORD attribs);
	UTIL_API FILETIME GetFileModifiedTime(StringRef path);
	UTIL_API FILETIME GetFileCreatedTime(StringRef path);
	UTIL_API FILETIME GetFileAccessedTime(StringRef path);
	UTIL_API size_t GetFileSize(StringRef path);
	UTIL_API size_t GetFileSize(HANDLE hFile);
	UTIL_API size_t GetFilePointer(HANDLE hFile);
	UTIL_API size_t SetFilePointer(HANDLE hFile, size_t pos);
	UTIL_API size_t SetFilePointerToEnd(HANDLE hFile);
	UTIL_API bool ReadFile(HANDLE hFile, size_t nBytes, Vector<BYTE> &outData);
	UTIL_API bool ReadFile(HANDLE hFile, size_t nBytes, void *pOutData);
	UTIL_API bool ReadFile(HANDLE hFile, size_t pos, size_t nBytes, Vector<BYTE> &outData);
	UTIL_API bool ReadFile(HANDLE hFile, size_t pos, size_t nBytes, void *pOutData);
	UTIL_API bool WriteFile(HANDLE hFile, LPCVOID pMem, size_t nBytes);
	UTIL_API bool WriteFile(HANDLE hFile, IData *data);
	UTIL_API bool WriteUnicodeBOMToFile(HANDLE hFile);
	UTIL_API bool WriteUnicodeBOM(IDataWriter *pWriter);
	UTIL_API bool ReadWholeFile(StringRef path, IData **ppData);
	UTIL_API bool ReadWholeFile(StringRef path, StringOut szOut);
	UTIL_API String CleanFileName(StringRef name);
	UTIL_API int CompareFileTime(const FILETIME &lhs, const FILETIME &rhs);
	UTIL_API HANDLE FindFirstChangeNotification(StringRef path, BOOL subDirs, DWORD filter);
}
