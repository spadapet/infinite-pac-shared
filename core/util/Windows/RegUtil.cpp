#include "pch.h"
#include "String/StringUtil.h"
#include "Windows/RegUtil.h"

#if !METRO_APP

typedef ff::Vector<wchar_t, 512> StackCharVector512;

bool ff::RegGetValue(HKEY key, StringRef name, DWORD &value, DWORD dwDefault)
{
	DWORD type = 0;
	DWORD size = sizeof(value);

	if (::RegQueryValueEx(key, name.c_str(), nullptr, &type, (LPBYTE)&value, &size) != ERROR_SUCCESS ||
		type != REG_DWORD)
	{
		value = dwDefault;
		return false;
	}

	return true;
}

bool ff::RegGetValue(HKEY key, StringRef name, StringOut value, StringRef defaultValue)
{
	DWORD type = 0;
	DWORD size = 0;

	if (::RegQueryValueEx(key, name.c_str(), nullptr, &type, nullptr, &size) != ERROR_MORE_DATA ||
		type != REG_SZ || !size || size % sizeof(wchar_t))
	{
		value = defaultValue;
		return false;
	}

	StackCharVector512 data;
	data.Resize(size / sizeof(wchar_t) + 1); // add one for the bonus nullptr terminator

	if (::RegQueryValueEx(key, name.c_str(), nullptr, &type, (LPBYTE)data.Data(), &size) != ERROR_SUCCESS)
	{
		value = defaultValue;
		return false;
	}

	assert(data.Size() >= 2 && !data[data.Size() - 2]); // should already be nullptr terminated
	data[data.Size() - 1] = 0; // ...but nullptr terminate it anyway

	value = data.Data();

	return true;
}

bool ff::RegGetValue(HKEY key, StringRef name, GUID &guidValue, REFGUID guidDefault)
{
	String szGuid;

	if (!RegGetValue(key, name, szGuid) ||
		!StringToGuid(szGuid, guidValue))
	{
		guidValue = guidDefault;
		return false;
	}

	return true;
}

bool ff::RegSetValue(HKEY key, StringRef name, DWORD value)
{
	return ::RegSetValueEx(key, name.c_str(), 0, REG_DWORD, (LPBYTE)&value, sizeof(value)) == ERROR_SUCCESS;
}

bool ff::RegSetValue(HKEY key, StringRef name, StringRef value)
{
	DWORD size = ((DWORD)value.size() + 1) * sizeof(wchar_t);

	return ::RegSetValueEx(key, name.c_str(), 0, REG_SZ, (LPBYTE)value.c_str(), size) == ERROR_SUCCESS;
}

bool ff::RegSetValue(HKEY key, StringRef name, REFGUID guidValue)
{
	String szGuid = StringFromGuid(guidValue);

	return RegSetValue(key, name, szGuid);
}

bool ff::RegDeleteValue(HKEY key, StringRef name)
{
	return ::RegDeleteValue(key, name.c_str()) == ERROR_SUCCESS;
}

static bool InternalRegDeleteKey(HKEY parentKey, ff::StringRef keyName, int depth)
{
	assertRetVal(depth < 32, false);

	// delete child keys first
	{
		ff::RegKey key;
		if (!key.Open(parentKey, keyName))
		{
			return false;
		}

		ff::String szSubKeyName;
		ff::String szPreviousKeyName;

		for (; ff::RegEnumKey(key, 0, szSubKeyName); szPreviousKeyName = szSubKeyName)
		{
			if(szPreviousKeyName == szSubKeyName ||
				!InternalRegDeleteKey(key, szSubKeyName, depth + 1))
			{
				return false;
			}
		}
	}

	return ::RegDeleteKey(parentKey, keyName.c_str()) == ERROR_SUCCESS;
}

bool ff::RegDeleteKey(HKEY parentKey, StringRef keyName)
{
	// RegDeleteTree could be used in Vista, but this might still be used on XP

	return InternalRegDeleteKey(parentKey, keyName, 0);
}

bool ff::RegEnumKey(HKEY key, DWORD nIndex, StringOut outName)
{
	DWORD nSubKeys = 0;
	DWORD nMaxSubKeyNameLen = 0;

	if (::RegQueryInfoKey(key,
		nullptr, nullptr, nullptr,
		&nSubKeys,
		&nMaxSubKeyNameLen,
		nullptr,
		nullptr, // &nValues,
		nullptr, // &nMaxValueNameLen,
		nullptr, // &nMaxValueLen,
		nullptr, nullptr) != ERROR_SUCCESS)
	{
		return false;
	}

	if (nIndex < nSubKeys)
	{
		StackCharVector512 data;
		data.Resize(++nMaxSubKeyNameLen);

		switch (::RegEnumKeyEx(key, nIndex, data.Data(), &nMaxSubKeyNameLen, nullptr, nullptr, nullptr, nullptr))
		{
		case ERROR_SUCCESS:
			outName = data.Data();
			return true;

		default:
			assert(false);
			__fallthrough;

		case ERROR_NO_MORE_ITEMS:
			outName.clear();
			return false;
		}
	}

	return false;
}

bool ff::RegEnumValue(HKEY key, DWORD nIndex, StringOut outName, LPDWORD pType)
{
	DWORD nValues = 0;
	DWORD nMaxValueNameLen = 0;

	if (::RegQueryInfoKey(key,
		nullptr, nullptr, nullptr,
		nullptr, // &nSubKeys,
		nullptr, // &nMaxSubKeyNameLen,
		nullptr,
		&nValues,
		&nMaxValueNameLen,
		nullptr, // &nMaxValueLen,
		nullptr, nullptr) != ERROR_SUCCESS)
	{
		return false;
	}

	if (nIndex >= nValues)
	{
		return false;
	}

	StackCharVector512 data;
	data.Resize(++nMaxValueNameLen);

	switch (::RegEnumValue(key, nIndex, data.Data(), &nMaxValueNameLen, nullptr, pType, nullptr, nullptr))
	{
	case ERROR_SUCCESS:
		outName = data.Data();
		return true;

	default:
		assert(false);
		__fallthrough;

	case ERROR_NO_MORE_ITEMS:
		return false;
	}
}

ff::RegKey::RegKey()
	: _key(nullptr)
{
}

ff::RegKey::~RegKey()
{
	Close();
}

ff::RegKey::operator HKEY() const
{
	return _key;
}

HKEY ff::RegKey::Create(HKEY parentKey, StringRef keyName)
{
	Close();

	if (::RegCreateKeyEx(parentKey, keyName.c_str(), 0, nullptr,
		REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nullptr, &_key, nullptr) != ERROR_SUCCESS)
	{
		_key = nullptr;
	}

	return _key;
}

HKEY ff::RegKey::Open(HKEY parentKey, StringRef keyName, bool bReadOnly)
{
	Close();

	if (::RegOpenKeyEx(parentKey, keyName.c_str(), 0, bReadOnly ? KEY_READ : KEY_ALL_ACCESS, &_key) != ERROR_SUCCESS)
	{
		_key = nullptr;
	}

	return _key;
}

void ff::RegKey::Close()
{
	if (_key)
	{
		::RegCloseKey(_key);
		_key = nullptr;
	}
}

#endif // !METRO_APP
