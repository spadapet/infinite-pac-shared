#include "pch.h"
#include "Dict/Dict.h"
#include "Dict/ValueTable.h"
#include "Module/ModuleFactory.h"
#include "Resource/ResourcePersist.h"

class __declspec(uuid("920b9c39-ced0-4ccf-865b-6a2a4e3b6365"))
	ValueTable : public ff::ComBase, public ff::IValueTable, public ff::IResourceLoad
{
public:
	DECLARE_HEADER(ValueTable);

	bool Init(const ff::Dict &dict);

	// IValueTable
	virtual ff::Value *GetValue(ff::StringRef name) const override;
	virtual ff::String GetString(ff::StringRef name) const override;

	// IResourceLoad
	virtual bool LoadResource(const ff::Dict &dict) override;

private:
	void UpdateUserLanguages();

	ff::Vector<ff::String> _userLangs;
	ff::Map<ff::String, ff::Dict> _langToDict;
};

BEGIN_INTERFACES(ValueTable)
	HAS_INTERFACE(ff::IValueTable)
	HAS_INTERFACE(ff::IResourceLoad)
END_INTERFACES()

static ff::ModuleStartup Register([](ff::Module &module)
{
	static ff::StaticString name(L"values");
	module.RegisterClassT<ValueTable>(name);
});

bool ff::CreateValueTable(const Dict &dict, ff::IValueTable **obj)
{
	assertRetVal(obj, false);

	ComPtr<ValueTable, IValueTable> myObj;
	assertHrRetVal(ff::ComAllocator<ValueTable>::CreateInstance(&myObj), false);
	assertRetVal(myObj->LoadResource(dict), false);

	*obj = myObj.Detach();
	return true;
}

ValueTable::ValueTable()
{
	UpdateUserLanguages();
}

ValueTable::~ValueTable()
{
}

bool ValueTable::Init(const ff::Dict &dict)
{
	return LoadResource(dict);
}

ff::Value *ValueTable::GetValue(ff::StringRef name) const
{
	for (ff::StringRef langName : _userLangs)
	{
		ff::BucketIter iter = _langToDict.Get(langName);
		if (iter != ff::INVALID_ITER)
		{
			const ff::Dict &dict = _langToDict.ValueAt(iter);
			ff::Value *value = dict.GetValue(name);
			if (value)
			{
				return value;
			}
		}
	}

	return nullptr;
}

ff::String ValueTable::GetString(ff::StringRef name) const
{
	ff::Value *value = GetValue(name);

	ff::ValuePtr strValue;
	if (value && value->Convert(ff::Value::Type::String, &strValue))
	{
		return strValue->AsString();
	}

	return ff::GetEmptyString();
}

bool ValueTable::LoadResource(const ff::Dict &dict)
{
	ff::Vector<ff::String> langNames = dict.GetAllNames();
	for (ff::StringRef langName : langNames)
	{
		ff::ValuePtr langDictValue;
		if (dict.GetValue(langName)->Convert(ff::Value::Type::Dict, &langDictValue))
		{
			ff::BucketIter iter = _langToDict.Get(langName);
			if (iter == ff::INVALID_ITER)
			{
				iter = _langToDict.SetKey(langName, ff::Dict());
			}

			_langToDict.ValueAt(iter).Add(langDictValue->AsDict());
		}
	}

	return true;
}

void ValueTable::UpdateUserLanguages()
{
	static ff::StaticString overrideName(L"override");
	static ff::StaticString globalName(L"global");

	_userLangs.Clear();
	_userLangs.Push(overrideName);

	wchar_t fullLangName[LOCALE_NAME_MAX_LENGTH];
	if (::GetUserDefaultLocaleName(fullLangName, LOCALE_NAME_MAX_LENGTH))
	{
		ff::String langName(fullLangName);
		while (langName.size())
		{
			_userLangs.Push(langName);

			size_t lastDash = langName.rfind(L'-');
			if (lastDash == ff::INVALID_SIZE)
			{
				lastDash = 0;
			}

			langName = langName.substr(0, lastDash);
		}
	}

	_userLangs.Push(globalName);
	_userLangs.Push(ff::GetEmptyString());
}
