#include "pch.h"
#include "Data/Data.h"
#include "Data/SavedData.h"
#include "Dict/Dict.h"
#include "Dict/DictPersist.h"
#include "Globals/Log.h"
#include "Globals/ProcessGlobals.h"
#include "String/StringUtil.h"

static const size_t MAX_SMALL_DICT = 128;

ff::Dict::Dict()
	: _atomizer(nullptr)
{
}

ff::Dict::Dict(const Dict &rhs)
	: _atomizer(rhs._atomizer)
{
	*this = rhs;
}

ff::Dict::Dict(Dict &&rhs)
	: _atomizer(rhs._atomizer)
	, _propsLarge(std::move(rhs._propsLarge))
	, _propsSmall(std::move(rhs._propsSmall))
{
}

ff::Dict::Dict(const SmallDict &rhs)
	: _atomizer(nullptr)
{
	*this = rhs;
}

ff::Dict::Dict(SmallDict &&rhs)
	: _atomizer(nullptr)
	, _propsSmall(std::move(rhs))
{
}

const ff::Dict &ff::Dict::operator=(const Dict &rhs)
{
	if (this != &rhs)
	{
		Clear();

		_atomizer = rhs._atomizer;
		_propsSmall = rhs._propsSmall;
		_propsLarge.reset();
		
		if (rhs._propsLarge != nullptr)
		{
			_propsLarge.reset(new PropsMap(*rhs._propsLarge));
		}
	}

	return *this;
}

const ff::Dict &ff::Dict::operator=(const SmallDict &rhs)
{
	if (&_propsSmall != &rhs)
	{
		Clear();
		_propsSmall = rhs;
		_propsLarge.reset();
	}

	return *this;
}

ff::Dict::~Dict()
{
	Clear();
}

void ff::Dict::Clear()
{
	_propsSmall.Clear();
	_propsLarge.reset();
}

void ff::Dict::Add(const Dict &rhs)
{
	Vector<String> names = rhs.GetAllNames();

	for (StringRef name: names)
	{
		Value *value = rhs.GetValue(name);
		SetValue(name, value);
	}
}

void ff::Dict::Merge(const Dict &rhs)
{
	for (StringRef name: rhs.GetAllNames())
	{
		Value *value = rhs.GetValue(name);
		ff::ValuePtr newValue;

		if (value->IsType(ff::Value::Type::Dict) ||
			value->IsType(ff::Value::Type::SavedDict))
		{
			Value *myValue = GetValue(name);
			if (myValue && (myValue->IsType(ff::Value::Type::Dict) ||
				myValue->IsType(ff::Value::Type::SavedDict)))
			{
				ff::ValuePtr dictValue, myDictValue;
				if (value->Convert(ff::Value::Type::Dict, &dictValue) &&
					myValue->Convert(ff::Value::Type::Dict, &myDictValue))
				{
					Dict myDict = myDictValue->AsDict();
					myDict.Merge(dictValue->AsDict());

					if (ff::Value::CreateDict(std::move(myDict), &newValue))
					{
						value = newValue;
					}
				}
			}
		}

		SetValue(name, value);
	}
}

void ff::Dict::Reserve(size_t count)
{
	if (_propsLarge == nullptr)
	{
		_propsSmall.Reserve(count, false);
	}
}

bool ff::Dict::IsEmpty() const
{
	bool empty = true;

	if (_propsLarge != nullptr)
	{
		empty = _propsLarge->IsEmpty();
	}
	else if (_propsSmall.Size())
	{
		empty = false;
	}

	return false;
}

size_t ff::Dict::Size() const
{
	size_t size = 0;

	if (_propsLarge != nullptr)
	{
		size = _propsLarge->Size();
	}
	else
	{
		size = _propsSmall.Size();
	}

	return size;
}

void ff::Dict::Add(const SmallDict &rhs)
{
	for (size_t i = 0; i < rhs.Size(); i++)
	{
		SetValue(rhs.KeyAt(i), rhs.ValueAt(i));
	}
}

