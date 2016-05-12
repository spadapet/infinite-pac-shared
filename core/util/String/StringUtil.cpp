#include "pch.h"
#include "Globals/ProcessGlobals.h"
#include "Module/Module.h"
#include "String/StringUtil.h"
#include "Windows/FileUtil.h"

#if !METRO_APP
ff::String ff::LoadString(HINSTANCE hInstance, UINT nID)
{
	String str;
	assertRetVal(hInstance, str);

	wchar_t buf[1024];
	assertRetVal(::LoadString(hInstance, nID, buf, _countof(buf)) != 0 || GetLastError() == NO_ERROR, str);
	str = buf;

	return str;
}

BSTR ff::LoadBSTR(HINSTANCE hInstance, UINT nID)
{
	BSTR ret = nullptr;
	wchar_t buf[1024];

	// cannot load from a nullptr instance
	assertRetVal(hInstance, SysAllocString(L""));

	if(::LoadStringW(hInstance, nID, buf, _countof(buf) - 1) != 0 ||
		GetLastError() == NO_ERROR)
	{
		ret = SysAllocString(buf);
	}
	else
	{
		assert(false); // bad string ID
		ret = SysAllocString(L""); // I don't want to return nullptr
	}

	assert(ret);
	return ret;
}
#endif

ff::StringOut ff::ReplaceAll(StringOut szText, StringRef szFind, StringRef szReplace)
{
	size_t pos = szText.find(szFind);
	size_t len = szFind.size();
	size_t newLen = szReplace.size();

	while (pos != INVALID_SIZE)
	{
		szText.replace(pos, len, szReplace);

		if (pos + newLen < szText.size())
		{
			pos = szText.find(szFind, pos + newLen);
		}
		else
		{
			pos = INVALID_SIZE;
		}
	}

	return szText;
}

ff::StringOut ff::ReplaceAll(StringOut szText, wchar_t chFind, wchar_t chReplace)
{
	for (auto ch = szText.begin(); ch != szText.end(); ch++)
	{
		if (*ch == chFind)
		{
			*ch = chReplace;
		}
	}

	return szText;
}

ff::StringOut ff::StripSpaces(StringOut szText, bool bStart, bool bEnd)
{
	const wchar_t *szSpaces = L" \t\r\n";
	size_t nFirstNonSpace = szText.find_first_not_of(szSpaces);

	if (nFirstNonSpace != INVALID_SIZE && nFirstNonSpace)
	{
		szText.erase(0, nFirstNonSpace);
	}

	size_t nLastNonSpace = szText.find_last_not_of(szSpaces);

	if (nLastNonSpace != INVALID_SIZE && nLastNonSpace + 1 < szText.size())
	{
		szText.erase(nLastNonSpace + 1);
	}

	if (szText.size() && wcschr(szSpaces, szText[0]))
	{
		// it was all spaces
		szText.clear();
	}

	return szText;
}

ff::Vector<ff::String> ff::SplitString(StringRef text, StringRef splitChars, bool bKeepEmpty)
{
	Vector<String> split;

	const wchar_t *cur = text.c_str();
	for (const wchar_t *start = cur; *cur; cur = *cur ? cur + 1 : cur, start = cur)
	{
		cur += wcscspn(cur, splitChars.c_str());

		if (cur > start || (*cur && bKeepEmpty))
		{
			split.Push(String(start, cur));
		}
	}

	return split;
}

ff::String ff::GetDateAsString()
{
	wchar_t str[128] = L"";
	SYSTEMTIME st;

	GetLocalTime(&st);
	_snwprintf_s(str, _countof(str), _TRUNCATE, L"%02d/%02d/%04d", st.wMonth, st.wDay, st.wYear);

	return String(str);
}

