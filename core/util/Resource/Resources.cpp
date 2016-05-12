#include "pch.h"
#include "Globals/AppGlobals.h"
#include "Globals/ProcessGlobals.h"
#include "Module/ModuleFactory.h"
#include "Resource/ResourcePersist.h"
#include "Resource/Resources.h"
#include "Thread/ThreadPool.h"
#include "Thread/ThreadUtil.h"
#include "Windows/Handles.h"

static ff::StaticString RES_TYPE(L"res:type");

class __declspec(uuid("676f438b-b858-4fa7-82f8-50dae9505bce"))
	Resources : public ff::ComBase, public ff::IResources, public ff::IResourceSave
{
public:
	DECLARE_HEADER(Resources);

	bool Init(ff::AppGlobals *globals, const ff::Dict &dict);

	// IResources
	virtual void SetResources(const ff::Dict &dict) override;
	virtual ff::Dict GetResources() const override;
	virtual void Clear() override;
	virtual bool IsLoading() const override;

	virtual ff::SharedResourceValue GetResource(ff::StringRef name) override;
	virtual ff::SharedResourceValue FlushResource(ff::SharedResourceValue value) override;
	virtual ff::AppGlobals *GetContext() const override;

	// IResourceSave
	virtual bool LoadResource(const ff::Dict &dict) override;
	virtual bool SaveResource(ff::Dict &dict) override;

private:
	typedef std::weak_ptr<ff::ResourceValue> WeakResourceValue;

	struct ValueInfo
	{
		WeakResourceValue _value;
		HANDLE _event;
	};

	typedef std::shared_ptr<ValueInfo> ValueInfoPtr;

	ValueInfoPtr GetValueInfo(ff::StringRef name);
	ff::SharedResourceValue CreateNullResource(ff::StringRef name) const;

	ff::SharedResourceValue StartLoading(ff::StringRef name);
	void DoLoad(ValueInfoPtr info, ff::StringRef name, ff::ValuePtr value);
	void Invalidate(ff::StringRef name);
	void UpdateValueInfo(ValueInfoPtr info, ff::SharedResourceValue newValue);
	ff::ValuePtr CreateObjects(ff::ValuePtr value);

	ff::Mutex _cs;
	ff::Dict _dict;
	ff::Map<ff::String, ValueInfoPtr> _values;
	ff::AppGlobals *_globals;
	long _loadingCount;
};

BEGIN_INTERFACES(Resources)
	HAS_INTERFACE(ff::IResources)
	HAS_INTERFACE(ff::IResourceLoad)
END_INTERFACES()

static ff::StaticString RESOURCES_CLASS_NAME(L"resources");
static ff::ModuleStartup Register([](ff::Module &module)
{
	module.RegisterClassT<Resources>(RESOURCES_CLASS_NAME);
});

bool ff::CreateResources(AppGlobals *globals, const Dict &dict, ff::IResources **obj)
{
	assertRetVal(obj, false);

	ComPtr<Resources, IResources> myObj;
	assertHrRetVal(ff::ComAllocator<Resources>::CreateInstance(&myObj), false);
	assertRetVal(myObj->Init(globals, dict), false);

	*obj = myObj.Detach();
	return true;
}

Resources::Resources()
	: _loadingCount(0)
{
}

Resources::~Resources()
{
	Clear();
}

bool Resources::Init(ff::AppGlobals *globals, const ff::Dict &dict)
{
	_globals = globals;
	return LoadResource(dict);
}

void Resources::SetResources(const ff::Dict &dict)
{
	ff::LockMutex crit(_cs);

	_dict.Add(dict);

	// Invalidate and reload existing resources that match the new names
	ff::Vector<ff::String> names = dict.GetAllNames();
	for (ff::StringRef name : names)
	{
		if (_values.Exists(name))
		{
			Invalidate(name);
			StartLoading(name);
		}
	}
}

