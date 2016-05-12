#pragma once

namespace ff
{
#if !METRO_APP
	UTIL_API String LoadString(HINSTANCE hInstance, UINT nID); // from resource
	UTIL_API BSTR LoadBSTR(HINSTANCE hInstance, UINT nID); // never returns nullptr
#endif
	UTIL_API StringOut ReplaceAll(StringOut szText, StringRef szFind, StringRef szReplace);
	UTIL_API StringOut ReplaceAll(StringOut szText, wchar_t chFind, wchar_t chReplace);
	UTIL_API StringOut StripSpaces(StringOut szText, bool bStart = true, bool bEnd = true);
	UTIL_API Vector<String> SplitString(StringRef text, StringRef splitChars, bool bKeepEmpty);

	UTIL_API String GetDateAsString();
	UTIL_API String GetTimeAsString();

	UTIL_API String CanonicalizeString(StringRef text);
	UTIL_API void CanonicalizeStringInPlace(StringOut text);
	UTIL_API void LowerCaseInPlace(StringOut text);
	UTIL_API void UpperCaseInPlace(StringOut text);

	UTIL_API String StringFromBSTR(BSTR szText);
	UTIL_API String StringFromACP(const char *text, size_t len = INVALID_SIZE);
	UTIL_API String StringFromUTF8(const char *text, size_t len = INVALID_SIZE);
	UTIL_API BSTR StringToBSTR(StringRef text);
	UTIL_API Vector<char> StringToACP(StringRef text);

	UTIL_API String StringFromGuid(const GUID &guid);
	UTIL_API bool StringToGuid(StringRef text, GUID &guid); // returns false on error

	UTIL_API String GetExecutableDirectory(StringRef subDir = GetEmptyString());
	UTIL_API FARPROC GetProcAddress(HINSTANCE hInstance, StringRef proc);
	UTIL_API StringOut StripPathTail(StringOut szPath);
	UTIL_API StringOut AppendPathTail(StringOut szPath, StringRef tail);
	UTIL_API String GetPathTail(StringRef path);
	UTIL_API String GetPathExtension(StringRef path);
	UTIL_API StringOut ChangePathExtension(StringOut szPath, StringRef newExtension);
	// Removes "..", ".", double slashes, and trailing slashes
	// Prepends \\?\ or \\?\UNC\ if bSuperLong is true
	UTIL_API String CanonicalizePath(StringRef path, bool bSuperLong = false, bool bLowerCase = false);
	UTIL_API Vector<String> TokenizeCommandLine(const String *commandLine = nullptr);
	UTIL_API String GetCommandLine();

#if METRO_APP
	UTIL_API void SetCommandLineArgs(Platform::Array<Platform::String ^> ^args);
	UTIL_API Platform::Array<Platform::String ^> ^GetCommandLineArgs();
#else
	UTIL_API String GetModulePath(HINSTANCE hInstance);
	UTIL_API String GetExecutablePath();
	UTIL_API String GetExecutableName();
	UTIL_API String GetEnvironmentVariable(StringRef name);
	UTIL_API Map<String, String> GetEnvironmentVariables();
	UTIL_API String ExpandEnvironmentVariables(StringRef text);
#endif // !METRO_APP
}
