#include "pch.h"
#include "Globals/ProcessGlobals.h"
#include "Resource/Resources.h"
#include "Resource/ResourceValue.h"

ff::ResourceValue::ResourceValue(IUnknown *obj, ff::StringRef name)
	: _name(name)
	, _owner(nullptr)
{
	if (obj)
	{
		verify(ff::Value::CreateObject(obj, &_value));
	}
	else
	{
		verify(ff::Value::CreateNull(&_value));
	}
}

ff::ResourceValue::ResourceValue(Value *value, StringRef name)
	: _value(value)
	, _name(name)
	, _owner(nullptr)
{
}

ff::ResourceValue::ResourceValue(ResourceValue &&rhs)
	: _value(std::move(rhs._value))
	, _newValue(std::move(rhs._newValue))
	, _name(std::move(rhs._name))
	, _owner(rhs._owner)
{
	rhs._owner = nullptr;
}

ff::Value *ff::ResourceValue::GetValue() const
{
	return _value;
}

ff::StringRef ff::ResourceValue::GetName() const
{
	return _name;
}

bool ff::ResourceValue::QueryObject(const GUID &iid, void **obj) const
{
	noAssertRetVal(_value != nullptr && _value->IsType(Value::Type::Object), false);
	return SUCCEEDED(_value->AsObject()->QueryInterface(iid, obj));
}

bool ff::ResourceValue::IsValid() const
{
	return _newValue == nullptr;
}

void ff::ResourceValue::StartedLoading(IResources *owner)
{
	LockMutex lock(GCS_RESOURCE_VALUE);
	assert(!_owner && owner);
	_owner = owner;
}

void ff::ResourceValue::Invalidate(SharedResourceValue newValue)
{
	assertRet(newValue != nullptr);

	LockMutex lock(GCS_RESOURCE_VALUE);
	_newValue = newValue;
	_owner = nullptr;
}

ff::ComPtr<ff::IResources> ff::ResourceValue::GetLoadingOwner() const
{
	LockMutex lock(GCS_RESOURCE_VALUE);
	return _owner;
}

ff::SharedResourceValue ff::ResourceValue::GetNewValue() const
{
	SharedResourceValue newValue;

	if (!IsValid())
	{
		LockMutex lock(GCS_RESOURCE_VALUE);
		if (!IsValid())
		{
			newValue = _newValue;

			while (!newValue->IsValid())
			{
				newValue = newValue->GetNewValue();
			}
		}
	}

	return newValue;
}

ff::AutoResourceValue::AutoResourceValue()
{
}

ff::AutoResourceValue::AutoResourceValue(IResources *resources, StringRef name)
{
	Init(resources, name);
}

ff::AutoResourceValue::AutoResourceValue(const AutoResourceValue &rhs)
	: _value(rhs._value)
{
}

ff::AutoResourceValue::AutoResourceValue(AutoResourceValue &&rhs)
	: _value(std::move(rhs._value))
{
}

ff::AutoResourceValue &ff::AutoResourceValue::operator=(const AutoResourceValue &rhs)
{
	_value = rhs._value;
	return *this;
}

void ff::AutoResourceValue::Init(IResources *resources, StringRef name)
{
	if (resources)
	{
		_value = resources->GetResource(name);
	}
	else
	{
		ValuePtr nullValue;
		Value::CreateNull(&nullValue);
		_value = std::make_shared<ResourceValue>(nullValue, name);
	}
}

void ff::AutoResourceValue::Init(SharedResourceValue value)
{
	assert(value != nullptr);
	_value = value;
}

bool ff::AutoResourceValue::DidInit() const
{
	return _value != nullptr;
}

ff::Value *ff::AutoResourceValue::Flush()
{
	if (DidInit())
	{
		ff::ComPtr<ff::IResources> owner = _value->GetLoadingOwner();
		if (owner)
		{
			_value = owner->FlushResource(_value);
		}

		return GetValue();
	}

	return nullptr;
}

ff::StringRef ff::AutoResourceValue::GetName()
{
	return UpdateValue()->GetName();
}

ff::Value *ff::AutoResourceValue::GetValue()
{
	return UpdateValue()->GetValue();
}

ff::SharedResourceValue ff::AutoResourceValue::GetResourceValue()
{
	return UpdateValue();
}

bool ff::AutoResourceValue::QueryObject(const GUID &iid, void **obj)
{
	return UpdateValue()->QueryObject(iid, obj);
}

const ff::SharedResourceValue &ff::AutoResourceValue::UpdateValue()
{
	if (!_value)
	{
		static ff::SharedResourceValue s_null;
		if (!s_null)
		{
			LockMutex crit(GCS_RESOURCE_VALUE);
			if (!s_null)
			{
				ff::ValuePtr nullValue;
				ff::Value::CreateNull(&nullValue);
				s_null = std::make_shared<ff::ResourceValue>(nullValue, ff::GetEmptyString());

				ff::SharedResourceValue *p_null = &s_null;
				ff::AtProgramShutdown([p_null]()
				{
					p_null->reset();
				});
			}
		}

		return s_null;
	}

	if (!_value->IsValid())
	{
		_value = _value->GetNewValue();
	}

	return _value;
}
