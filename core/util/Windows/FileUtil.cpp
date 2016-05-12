#include "pch.h"
#include "Data/Data.h"
#include "Data/DataWriterReader.h"
#include "Globals/ProcessGlobals.h"
#include "String/StringUtil.h"
#include "Windows/FileUtil.h"

typedef ff::Vector<wchar_t, 512> StackCharVector512;

// STATIC_DATA (object)
static ff::String s_tempPath;
static ff::String s_oldTempSubName;
static ff::String s_tempSubName;
static bool s_initTempPathStatics;

// Must be called before setting any of the previous static Strings
static void SettingTempPath()
{
	if (!s_initTempPathStatics)
	{
		ff::LockMutex crit(ff::GCS_FILE_UTIL);

		if (!s_initTempPathStatics)
		{
			s_initTempPathStatics = true;

			ff::AtProgramShutdown([]()
			{
				ff::LockMutex crit(ff::GCS_FILE_UTIL);
				s_tempPath.clear();
				s_oldTempSubName.clear();
				s_tempSubName.clear();
				s_initTempPathStatics = false;
			});
		}
	}
}

// helper function for paths that come from Windows
static ff::StringOut StripSuperLongPrefix(ff::StringOut szPath)
{
	if (!wcsncmp(szPath.c_str(), L"\\\\?\\", 4))
	{
		szPath.erase(0, 4);

		if (!_wcsnicmp(szPath.c_str(), L"UNC\\", 4))
		{
			szPath.erase(0, 3);
			szPath.insert(0, L"\\");
		}
	}

	return szPath;
}

ff::File::File()
	: _file(nullptr)
{
}

ff::File::~File()
{
	Close();
}

ff::File::operator HANDLE() const
{
	return _file;
}

bool ff::File::Open(StringRef path, bool bReadOnly, bool bAppend)
{
	return bReadOnly
		? OpenRead(path)
		: OpenWrite(path, bAppend);
}

bool ff::File::OpenWrite(StringRef path, bool bAppend)
{
	String szCheckPath = CanonicalizePath(path, true);

	Close();

#if METRO_APP
	_file = ::CreateFile2(szCheckPath.c_str(),
		GENERIC_WRITE,
		FILE_SHARE_READ,
		bAppend ? OPEN_ALWAYS : CREATE_ALWAYS,
		nullptr);
#else
	_file = ::CreateFile(szCheckPath.c_str(),
		GENERIC_WRITE, FILE_SHARE_READ, nullptr,
		bAppend ? OPEN_ALWAYS : CREATE_ALWAYS,
		0, nullptr);
#endif

	if (_file == INVALID_HANDLE_VALUE)
	{
		_file = nullptr;
	}

	if (_file && bAppend)
	{
		SetFilePointerToEnd(_file);
	}

	return _file != nullptr;
}