void ff::Dict::SetValue(ff::StringRef name, ff::Value *value)
{
	if (value)
	{
		if (_propsLarge != nullptr)
		{
			hash_t hash = GetAtomizer().CacheString(name);
			_propsLarge->SetKey(hash, value);
		}
		else
		{
			_propsSmall.Set(name, value);
			CheckSize();
		}
	}
	else if (_propsLarge != nullptr)
	{
		hash_t hash = GetAtomizer().CacheString(name);
		_propsLarge->DeleteKey(hash);
	}
	else
	{
		_propsSmall.Remove(name);
	}
}

ff::Value *ff::Dict::GetValue(ff::StringRef name) const
{
	Value *value = nullptr;

	if (_propsLarge != nullptr)
	{
		hash_t hash = GetAtomizer().GetHash(name);
		BucketIter iter = _propsLarge->Get(hash);

		if (iter != INVALID_ITER)
		{
			value = _propsLarge->ValueAt(iter);
		}
	}
	else
	{
		value = _propsSmall.GetValue(name);
	}

	return value;
}

void ff::Dict::SetInt(ff::StringRef name, int value)
{
	ValuePtr newValue;
	assertRet(Value::CreateInt(value, &newValue));

	SetValue(name, newValue);
}

void ff::Dict::SetSize(StringRef name, size_t value)
{
	assert(value == ff::INVALID_SIZE || value <= INT_MAX);
	SetInt(name, value != ff::INVALID_SIZE ? (int)value : -1);
}

void ff::Dict::SetBool(ff::StringRef name, bool value)
{
	ValuePtr newValue;
	assertRet(Value::CreateBool(value, &newValue));

	SetValue(name, newValue);
}

void ff::Dict::SetRect(ff::StringRef name, ff::RectInt value)
{
	ValuePtr newValue;
	assertRet(Value::CreateRect(value, &newValue));

	SetValue(name, newValue);
}

void ff::Dict::SetRectF(ff::StringRef name, ff::RectFloat value)
{
	ValuePtr newValue;
	assertRet(Value::CreateRectF(value, &newValue));

	SetValue(name, newValue);
}

void ff::Dict::SetPoint(ff::StringRef name, ff::PointInt value)
{
	ValuePtr newValue;
	assertRet(Value::CreatePoint(value, &newValue));

	SetValue(name, newValue);
}

void ff::Dict::SetPointF(ff::StringRef name, ff::PointFloat value)
{
	ValuePtr newValue;
	assertRet(Value::CreatePointF(value, &newValue));

	SetValue(name, newValue);
}

void ff::Dict::SetFloat(ff::StringRef name, float value)
{
	ValuePtr newValue;
	assertRet(Value::CreateFloat(value, &newValue));

	SetValue(name, newValue);
}

void ff::Dict::SetDouble(ff::StringRef name, double value)
{
	ValuePtr newValue;
	assertRet(Value::CreateDouble(value, &newValue));

	SetValue(name, newValue);
}

void ff::Dict::SetString(ff::StringRef name, ff::StringRef value)
{
	ValuePtr newValue;
	assertRet(Value::CreateString(value, &newValue));

	SetValue(name, newValue);
}

void ff::Dict::SetGuid(ff::StringRef name, REFGUID value)
{
	ValuePtr newValue;
	assertRet(Value::CreateGuid(value, &newValue));

	SetValue(name, newValue);
}

void ff::Dict::SetData(ff::StringRef name, ff::IData *value)
{
	ValuePtr newValue;
	assertRet(Value::CreateData(value, &newValue));

	SetValue(name, newValue);
}

void ff::Dict::SetData(StringRef name, const void *data, size_t size)
{
	ff::ComPtr<ff::IDataVector> dataVector;
	if (ff::CreateDataVector(size, &dataVector))
	{
		memcpy(dataVector->GetVector().Data(), data, size);
		SetData(name, dataVector);
	}
}