ff::Dict Resources::GetResources() const
{
	ff::Dict dict;
	{
		ff::LockMutex crit(_cs);
		dict = _dict;
	}

	return dict;
}

void Resources::Clear()
{
	ff::LockMutex crit(_cs);

	for (ff::BucketIter i = _values.StartIteration(); i != ff::INVALID_ITER; i = _values.Iterate(i))
	{
		Invalidate(_values.KeyAt(i));
	}

	_values.Clear();
	_dict.Clear();
}

bool Resources::IsLoading() const
{
	return ff::InterlockedAccess(_loadingCount) != 0;
}

ff::SharedResourceValue Resources::GetResource(ff::StringRef name)
{
	ff::LockMutex crit(_cs);
	noAssertRetVal(_dict.GetValue(name), CreateNullResource(name));
	return StartLoading(name);
}

ff::SharedResourceValue Resources::FlushResource(ff::SharedResourceValue value)
{
	ff::ComPtr<ff::IResources> owner = value->GetLoadingOwner();
	noAssertRetVal(owner, value);
	assertRetVal(owner == this, value);

	ff::WinHandle loadEvent;
	{
		ff::LockMutex crit(_cs);

		for (ff::BucketIter i = _values.StartIteration(); i != ff::INVALID_ITER; i = _values.Iterate(i))
		{
			ValueInfo &info = *_values.ValueAt(i).get();
			if (info._event)
			{
				ff::SharedResourceValue curValue = info._value.lock();
				if (value == curValue)
				{
					loadEvent = ff::DuplicateHandle(info._event);
					break;
				}
			}
		}
	}

	if (loadEvent)
	{
		ff::WaitForHandle(loadEvent);
	}

	return value->IsValid() ? value : value->GetNewValue();
}

ff::AppGlobals *Resources::GetContext() const
{
	return _globals ? _globals : ff::AppGlobals::Get();
}

bool Resources::LoadResource(const ff::Dict &dict)
{
	ff::LockMutex crit(_cs);

	Clear();
	SetResources(dict);
	return true;
}

bool Resources::SaveResource(ff::Dict &dict)
{
	dict = GetResources();
	return true;
}

Resources::ValueInfoPtr Resources::GetValueInfo(ff::StringRef name)
{
	ff::LockMutex crit(_cs);
	ff::BucketIter iter = _values.Get(name);

	if (iter == ff::INVALID_ITER)
	{
		ValueInfoPtr info = std::make_shared<ValueInfo>();
		info->_value = CreateNullResource(name);
		info->_event = nullptr;
		iter = _values.SetKey(name, info);
	}

	return _values.ValueAt(iter);
}

ff::SharedResourceValue Resources::CreateNullResource(ff::StringRef name) const
{
	ff::ValuePtr nullValue;
	ff::Value::CreateNull(&nullValue);
	return std::make_shared<ff::ResourceValue>(nullValue, name);
}

ff::SharedResourceValue Resources::StartLoading(ff::StringRef name)
{
	ff::LockMutex crit(_cs);

	ValueInfoPtr infoPtr = GetValueInfo(name);
	ValueInfo &info = *infoPtr.get();
	ff::SharedResourceValue value = info._value.lock();

	if (value == nullptr)
	{
		value = CreateNullResource(name);
		value->StartedLoading(this);
		info._value = value;
	}

	noAssertRetVal(!info._event, value);
	noAssertRetVal(value->GetValue()->IsType(ff::Value::Type::Null), value);

	ff::ValuePtr dictValue = _dict.GetValue(name);
	assertRetVal(dictValue && !dictValue->IsType(ff::Value::Type::Null), value);

	info._event = ff::CreateEvent();
	::InterlockedIncrement(&_loadingCount);

	ff::ComPtr<Resources, IResources> keepAlive = this;
	ff::String keepName = name;

	ff::GetThreadPool()->Add([=]()
	{
		keepAlive->DoLoad(infoPtr, keepName, dictValue);
		::InterlockedDecrement(&keepAlive->_loadingCount);
	});

	return value;
}