bool ff::File::OpenRead(StringRef path)
{
	String szCheckPath = CanonicalizePath(path, true);

	Close();

#if METRO_APP
	_file = ::CreateFile2(szCheckPath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, OPEN_EXISTING, nullptr);
#else
	_file = ::CreateFile(szCheckPath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
#endif

	if (_file == INVALID_HANDLE_VALUE)
	{
		_file = nullptr;
	}

	return _file != nullptr;
}

void ff::File::Close()
{
	if (_file)
	{
		assert(_file != INVALID_HANDLE_VALUE);
		::CloseHandle(_file);
		_file = nullptr;
	}
}

ff::MemMappedFile::MemMappedFile()
	: _mapping(nullptr)
	, _mem(nullptr)
	, _size(0)
{
}

ff::MemMappedFile::~MemMappedFile()
{
	Close();
}

bool ff::MemMappedFile::Open(HANDLE hFile, bool bReadOnly)
{
	return bReadOnly
		? OpenRead(hFile)
		: OpenWrite(hFile);
}

bool ff::MemMappedFile::OpenWrite(HANDLE hFile, size_t nNewSize)
{
	Close();

	nNewSize = (nNewSize != INVALID_SIZE) ? nNewSize : ff::GetFileSize(hFile);

	LARGE_INTEGER newSizeLarge;
	newSizeLarge.QuadPart = nNewSize;

#if METRO_APP
	_mapping = ::CreateFileMappingFromApp(hFile, nullptr, PAGE_READWRITE, newSizeLarge.QuadPart, nullptr);
#else
	_mapping = ::CreateFileMapping(hFile, nullptr, PAGE_READWRITE, newSizeLarge.HighPart, newSizeLarge.LowPart, nullptr);
#endif
	assert(_mapping);

	if (!_mapping)
	{
		_size = 0;
		_mem = nullptr;
	}
	else
	{
		_size = nNewSize;

#if METRO_APP
		_mem = (LPBYTE)::MapViewOfFileFromApp(_mapping, FILE_MAP_WRITE, 0, _size);
#else
		_mem = (LPBYTE)::MapViewOfFile(_mapping, FILE_MAP_WRITE, 0, 0, _size);
#endif
		assert(_mem);
	}

	return _mem != nullptr;
}

bool ff::MemMappedFile::OpenRead(HANDLE hFile)
{
	Close();

	size_t nSize = ff::GetFileSize(hFile);
	if (!nSize)
	{
		return false;
	}

#if METRO_APP
	_mapping = ::CreateFileMappingFromApp(hFile, nullptr, PAGE_READONLY, 0, nullptr);
#else
	_mapping = ::CreateFileMapping(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
#endif
	assert(_mapping);

	if (!_mapping)
	{
		_size = 0;
		_mem = nullptr;
	}
	else
	{
		_size = nSize;

#if METRO_APP
		_mem = (LPBYTE)::MapViewOfFileFromApp(_mapping, FILE_MAP_READ, 0, _size);
#else
		_mem = (LPBYTE)::MapViewOfFile(_mapping, FILE_MAP_READ, 0, 0, _size);
#endif
		assert(_mem);
	}

	return _mem != nullptr;
}

void ff::MemMappedFile::Close()
{
	if (_mapping)
	{
		if (_mem)
		{
			::UnmapViewOfFile(_mem);
		}

		::CloseHandle(_mapping);

		_mapping = nullptr;
		_size = 0;
		_mem = nullptr;
	}
}

ff::MemMappedFile::operator HANDLE() const
{
	return _mapping;
}

LPBYTE ff::MemMappedFile::GetMem() const
{
	return _mapping ? _mem : nullptr;
}

size_t ff::MemMappedFile::GetSize() const
{
	return _mem ? _size : 0;
}

ff::String ff::GetCurrentDirectory()
{
#if METRO_APP
	return GetExecutableDirectory();
#else
	String szValue;

	StackCharVector512 value;
	value.Resize(512);

	DWORD size = ::GetCurrentDirectory((DWORD)value.Size(), value.Data());

	if (size > value.Size())
	{
		value.Resize(size);
		size = ::GetCurrentDirectory((DWORD)value.Size(), value.Data());
	}

	if (size)
	{
		szValue = value.Data();
		StripSuperLongPrefix(szValue);
	}

	return szValue;
#endif
}

#if !METRO_APP
bool ff::SetCurrentDirectory(StringRef path)
{
	String szRealPath = CanonicalizePath(path, true);

	return ::SetCurrentDirectory(szRealPath.c_str()) ? true : false;
}
#endif

// This is all fancy because it only creates one directory at a time,
// starting with the first missing subdirectory
bool ff::CreateDirectory(StringRef path)
{
	String szDrive;
	String szRealPath = CanonicalizePath(path);
	Vector<String> paths;

	if (szRealPath.empty())
	{
		return false;
	}
	else if (!wcsncmp(szRealPath.c_str(), L"\\\\.\\", 4))
	{
		// ignore physical paths
		return false;
	}
	else if (!wcsncmp(szRealPath.c_str(), L"\\\\", 2))
	{
		// UNC share, find the slash after the share name
		const wchar_t *szSlash = wcschr(szRealPath.c_str() + 2, '\\');
		szSlash = szSlash ? wcschr(szSlash + 1, '\\') : nullptr;

		if (!szSlash)
		{
			// can't create a server or share name
			return false;
		}

		szDrive = szRealPath.substr(0, szSlash - szRealPath.c_str() + 1);
		szRealPath.erase(0, szDrive.size());
	}
	else if (isalpha(szRealPath[0]) &&
		szRealPath[1] == ':' &&
		szRealPath[2] == '\\')
	{
		// absolute drive path
		szDrive = szRealPath.substr(0, 3);
		szRealPath.erase(0, 3);
	}
	else if (szRealPath[0] == '\\')
	{
		// relative drive path
		szDrive = L"\\";
		szRealPath.erase(0, 1);
	}
	else
	{
		// relative to the current directory
	}

	if (szRealPath.empty())
	{
		// nothing to create
		return false;
	}

	// Create the chain of parent directories
	{
		LockMutex crit(GCS_FILE_UTIL);

		for (; szRealPath.size() && !DirectoryExists(szDrive + szRealPath);
			StripPathTail(szRealPath))
		{
			paths.Push(szDrive + szRealPath);
		}

		for (size_t i = PreviousSize(paths.Size()); i != INVALID_SIZE; i = PreviousSize(i))
		{
			szRealPath = CanonicalizePath(paths[i], true);

			if (!::CreateDirectory(szRealPath.c_str(), nullptr))
			{
				assertRetVal(::GetLastError() == ERROR_ALREADY_EXISTS, false);
			}
		}
	}

	return true;
}

bool ff::DeleteDirectory(StringRef path, bool bMustBeEmpty)
{
	if (bMustBeEmpty)
	{
		String szRealPath = CanonicalizePath(path, true);

		return ::RemoveDirectory(szRealPath.c_str()) ? true : false;
	}
	else if (DirectoryExists(path))
	{
		Vector<String> dirs;
		Vector<String> files;

		if (GetDirectoryContents(path, dirs, files))
		{
			String szRealPath = CanonicalizePath(path, true);

			for (size_t i = 0; i < files.Size(); i++)
			{
				String szDelete = szRealPath;
				AppendPathTail(szDelete, files[i]);

				if (!DeleteFile(szDelete))
				{
					return false;
				}
			}

			for (size_t i = 0; i < dirs.Size(); i++)
			{
				String szDelete = szRealPath;
				AppendPathTail(szDelete, dirs[i]);

				if (!DeleteDirectory(szDelete, false))
				{
					return false;
				}
			}

			return DeleteDirectory(path, true);
		}
	}

	return false;
}

ff::String ff::GetTempDirectory()
{
	LockMutex crit(GCS_FILE_UTIL);

	if(!s_tempPath.empty() && s_oldTempSubName == s_tempSubName)
	{
		return s_tempPath;
	}

	SettingTempPath();

	s_tempPath.clear();
	s_oldTempSubName = s_tempSubName;

#if METRO_APP
	s_tempPath = String::from_pstring(Windows::Storage::ApplicationData::Current->TemporaryFolder->Path);
#else
	StackCharVector512 stackPath;
	stackPath.Resize(512);

	DWORD size = ::GetTempPath((DWORD)stackPath.Size(), stackPath.Data());

	if (size > stackPath.Size())
	{
		stackPath.Resize(size);
		size = ::GetTempPath((DWORD)stackPath.Size(), stackPath.Data());
	}

	if (size)
	{
		s_tempPath = CanonicalizePath(String(stackPath.Data()));
		StripSuperLongPrefix(s_tempPath);

		static StaticString defaultTail(L"Ferret Face");
		AppendPathTail(s_tempPath, defaultTail);
	}
#endif

	if (!s_tempPath.empty())
	{
		AppendPathTail(s_tempPath, s_tempSubName);

		if (!DirectoryExists(s_tempPath))
		{
			CreateDirectory(s_tempPath);
		}
	}

	return s_tempPath;
}

ff::String ff::GetTempSubDirectory()
{
	LockMutex crit(GCS_FILE_UTIL);

	return s_tempSubName;
}

void ff::SetTempSubDirectory(StringRef name)
{
	SettingTempPath();

	LockMutex crit(GCS_FILE_UTIL);

	s_tempSubName = CanonicalizePath(name);
}

ff::String ff::CreateTempFile(StringRef base, StringRef extension)
{
	LockMutex crit(GCS_FILE_UTIL);

	// STATIC_DATA (pod)
	static UINT nUnique = 0;
	const int maxTries = 1000;
	String szFinal;

	static StaticString defaultBase(L"TempFile");
	static StaticString defaultExt(L"tmp");

	String szBase = base.size() ? szBase : defaultBase;
	String szExtension = extension.size() ? szExtension : defaultExt;

	for (int nTry = 0; nTry < maxTries; nTry++, nUnique++)
	{
		wchar_t szUnique[20];
		_snwprintf_s(szUnique, _countof(szUnique), _TRUNCATE, L"%u", nUnique);

		ff::String szTry = GetTempDirectory();
		AppendPathTail(szTry, szBase);
		szTry += szUnique;
		szTry += L".";
		szTry += szExtension;

		if (!FileExists(szTry))
		{
			szFinal = szTry;
			break;
		}
	}

	if (szFinal.size())
	{
		nUnique++;

		// Create the file to reserve it
		File file;
		if (!file.OpenWrite(szFinal))
		{
			szFinal.empty();
		}
	}

	assertSz(szFinal.size(), L"Can't create a file in the temp folder");

	return szFinal;
}

bool ff::IsTempFile(StringRef path)
{
	return ff::PathInPath(ff::GetTempDirectory(), path);
}

void ff::ClearTempSubDirectory()
{
#if !METRO_APP
	if (GetTempSubDirectory().empty())
	{
		// don't clear the root temp path
		return;
	}
#endif

	ff::String szFilter = ff::GetTempDirectory();
	AppendPathTail(szFilter, String(L"*"));

	WIN32_FIND_DATA data;
	ZeroObject(data);

	HANDLE hFind = FindFirstFileEx(szFilter.c_str(), FindExInfoStandard, &data, FindExSearchNameMatch, nullptr, 0);

	for (BOOL bDone = FALSE; !bDone && hFind != INVALID_HANDLE_VALUE; bDone = !FindNextFile(hFind, &data))
	{
		if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			ff::String szFile = ff::GetTempDirectory();
			AppendPathTail(szFile, String(data.cFileName));

			DeleteFile(szFile);
		}
	}

	if (hFind != INVALID_HANDLE_VALUE)
	{
		FindClose(hFind);
	}
}

bool ff::FileExists(StringRef path, WIN32_FIND_DATA *pFindData)
{
	String szCheckPath = CanonicalizePath(path, true);

	if (szCheckPath.size())
	{
		WIN32_FIND_DATA data;
		pFindData = pFindData ? pFindData : &data;

		HANDLE hFind = FindFirstFileEx(szCheckPath.c_str(), FindExInfoStandard, pFindData, FindExSearchNameMatch, nullptr, 0);

		if (hFind != INVALID_HANDLE_VALUE)
		{
			FindClose(hFind);
			return !(pFindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
		}
	}

	return false;
}

bool ff::FileReadable(StringRef path)
{
	File file;
	return file.OpenRead(path);
}

bool ff::DeleteFile(StringRef path)
{
	if (FileExists(path))
	{
		String szCheckPath = CanonicalizePath(path, true);

		return ::DeleteFile(szCheckPath.c_str()) ? true : false;
	}

	return false;
}

bool ff::CopyFile(StringRef pathFrom, StringRef pathTo, bool bAllowOverwrite)
{
	String szCheckPathFrom = CanonicalizePath(pathFrom, true);
	String szCheckPathTo = CanonicalizePath(pathTo, true);

#if METRO_APP
	BOOL bCancel = FALSE;
	COPYFILE2_EXTENDED_PARAMETERS cp;
	ZeroObject(cp);
	cp.dwSize = sizeof(cp);
	cp.dwCopyFlags = bAllowOverwrite ? 0 : COPY_FILE_FAIL_IF_EXISTS;
	cp.pfCancel = &bCancel;

	return SUCCEEDED(::CopyFile2(szCheckPathFrom.c_str(), szCheckPathTo.c_str(), &cp)) ? true : false;
#else
	return ::CopyFile(szCheckPathFrom.c_str(), szCheckPathTo.c_str(), !bAllowOverwrite) ? true : false;
#endif
}

bool ff::MoveFile(StringRef pathFrom, StringRef pathTo, bool bAllowOverwrite)
{
	String szCheckPathFrom = CanonicalizePath(pathFrom, true);
	String szCheckPathTo = CanonicalizePath(pathTo, true);

	return ::MoveFileEx(
		szCheckPathFrom.c_str(),
		szCheckPathTo.c_str(),
		MOVEFILE_COPY_ALLOWED | (bAllowOverwrite ? MOVEFILE_REPLACE_EXISTING : 0)) ? true : false;
}

#if !METRO_APP
// Creates a hard-link between two files, but only if the destination
// is older than the source.
bool ff::PlaceFile(StringRef pathFrom, StringRef pathTo, bool onlyNewer, bool allowCopyFallback)
{
	String szCheckPathFrom = ff::CanonicalizePath(pathFrom, true);
	String szCheckPathTo = ff::CanonicalizePath(pathTo, true);

	WIN32_FIND_DATA srcData;
	WIN32_FIND_DATA destData;

	if (!ff::FileExists(pathFrom, &srcData))
	{
		return false;
	}

	if (ff::FileExists(pathTo, &destData))
	{
		// Either keep the dest, or delete it

		if (onlyNewer && ff::CompareFileTime(srcData.ftLastWriteTime, destData.ftLastWriteTime) <= 0)
		{
			return true;
		}

		if (!ff::DeleteFile(pathTo))
		{
			return false;
		}
	}
	else
	{
		// Ensure that the dest directory exists

		String szDestDir(pathTo);
		ff::StripPathTail(szDestDir);

		if (!ff::DirectoryExists(szDestDir))
		{
			if (!ff::CreateDirectory(szDestDir))
			{
				return false;
			}
		}
	}

	// Everything is set for a hard link to be created

	if (!::CreateHardLink(szCheckPathTo.c_str(), szCheckPathFrom.c_str(), nullptr))
	{
		if (!allowCopyFallback || !ff::CopyFile(szCheckPathFrom, szCheckPathTo))
		{
			return false;
		}
	}

	return true;
}
#endif

#if !METRO_APP
ff::String ff::GetShortPath(StringRef path)
{
	String szCheckPath = CanonicalizePath(path, true);

	StackCharVector512 stackPath;
	stackPath.Resize(512);

	DWORD size = ::GetShortPathName(szCheckPath.c_str(), stackPath.Data(), (DWORD)stackPath.Size());

	if (size > stackPath.Size())
	{
		stackPath.Resize(size);

		size = ::GetShortPathName(szCheckPath.c_str(), stackPath.Data(), (DWORD)stackPath.Size());
	}

	if (size)
	{
		szCheckPath = stackPath.Data();
		StripSuperLongPrefix(szCheckPath);
	}
	else
	{
		szCheckPath.empty();
	}

	return szCheckPath;
}
#endif

#if !METRO_APP
ff::String ff::GetLongPath(StringRef path)
{
	String szCheckPath = CanonicalizePath(path, true);

	StackCharVector512 stackPath;
	stackPath.Resize(512);

	DWORD size = ::GetLongPathName(szCheckPath.c_str(), stackPath.Data(), (DWORD)stackPath.Size());

	if (size > stackPath.Size())
	{
		stackPath.Resize(size);

		size = ::GetLongPathName(szCheckPath.c_str(), stackPath.Data(), (DWORD)stackPath.Size());
	}

	if (size)
	{
		szCheckPath = stackPath.Data();
		StripSuperLongPrefix(szCheckPath);
	}
	else
	{
		szCheckPath.empty();
	}

	return szCheckPath;
}
#endif

#if !METRO_APP
ff::String ff::GetOriginalCasePath(StringRef path)
{
	String sz = GetShortPath(path);
	return GetLongPath(sz);
}
#endif

bool ff::GetFileAttributes(StringRef path, DWORD &attribs)
{
	String szCheckPath = CanonicalizePath(path, true);

	WIN32_FILE_ATTRIBUTE_DATA data;
	attribs = ::GetFileAttributesEx(szCheckPath.c_str(), GetFileExInfoStandard, &data)
		? data.dwFileAttributes
		: INVALID_FILE_ATTRIBUTES;

	return attribs != INVALID_FILE_ATTRIBUTES;
}

bool ff::SetFileAttributes(StringRef path, DWORD attribs)
{
	String szCheckPath = CanonicalizePath(path, true);

	return ::SetFileAttributes(szCheckPath.c_str(), attribs) ? true : false;
}

size_t ff::GetFileSize(StringRef path)
{
	WIN32_FIND_DATA data;

	if (FileExists(path, &data))
	{
#ifdef _WIN64
		return ((size_t)data.nFileSizeHigh << 32) | (size_t)data.nFileSizeLow;
#else
		if (data.nFileSizeHigh)
		{
			// too large for the return value
			assert(false);
			return 0;
		}
		else
		{
			return data.nFileSizeLow;
		}
#endif
	}

	return 0;
}

size_t ff::GetFileSize(HANDLE hFile)
{
	assertRetVal(hFile, 0);

	FILE_STANDARD_INFO info;
	if (GetFileInformationByHandleEx(hFile, FileStandardInfo, &info, sizeof(info)))
	{
#ifdef _WIN64
		return info.EndOfFile.QuadPart;
#else
		assert(!info.EndOfFile.HighPart);
		return info.EndOfFile.HighPart ? 0 : info.EndOfFile.LowPart;
#endif
	}
	
	assert(false);
	return 0;
}

FILETIME ff::GetFileModifiedTime(StringRef path)
{
	WIN32_FIND_DATA data;

	if (FileExists(path, &data))
	{
		return data.ftLastWriteTime;
	}

	assertSz(false, L"File doesn't exist");

	FILETIME dtm = { 0, 0 };
	return dtm;
}

FILETIME ff::GetFileCreatedTime(StringRef path)
{
	WIN32_FIND_DATA data;

	if (FileExists(path, &data))
	{
		return data.ftCreationTime;
	}

	assertSz(false, L"File doesn't exist");

	FILETIME dtm = { 0, 0 };
	return dtm;
}

FILETIME ff::GetFileAccessedTime(StringRef path)
{
	WIN32_FIND_DATA data;

	if (FileExists(path, &data))
	{
		return data.ftLastAccessTime;
	}

	assertSz(false, L"File doesn't exist");

	FILETIME dtm = { 0, 0 };
	return dtm;
}

bool ff::DirectoryExists(StringRef path, WIN32_FIND_DATA *pFindData)
{
	String szCheckPath = CanonicalizePath(path, true);

	WIN32_FIND_DATA data;
	pFindData = pFindData ? pFindData : &data;

	HANDLE hFind = FindFirstFileEx(szCheckPath.c_str(), FindExInfoStandard, pFindData, FindExSearchLimitToDirectories, nullptr, 0);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		FindClose(hFind);
		return (pFindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
	}

	return false;
}

bool ff::GetDirectoryContents(StringRef path, Vector<String> &dirs, Vector<String> &files, Vector<FILETIME> *fileTimes)
{
	dirs.Clear();
	files.Clear();
	if (fileTimes)
	{
		fileTimes->Clear();
	}

	String szFilter = CanonicalizePath(path, true);
	assertRetVal(szFilter.size(), false);
	AppendPathTail(szFilter, String(L"*"));

	WIN32_FIND_DATA data;
	ZeroObject(data);

	HANDLE hFind = FindFirstFileEx(szFilter.c_str(), FindExInfoStandard, &data, FindExSearchNameMatch, nullptr, 0);

	for (BOOL bDone = FALSE; !bDone && hFind != INVALID_HANDLE_VALUE; bDone = !FindNextFile(hFind, &data))
	{
		if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			files.Push(String(data.cFileName));

			if (fileTimes)
			{
				fileTimes->Push(data.ftLastWriteTime);
			}
		}
		else if (wcscmp(data.cFileName, L".") && wcscmp(data.cFileName, L".."))
		{
			dirs.Push(String(data.cFileName));
		}
	}

	if (hFind != INVALID_HANDLE_VALUE)
	{
		FindClose(hFind);
	}

	return hFind != INVALID_HANDLE_VALUE;
}

ff::String ff::GetLocalUserDirectory()
{
#if METRO_APP
	return String::from_pstring(Windows::Storage::ApplicationData::Current->LocalFolder->Path);
#else
	return GetKnownDirectory(FOLDERID_LocalAppData, true);
#endif
}

ff::String ff::GetRoamingUserDirectory()
{
#if METRO_APP
	return String::from_pstring(Windows::Storage::ApplicationData::Current->RoamingFolder->Path);
#else
	return GetKnownDirectory(FOLDERID_RoamingAppData, true);
#endif
}

#if !METRO_APP
ff::String ff::GetKnownDirectory(REFGUID id, bool bCreate)
{
	String szReturnPath;
	LPWSTR szPath = nullptr;

	assertHrRetVal(SHGetKnownFolderPath(id, bCreate ? KF_FLAG_CREATE : 0, nullptr, &szPath), szReturnPath);

	szReturnPath = szPath;
	CoTaskMemFree(szPath);

	return szReturnPath;
}
#endif

bool ff::PathsEqual(StringRef path1, StringRef path2, bool bCheckShortPath)
{
	String szCheckPath1 = CanonicalizePath(path1, false, true);
	String szCheckPath2 = CanonicalizePath(path2, false, true);

#if !METRO_APP
	if (bCheckShortPath && szCheckPath1 != szCheckPath2)
	{
		szCheckPath1 = GetShortPath(path1);
		szCheckPath2 = GetShortPath(path2);
	}
#endif

	return szCheckPath1 == szCheckPath2;
}

bool ff::PathInPath(StringRef parent, StringRef child, bool bCheckShortPath)
{
#if !METRO_APP
	String szCheckParent = bCheckShortPath ? GetShortPath(parent) : CanonicalizePath(parent, false, true);
	String szCheckChild = bCheckShortPath ? GetShortPath(child) : CanonicalizePath(child, false, true);
#else
	String szCheckParent = CanonicalizePath(parent, false, true);
	String szCheckChild = CanonicalizePath(child, false, true);
#endif

	if (szCheckChild.size() > szCheckParent.size() &&
		szCheckChild[szCheckParent.size()] == '\\' &&
		!_wcsnicmp(szCheckChild.c_str(), szCheckParent.c_str(), szCheckParent.size()))
	{
		return true;
	}

	return false;
}

bool ff::IsAbsolutePath(StringRef path)
{
	return path.size() >= 2 &&
		isalpha(path[0]) &&
		path[1] == ':' &&
		(path.size() == 2 || path[2] == '/' || path[2] == '\\');
}

bool ff::IsDriveRelativePath(StringRef path)
{
	return path.size() && (path[0] == '\\' || path[0] == '/');
}

wchar_t ff::GetDriveLetter(StringRef path)
{
	return IsAbsolutePath(path) ? path[0] : 0;
}

size_t ff::GetFilePointer(HANDLE hFile)
{
	assertRetVal(hFile, INVALID_SIZE);

	LARGE_INTEGER cur;
	LARGE_INTEGER move;
	move.QuadPart = 0;

	assertRetVal(::SetFilePointerEx(hFile, move, &cur, FILE_CURRENT), INVALID_SIZE);

#ifdef _WIN64
	return cur.QuadPart;
#else
	assertRetVal(!cur.HighPart, INVALID_SIZE);
	return cur.LowPart;
#endif
}

size_t ff::SetFilePointer(HANDLE hFile, size_t pos)
{
	assertRetVal(hFile, INVALID_SIZE);

	LARGE_INTEGER cur;
	LARGE_INTEGER move;
	move.QuadPart = pos;

	assertRetVal(::SetFilePointerEx(hFile, move, &cur, FILE_BEGIN), INVALID_SIZE);

#ifdef _WIN64
	return cur.QuadPart;
#else
	assertRetVal(!cur.HighPart, INVALID_SIZE);
	return cur.LowPart;
#endif
}

size_t ff::SetFilePointerToEnd(HANDLE hFile)
{
	assertRetVal(hFile, INVALID_SIZE);

	LARGE_INTEGER cur;
	LARGE_INTEGER move;
	move.QuadPart = 0;

	assertRetVal(::SetFilePointerEx(hFile, move, &cur, FILE_END), INVALID_SIZE);

#ifdef _WIN64
	return cur.QuadPart;
#else
	assertRetVal(!cur.HighPart, INVALID_SIZE);
	return cur.LowPart;
#endif
}

bool ff::ReadFile(HANDLE hFile, size_t nBytes, Vector<BYTE> &outData)
{
	assertRetVal(hFile, false);

	if (!nBytes)
	{
		outData.Clear();
		return true;
	}

	outData.Resize(nBytes);
	if (outData.Size() != nBytes)
	{
		return false;
	}

	DWORD nRead = 0;

	return ::ReadFile(hFile, outData.Data(), (DWORD)nBytes, &nRead, nullptr) &&
		(size_t)nRead == nBytes;
}

bool ff::ReadFile(HANDLE hFile, size_t nBytes, void *pOutData)
{
	assertRetVal(hFile && pOutData, false);

	if (!nBytes)
	{
		return true;
	}

	DWORD nRead = 0;

	return ::ReadFile(hFile, pOutData, (DWORD)nBytes, &nRead, nullptr) &&
		(size_t)nRead == nBytes;
}

bool ff::ReadFile(HANDLE hFile, size_t pos, size_t nBytes, Vector<BYTE> &outData)
{
	return SetFilePointer(hFile, pos) == pos && ReadFile(hFile, nBytes, outData);
}

bool ff::ReadFile(HANDLE hFile, size_t pos, size_t nBytes, void *pOutData)
{
	return SetFilePointer(hFile, pos) == pos && ReadFile(hFile, nBytes, pOutData);
}

bool ff::WriteFile(HANDLE hFile, LPCVOID pMem, size_t nBytes)
{
	assertRetVal(hFile && pMem, false);

	if (!nBytes)
	{
		return true;
	}

	DWORD nWritten = 0;

	return ::WriteFile(hFile, pMem, (DWORD)nBytes, &nWritten, nullptr) &&
		(size_t)nWritten == nBytes;
}

bool ff::WriteFile(HANDLE hFile, IData *data)
{
	assertRetVal(data, false);
	return ff::WriteFile(hFile, data->GetMem(), data->GetSize());
}

bool ff::WriteUnicodeBOMToFile(HANDLE hFile)
{
	assertRetVal(hFile, false);

	WORD bom = 0xFEFF;

	return WriteFile(hFile, &bom, sizeof(bom));
}

bool ff::WriteUnicodeBOM(IDataWriter *pWriter)
{
	assertRetVal(pWriter, false);

	WORD bom = 0xFEFF;

	return pWriter->Write(&bom, sizeof(bom));
}

bool ff::ReadWholeFile(StringRef path, IData **ppData)
{
	assertRetVal(ppData, false);
	*ppData = nullptr;

	File file;
	if (!file.OpenRead(path))
	{
		return false;
	}

	size_t nSize = GetFileSize(file);

	ComPtr<IDataVector> pData;
	assertRetVal(CreateDataVector(nSize, &pData), false);
	assertRetVal(pData->GetSize() == nSize, false);

	assertRetVal(ReadFile(file, nSize, pData->GetVector().Data()), false);

	*ppData = pData.Detach();
	return true;
}

bool ff::ReadWholeFile(StringRef path, StringOut szOut)
{
	szOut.clear();

	File file;
	if (!file.OpenRead(path))
	{
		return false;
	}

	MemMappedFile memFile;
	if (!memFile.OpenRead(file))
	{
		return false;
	}

	const BYTE *pMem = memFile.GetMem();

	if (memFile.GetSize() >= 3 &&
		pMem[0] == 0xEF &&
		pMem[1] == 0xBB &&
		pMem[2] == 0xBF)
	{
		// UTF-8

		szOut = StringFromUTF8((const char *)(pMem + 3), memFile.GetSize() - 3);
	}
	else if (memFile.GetSize() >= 2 &&
		pMem[0] == 0xFF &&
		pMem[1] == 0xFE)
	{
		// UTF-16 little endian

		szOut = String((const wchar_t *)(pMem + 2), memFile.GetSize() - 2);
	}
	// ignore UTF-16 big endian and UTF-32 little/big endian
	else
	{
		// assume ACP

		szOut = StringFromACP((const char *)memFile.GetMem(), memFile.GetSize());
	}

	return true;
}

ff::String ff::CleanFileName(StringRef name)
{
	String szNewName(name);

	for (size_t i = PreviousSize(szNewName.length()); i != INVALID_SIZE; i = PreviousSize(i))
	{
		wchar_t ch = szNewName[i];

		if (ch == '.' && (i == szNewName.length() - 1 || (i > 0 && szNewName[i - 1] == '.')))
		{
			// Don't allow more than one period in a row,
			// and don't allow a period at the end
			szNewName.erase(i, 1);
		}
		else if (ch < 32 || ch > 125)
		{
			szNewName[i] = '-';
		}
		else switch (ch)
		{
		case '<': szNewName[i] = '('; break;
		case '>': szNewName[i] = ')'; break;
		case ':': szNewName[i] = '-'; break;
		case '\"': szNewName[i] = '\''; break;
		case '/': szNewName[i] = '-'; break;
		case '\\': szNewName[i] = '-'; break;
		case '|': szNewName[i] = '-'; break;
		case '?': szNewName[i] = '-'; break;
		case '*': szNewName[i] = '-'; break;
		}
	}

	String szExt = GetPathExtension(szNewName);
	ChangePathExtension(szNewName, GetEmptyString());

	if (!_wcsicmp(szNewName.c_str(), L"CON") ||
		!_wcsicmp(szNewName.c_str(), L"PRN") ||
		!_wcsicmp(szNewName.c_str(), L"AUX") ||
		!_wcsicmp(szNewName.c_str(), L"NUL") ||
		!_wcsicmp(szNewName.c_str(), L"COM1") ||
		!_wcsicmp(szNewName.c_str(), L"COM2") ||
		!_wcsicmp(szNewName.c_str(), L"COM3") ||
		!_wcsicmp(szNewName.c_str(), L"COM4") ||
		!_wcsicmp(szNewName.c_str(), L"COM5") ||
		!_wcsicmp(szNewName.c_str(), L"COM6") ||
		!_wcsicmp(szNewName.c_str(), L"COM7") ||
		!_wcsicmp(szNewName.c_str(), L"COM8") ||
		!_wcsicmp(szNewName.c_str(), L"COM9") ||
		!_wcsicmp(szNewName.c_str(), L"LPT1") ||
		!_wcsicmp(szNewName.c_str(), L"LPT2") ||
		!_wcsicmp(szNewName.c_str(), L"LPT3") ||
		!_wcsicmp(szNewName.c_str(), L"LPT4") ||
		!_wcsicmp(szNewName.c_str(), L"LPT5") ||
		!_wcsicmp(szNewName.c_str(), L"LPT6") ||
		!_wcsicmp(szNewName.c_str(), L"LPT7") ||
		!_wcsicmp(szNewName.c_str(), L"LPT8") ||
		!_wcsicmp(szNewName.c_str(), L"LPT9"))
	{
		szNewName += L"-File";
	}

	ChangePathExtension(szNewName, szExt);

	return szNewName;
}

int ff::CompareFileTime(const FILETIME &lhs, const FILETIME &rhs)
{
#if METRO_APP
	if (lhs.dwHighDateTime < rhs.dwHighDateTime)
	{
		return -1;
	}
	else if (lhs.dwHighDateTime > rhs.dwHighDateTime)
	{
		return 1;
	}
	else if (lhs.dwLowDateTime < rhs.dwLowDateTime)
	{
		return -1;
	}
	else if (lhs.dwLowDateTime > rhs.dwLowDateTime)
	{
		return 1;
	}

	return 0;
#else
	return ::CompareFileTime(&lhs, &rhs);
#endif
}

#if !METRO_APP
HANDLE ff::FindFirstChangeNotification(StringRef path, BOOL subDirs, DWORD filter)
{
	ff::String canon = CanonicalizePath(path, true);
	return ::FindFirstChangeNotification(canon.c_str(), subDirs, filter);
}
#endif