void ff::Dict::SetSavedData(StringRef name, ISavedData *value)
{
	ComPtr<ISavedData> valueClone;
	assertRet(value && value->Clone(&valueClone));

	ValuePtr newValue;
	assertRet(Value::CreateSavedData(valueClone, &newValue));

	SetValue(name, newValue);
}

void ff::Dict::SetDict(StringRef name, const Dict &value)
{
	ValuePtr newValue;
	assertRet(Value::CreateDict(Dict(value), &newValue));

	SetValue(name, newValue);
}

void ff::Dict::SetResource(ff::StringRef name, const ff::SharedResourceValue &value)
{
	ValuePtr newValue;
	assertRet(Value::CreateResource(value, &newValue));

	SetValue(name, newValue);
}

int ff::Dict::GetInt(ff::StringRef name, int defaultValue) const
{
	Value *value = GetValue(name);

	if (value)
	{
		if (value->IsType(Value::Type::Int))
		{
			return value->AsInt();
		}
		else
		{
			ValuePtr newValue;

			if (value->Convert(Value::Type::Int, &newValue))
			{
				return newValue->AsInt();
			}
		}
	}

	return defaultValue;
}

size_t ff::Dict::GetSize(StringRef name, size_t defaultValue) const
{
	Value *value = GetValue(name);

	if (value)
	{
		ValuePtr newValue;
		if (value->Convert(Value::Type::Int, &newValue))
		{
			int ival = newValue->AsInt();
			return ival < 0 ? ff::INVALID_SIZE : (size_t)ival;
		}
	}

	return defaultValue;
}

bool ff::Dict::GetBool(ff::StringRef name, bool defaultValue) const
{
	Value *value = GetValue(name);

	if (value)
	{
		if (value->IsType(Value::Type::Bool))
		{
			return value->AsBool();
		}
		else
		{
			ValuePtr newValue;

			if (value->Convert(Value::Type::Bool, &newValue))
			{
				return newValue->AsBool();
			}
		}
	}

	return defaultValue;
}

ff::RectInt ff::Dict::GetRect(ff::StringRef name, RectInt defaultValue) const
{
	Value *value = GetValue(name);

	if (value)
	{
		if (value->IsType(Value::Type::Rect))
		{
			return value->AsRect();
		}
		else
		{
			ValuePtr newValue;

			if (value->Convert(Value::Type::Rect, &newValue))
			{
				return newValue->AsRect();
			}
		}
	}

	return defaultValue;
}

ff::RectFloat ff::Dict::GetRectF(ff::StringRef name, RectFloat defaultValue) const
{
	Value *value = GetValue(name);

	if (value)
	{
		if (value->IsType(Value::Type::RectF))
		{
			return value->AsRectF();
		}
		else
		{
			ValuePtr newValue;

			if (value->Convert(Value::Type::RectF, &newValue))
			{
				return newValue->AsRectF();
			}
		}
	}

	return defaultValue;
}

float ff::Dict::GetFloat(ff::StringRef name, float defaultValue) const
{
	Value *value = GetValue(name);

	if (value)
	{
		if (value->IsType(Value::Type::Float))
		{
			return value->AsFloat();
		}
		else
		{
			ValuePtr newValue;

			if (value->Convert(Value::Type::Float, &newValue))
			{
				return newValue->AsFloat();
			}
		}
	}

	return defaultValue;
}

double ff::Dict::GetDouble(ff::StringRef name, double defaultValue) const
{
	Value *value = GetValue(name);

	if (value)
	{
		if (value->IsType(Value::Type::Double))
		{
			return value->AsDouble();
		}
		else
		{
			ValuePtr newValue;

			if (value->Convert(Value::Type::Double, &newValue))
			{
				return newValue->AsDouble();
			}
		}
	}

	return defaultValue;
}

ff::PointInt ff::Dict::GetPoint(ff::StringRef name, PointInt defaultValue) const
{
	Value *value = GetValue(name);

	if (value)
	{
		if (value->IsType(Value::Type::Point))
		{
			return value->AsPoint();
		}
		else
		{
			ValuePtr newValue;

			if (value->Convert(Value::Type::Point, &newValue))
			{
				return newValue->AsPoint();
			}
		}
	}

	return defaultValue;
}