ff::String ff::GetTimeAsString()
{
	wchar_t str[128] = L"";
	SYSTEMTIME st;

	GetLocalTime(&st);
	_snwprintf_s(str, _countof(str), _TRUNCATE, L"%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);

	return String(str);
}

ff::String ff::CanonicalizeString(StringRef text)
{
	String ret(text);
	CanonicalizeStringInPlace(ret);
	return ret;
}

void ff::CanonicalizeStringInPlace(StringOut text)
{
	for (auto ch = text.begin(); ch != text.end(); ch++)
	{
		if (*ch >= 'A' && *ch <= 'Z')
		{
			*ch += 'a' - 'A';
		}
	}
}

void ff::LowerCaseInPlace(StringOut text)
{
	for (auto ch = text.begin(); ch != text.end(); ch++)
	{
		*ch = tolower(*ch);
	}
}

void ff::UpperCaseInPlace(StringOut text)
{
	for (auto ch = text.begin(); ch != text.end(); ch++)
	{
		*ch = toupper(*ch);
	}
}

ff::String ff::StringFromBSTR(BSTR szText)
{
	assertRetVal(szText, String());
	return String(szText);
}

ff::String ff::StringFromACP(const char *text, size_t len)
{
	String tstr;

	if (!text || !*text || !len)
	{
		return tstr;
	}

	int bytes = (len == INVALID_SIZE) ? (int)strlen(text) : (int)len;
	tstr.resize(MultiByteToWideChar(CP_ACP, 0, text, bytes, nullptr, 0));
	MultiByteToWideChar(CP_ACP, 0, text, bytes, &tstr[0], (int)tstr.size());

	return tstr;
}

ff::String ff::StringFromUTF8(const char *text, size_t len)
{
	if (!text || !*text || !len)
	{
		return String();
	}

	int bytes = (len == INVALID_SIZE) ? (int)strlen(text) : (int)len;

	Vector<wchar_t> wbuf;
	wbuf.Resize(MultiByteToWideChar(CP_UTF8, 0, text, bytes, nullptr, 0) + 1);

	MultiByteToWideChar(CP_UTF8, 0, text, bytes, &wbuf[0], (int)wbuf.Size());

	return String(wbuf.Data());
}

BSTR ff::StringToBSTR(StringRef text)
{
	return ::SysAllocStringLen(text.c_str(), (UINT)text.size());
}

ff::Vector<char> ff::StringToACP(StringRef text)
{
	int len = (int)text.size() + 1;
	Vector<char> acpText;
	acpText.Resize(WideCharToMultiByte(CP_ACP, 0, text.c_str(), len, nullptr, 0, nullptr, nullptr));
	WideCharToMultiByte(CP_ACP, 0, text.c_str(), len, acpText.Data(), (int)acpText.Size(), nullptr, nullptr);
	return acpText;
}

ff::String ff::StringFromGuid(const GUID &guid)
{
	String szRet;
	OLECHAR szGuid[50] = L"";

	if (SUCCEEDED(StringFromGUID2(guid, szGuid, 45)))
	{
		szRet = String(szGuid);
	}

	return szRet;
}

bool ff::StringToGuid(StringRef text, GUID &guid)
{
	if (FAILED(::IIDFromString(text.c_str(), &guid)))
	{
		ZeroObject(guid);
		return false;
	}

	return true;
}

ff::Vector<ff::String> ff::TokenizeCommandLine(const String *commandLine)
{
	Vector<String> tokens;

#if METRO_APP
	if (!commandLine)
	{
		for (Platform::String^ token: GetCommandLineArgs())
		{
			tokens.Push(String::from_pstring(token));
		}

		return tokens;
	}
#endif

	String token;
	wchar_t quote = 0;

	for (const wchar_t *sz = commandLine ? commandLine->c_str()
#if METRO_APP
			: nullptr;
#else
			: ::GetCommandLine();
#endif
		sz && *sz; sz++)
	{
		if (!quote)
		{
			if (iswspace(*sz))
			{
				// end of a token

				if (token.size())
				{
					tokens.Push(token);
					token.clear();
				}
			}
			else if (*sz == '\"' || *sz == '\'')
			{
				// start of a string
				quote = *sz;
			}
			else
			{
				token.append(sz, 1);
			}
		}
		else
		{
			// inside of a quoted string

			if (*sz == quote)
			{
				// the string has ended
				quote = 0;
			}
			else
			{
				token.append(sz, 1);
			}
		}
	}

	if (token.size())
	{
		// save the last token

		tokens.Push(token);
		token.clear();
	}

	return tokens;
}

ff::String ff::GetCommandLine()
{
#if METRO_APP
	String args;
	for (auto str: GetCommandLineArgs())
	{
		if (args.size())
		{
			args += L" ";
		}

		args += str->Data();
	}

	return args;
#else
	return String(::GetCommandLine());
#endif
}

#if METRO_APP

static Platform::Array<Platform::String ^ > ^s_commandLineArgs;

void ff::SetCommandLineArgs(Platform::Array<Platform::String ^> ^args)
{
	s_commandLineArgs = args;
}

Platform::Array<Platform::String ^> ^ff::GetCommandLineArgs()
{
	return s_commandLineArgs ? s_commandLineArgs : ref new Platform::Array<Platform::String^>(0);
}

#else

ff::String ff::GetModulePath(HINSTANCE hInstance)
{
	ff::Vector<wchar_t, 512> path;
	path.Resize(512);

	DWORD size = ::GetModuleFileName(hInstance, path.Data(), (DWORD)path.Size());

	while (size == path.Size() &&
		GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		path.Resize(path.Size() * 2);
		size = ::GetModuleFileName(hInstance, path.Data(), (DWORD)path.Size());
	}

	String szPath;

	if (size)
	{
		szPath = path.Data();
	}

	return szPath;
}
#endif // !METRO_APP

#if !METRO_APP
ff::String ff::GetExecutablePath()
{
	return GetModulePath(nullptr);
}

ff::String ff::GetExecutableName()
{
	ff::String name = ff::GetExecutablePath();
	name = ff::GetPathTail(name);

	size_t i = name.rfind('.');
	if (i != ff::INVALID_SIZE)
	{
		name.erase(i);
	}

	return name;
}
#endif // !METRO_APP

ff::String ff::GetExecutableDirectory(StringRef subDir)
{
	ff::String dir;

#if METRO_APP
	Windows::ApplicationModel::Package^ package = Windows::ApplicationModel::Package::Current;
	dir = String::from_pstring(package->InstalledLocation->Path);
#else
	dir = StripPathTail(GetExecutablePath());
#endif

	if (subDir.size())
	{
		ff::AppendPathTail(dir, subDir);
	}

	return dir;
}

FARPROC ff::GetProcAddress(HINSTANCE hInstance, StringRef proc)
{
	assertRetVal(hInstance && proc.size(), nullptr);

	CHAR szProcA[1024];

	int nLen = WideCharToMultiByte(CP_ACP, 0,
		proc.c_str(), -1,
		szProcA, 1024,
		nullptr, nullptr);

	assertRetVal(nLen && nLen < _countof(szProcA), nullptr);

	return ::GetProcAddress(hInstance, szProcA);
}

ff::StringOut ff::StripPathTail(StringOut szPath)
{
	bool bFoundSlash = false;

	for (auto ch = szPath.rbegin(); ch != szPath.rend(); ch++)
	{
		if (*ch == L'\\' || *ch == L'/')
		{
			bFoundSlash = true;

			szPath.resize(&*ch - szPath.c_str());
			break;
		}
	}

	if (!bFoundSlash)
	{
		szPath.clear();
	}

	return szPath;
}

ff::StringOut ff::AppendPathTail(StringOut szPath, StringRef tail)
{
	if (tail.size())
	{
		if (IsAbsolutePath(tail))
		{
			szPath = tail;
		}
		else if (IsDriveRelativePath(tail))
		{
			if (GetDriveLetter(tail))
			{
				szPath.erase(2);
				szPath += tail;
			}
			else
			{
				// not really clear what to do here
				szPath = tail;
			}
		}
		else
		{
			if (szPath.size() &&
				szPath[szPath.size() - 1] != L'\\' &&
				szPath[szPath.size() - 1] != L'/')
			{
				szPath += L"\\";
			}

			szPath += tail;
		}
	}

	return szPath;
}

ff::String ff::GetPathTail(StringRef path)
{
	for (const wchar_t *ch = path.c_str() + path.size() - 1; ch >= path.c_str(); ch--)
	{
		if (*ch == L'\\' || *ch == L'/')
		{
			return String(ch + 1);
		}
	}

	return path;
}

ff::String ff::GetPathExtension(StringRef path)
{
	String szTail = GetPathTail(path);

	for (auto ch = szTail.rbegin(); ch != szTail.rend(); ch++)
	{
		if (*ch == L'.')
		{
			return String(&*ch + 1);
		}
	}

	return String();
}

ff::StringOut ff::ChangePathExtension(StringOut szPath, StringRef newExtension)
{
	String szAppend;

	String useExtension = newExtension;
	if (useExtension.size() && useExtension[0] == L'.')
	{
		useExtension.erase(0, 1);
	}

	if (useExtension.size())
	{
		szAppend = L".";
		szAppend += useExtension;
	}

	for (auto ch = szPath.rbegin(); ch != szPath.rend(); ch++)
	{
		if (*ch == L'.')
		{
			szPath.erase(&*ch, &*szPath.end());
			break;
		}
		else if (*ch == L'\\' || *ch == L'/')
		{
			break;
		}
	}

	szPath += szAppend;

	return szPath;
}

ff::String ff::CanonicalizePath(StringRef path, bool bSuperLong, bool bLowerCase)
{
	if (path.empty())
	{
		return String();
	}

	if (!wcsncmp(path.c_str(), L"\\\\?\\", 4))
	{
		// already canonicalized
		String szCanon(path);

		if (bLowerCase)
		{
			CanonicalizeStringInPlace(szCanon);
		}

		return szCanon;
	}

	const wchar_t *szCur = path.c_str();

	while (iswspace(*szCur))
	{
		szCur++;
	}

	// initialize szCanon
	String szCanon;
	{
		// find the first non-slash
		const wchar_t *szFirst = _wcsspnp(szCur, L"\\/");

		if (!szFirst)
		{
			// all slashes, doesn't mean anything
			return String();
		}
		else if (szFirst - szCur >= 2)
		{
			// Either UNC or physical path

			if (szFirst[0] == '.' && szFirst[1] == '\\')
			{
				szCanon = L"\\\\.\\";
				szCur = _wcsspnp(szFirst + 2, L"\\/");
			}
			else
			{
				szCanon = bSuperLong ? L"\\\\?\\UNC\\" : L"\\\\";
				szCur = szFirst;
			}
		}
		else if (szFirst - szCur == 1)
		{
			// drive relative path
			szCanon = L"\\";
			szCur = szFirst;
		}
		else if (szFirst == szCur)
		{
			if (bSuperLong && isalpha(szFirst[0]) && szFirst[1] == ':')
			{
				// drive absolute path
				szCanon = L"\\\\?\\";
			}
		}
	}

	struct PartLocation
	{
		size_t nPos;
		size_t nLen;
	};

	Vector<PartLocation, 32> parts;

	// split up the parts of the path

	while (szCur && *szCur)
	{
		const wchar_t *szSlash = szCur + wcscspn(szCur, L"\\/");

		if (szSlash && *szSlash)
		{
			PartLocation part;
			part.nPos = szCur - path.c_str();
			part.nLen = szSlash - szCur;
			parts.Push(part);

			szCur = szSlash + 1;
			szCur = _wcsspnp(szCur, L"\\/"); // skip extra slashes
		}
		else
		{
			PartLocation part;
			part.nPos = szCur - path.c_str();
			part.nLen = wcslen(szCur);

			// strip trailing whitespace
			while (part.nLen && iswspace(path[part.nPos + part.nLen - 1]))
			{
				part.nLen--;
			}

			if (part.nLen)
			{
				parts.Push(part);
			}

			szCur += wcslen(szCur);
		}
	}

	// remove some parts if necessary

	for (size_t i = 0; i < parts.Size(); )
	{
		const PartLocation &part = parts[i];

		if (part.nLen == 1 && path[part.nPos] == '.')
		{
			parts.Delete(i, 1);
		}
		else if (i > 0 && part.nLen == 2 && !wcsncmp(path.c_str() + part.nPos, L"..", 2))
		{
			parts.Delete(i - 1, 2);
			i--;
		}
		else
		{
			i++;
		}
	}

	// combine the parts again

	for (size_t i = 0; i < parts.Size(); i++)
	{
		if (i > 0)
		{
			szCanon += L"\\";
		}

		const PartLocation &part = parts[i];

		szCanon.append(path.c_str() + part.nPos, part.nLen);
	}

	if (bLowerCase)
	{
		CanonicalizeStringInPlace(szCanon);
	}

	return szCanon;
}

#if !METRO_APP
ff::String ff::GetEnvironmentVariable(StringRef name)
{
	String szValue;
	ff::Vector<wchar_t, 512> value;
	value.Resize(512);

	DWORD size = ::GetEnvironmentVariable(name.c_str(), value.Data(), 512);

	if (size > 512) // try again
	{
		value.Resize(size);
		size = ::GetEnvironmentVariable(name.c_str(), value.Data(), size);
	}

	if (size)
	{
		szValue = value.Data();
	}

	return szValue;
}
#endif // !METRO_APP

#if !METRO_APP
ff::Map<ff::String, ff::String> ff::GetEnvironmentVariables()
{
	Map<String, String> dict;
	const wchar_t *szEnv = ::GetEnvironmentStrings();

	if (szEnv)
	{
		for (const wchar_t *szPair = szEnv; *szPair; szPair += wcslen(szPair) + 1)
		{
			const wchar_t *szEqual = wcschr(szPair, '=');
			assert(szEqual);

			if (szEqual && szEqual != szPair)
			{
				String szName;
				szName.assign(szPair, szEqual - szPair);

				String szValue(szEqual + 1);

				dict.SetKey(szName, szValue);
			}
		}

		::FreeEnvironmentStrings((wchar_t *)szEnv);
	}

	return dict;
}
#endif // !METRO_APP

#if !METRO_APP
ff::String ff::ExpandEnvironmentVariables(StringRef text)
{
	String szValue;
	ff::Vector<wchar_t, 512> value;
	value.Resize(512);

	DWORD size = ::ExpandEnvironmentStrings(text.c_str(), value.Data(), 512);

	if (size > 512) // try again
	{
		value.Resize(size);
		::ExpandEnvironmentStrings(text.c_str(), value.Data(), size);
	}

	if (size)
	{
		szValue = value.Data();
	}

	return szValue;
}
#endif // !METRO_APP