// background thread
void Resources::DoLoad(ValueInfoPtr info, ff::StringRef name, ff::ValuePtr value)
{
	value = CreateObjects(value);

	ff::SharedResourceValue newValue = std::make_shared<ff::ResourceValue>(value, name);
	UpdateValueInfo(info, newValue);
}

void Resources::Invalidate(ff::StringRef name)
{
	UpdateValueInfo(GetValueInfo(name), CreateNullResource(name));
}

void Resources::UpdateValueInfo(ValueInfoPtr infoPtr, ff::SharedResourceValue newValue)
{
	ff::LockMutex crit(_cs);
	ValueInfo &info = *infoPtr.get();
	ff::SharedResourceValue oldResource = info._value.lock();

	if (oldResource != nullptr)
	{
		oldResource->Invalidate(newValue);
	}

	info._value = newValue;

	if (info._event)
	{
		HANDLE event = info._event;
		info._event = nullptr;

		::SetEvent(event);
		::CloseHandle(event);
	}
}

// background thread
ff::ValuePtr Resources::CreateObjects(ff::ValuePtr value)
{
	assertRetVal(value, nullptr);

	switch (value->GetType())
	{
		// Uncompress all data
		case ff::Value::Type::SavedData:
		{
			ff::ValuePtr newValue;
			assertRetVal(value->Convert(ff::Value::Type::Data, &newValue), value);
			value = CreateObjects(newValue);
		}
		break;

		// Uncompress all dicts
		case ff::Value::Type::SavedDict:
		{
			ff::ValuePtr newValue;
			assertRetVal(value->Convert(ff::Value::Type::Dict, &newValue), value);
			value = CreateObjects(newValue);
		}
		break;

		// Resolve references to other resources
		case ff::Value::Type::String:
		{
			if (!wcsncmp(value->AsString().c_str(), L"ref:", 4))
			{
				ff::String refName = value->AsString().substr(4);
				ff::SharedResourceValue refValue = GetResource(refName);
				ff::ValuePtr newValue;
				assertRetVal(ff::Value::CreateResource(refValue, &newValue), value);
				value = CreateObjects(newValue);
			}
		}
		break;

		// Convert dicts with a "res:type" value to a COM object
		case ff::Value::Type::Dict:
		{
			ff::Dict dict = value->AsDict();
			ff::String type = dict.GetString(RES_TYPE);
			bool isNestedResources = (type == RESOURCES_CLASS_NAME.GetString());

			if (!isNestedResources)
			{
				ff::Vector<ff::String> names = dict.GetAllNames();
				for (ff::StringRef name : names)
				{
					ff::ValuePtr newValue = CreateObjects(dict.GetValue(name));
					dict.SetValue(name, newValue);
				}
			}

			if (!isNestedResources && type.size())
			{
				ff::ComPtr<IUnknown> obj = ff::ProcessGlobals::Get()->GetModules().CreateClass(type, GetContext());
				ff::ComPtr<IResourceLoad> resObj;
				assertRetVal(resObj.QueryFrom(obj) && resObj->LoadResource(dict), value);

				ff::ValuePtr newValue;
				assertRetVal(ff::Value::CreateObject(resObj, &newValue), value);
				value = CreateObjects(newValue);
			}
			else
			{
				ff::ValuePtr newValue;
				assertRetVal(ff::Value::CreateDict(std::move(dict), &newValue), value);
				value = newValue;
			}
		}
		break;

		case ff::Value::Type::ValueVector:
		{
			ff::Vector<ff::ValuePtr> vec = value->AsValueVector();
			for (size_t i = 0; i < vec.Size(); i++)
			{
				vec[i] = CreateObjects(vec[i]);
			}

			ff::ValuePtr newValue;
			assertRetVal(ff::Value::CreateValueVector(std::move(vec), &newValue), value);
			value = newValue;
		}
		break;
	}

	return value;
}