ff::PointFloat ff::Dict::GetPointF(ff::StringRef name, PointFloat defaultValue) const
{
	Value *value = GetValue(name);

	if (value)
	{
		if (value->IsType(Value::Type::PointF))
		{
			return value->AsPointF();
		}
		else
		{
			ValuePtr newValue;

			if (value->Convert(Value::Type::PointF, &newValue))
			{
				return newValue->AsPointF();
			}
		}
	}

	return defaultValue;
}

ff::String ff::Dict::GetString(ff::StringRef name, String defaultValue) const
{
	Value *value = GetValue(name);

	if (value)
	{
		if (value->IsType(Value::Type::String))
		{
			return value->AsString();
		}
		else
		{
			ValuePtr newValue;

			if (value->Convert(Value::Type::String, &newValue))
			{
				return newValue->AsString();
			}
		}
	}

	return defaultValue;
}

GUID ff::Dict::GetGuid(ff::StringRef name, REFGUID defaultValue) const
{
	Value *value = GetValue(name);

	if (value)
	{
		if (value->IsType(Value::Type::Guid))
		{
			return value->AsGuid();
		}
		else
		{
			ValuePtr newValue;

			if (value->Convert(Value::Type::Guid, &newValue))
			{
				return newValue->AsGuid();
			}
		}
	}

	return defaultValue;
}

ff::ComPtr<ff::IData> ff::Dict::GetData(ff::StringRef name) const
{
	Value *value = GetValue(name);
	ValuePtr dataValue;

	if (value && value->Convert(Value::Type::Data, &dataValue))
	{
		return dataValue->AsData();
	}

	return nullptr;
}

ff::ComPtr<ff::ISavedData> ff::Dict::GetSavedData(StringRef name) const
{
	Value *value = GetValue(name);
	ValuePtr dataValue;

	if (value && value->Convert(Value::Type::SavedData, &dataValue))
	{
		return dataValue->AsSavedData();
	}

	return nullptr;
}

ff::Dict ff::Dict::GetDict(StringRef name) const
{
	Value *value = GetValue(name);
	ValuePtr newValue;

	if (value && value->Convert(Value::Type::Dict, &newValue))
	{
		return newValue->AsDict();
	}

	return ff::Dict();
}

ff::SharedResourceValue ff::Dict::GetResource(ff::StringRef name) const
{
	Value *value = GetValue(name);
	ValuePtr newValue;

	if (value && value->Convert(Value::Type::Resource, &newValue))
	{
		return newValue->AsResource();
	}

	return ff::SharedResourceValue();
}

ff::Vector<ff::String> ff::Dict::GetAllNames(bool sorted) const
{
	Set<String> nameSet;
	InternalGetAllNames(nameSet);

	Vector<String> names;
	for (StringRef name: nameSet)
	{
		names.Push(name);
	}

	if (sorted && names.Size())
	{
		std::sort(names.begin(), names.end());
	}

	return names;
}

void ff::Dict::InternalGetAllNames(Set<String> &names) const
{
	if (_propsLarge != nullptr)
	{
		for (const auto &iter: *_propsLarge)
		{
			hash_t hash = iter.GetKey();
			String name = GetAtomizer().GetString(hash);
			names.SetKey(name);
		}
	}
	else
	{
		for (size_t i = 0; i < _propsSmall.Size(); i++)
		{
			String name = _propsSmall.KeyAt(i);
			names.SetKey(name);
		}
	}
}

void ff::Dict::DebugDump() const
{
	ff::DebugDumpDict(*this);
}

void ff::Dict::CheckSize()
{
	if (_propsSmall.Size() > MAX_SMALL_DICT)
	{
		_propsLarge.reset(new PropsMap());
		Add(_propsSmall);
		_propsSmall.Clear();
	}
}

ff::StringCache &ff::Dict::GetAtomizer() const
{
	return _atomizer ? *_atomizer : ff::ProcessGlobals::Get()->GetStringCache();
}
