#pragma once

#if !METRO_APP

namespace ff
{
	// These return false on failure, but also set the value to the default value
	UTIL_API bool RegGetValue(HKEY key, StringRef name, DWORD &value, DWORD dwDefault = 0);
	UTIL_API bool RegGetValue(HKEY key, StringRef name, StringOut value, StringRef defaultValue = GetEmptyString());
	UTIL_API bool RegGetValue(HKEY key, StringRef name, GUID &guidValue, REFGUID guidDefault = GUID_NULL);

	UTIL_API bool RegSetValue(HKEY key, StringRef name, DWORD value);
	UTIL_API bool RegSetValue(HKEY key, StringRef name, StringRef value);
	UTIL_API bool RegSetValue(HKEY key, StringRef name, REFGUID guidValue);

	UTIL_API bool RegDeleteValue(HKEY key, StringRef name);
	UTIL_API bool RegDeleteKey(HKEY parentKey, StringRef keyName);

	UTIL_API bool RegEnumKey(HKEY key, DWORD nIndex, StringOut outName);
	UTIL_API bool RegEnumValue(HKEY key, DWORD nIndex, StringOut outName, LPDWORD pType = nullptr);

	class RegKey
	{
	public:
		UTIL_API RegKey();
		UTIL_API ~RegKey();

		UTIL_API operator HKEY() const;

		UTIL_API HKEY Create(HKEY parentKey, StringRef keyName);
		UTIL_API HKEY Open(HKEY parentKey, StringRef keyName, bool bReadOnly = false);
		UTIL_API void Close();

	private:
		HKEY _key;
	};
}

#endif // !METRO_APP
