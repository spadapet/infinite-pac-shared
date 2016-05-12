#include "pch.h"
#include "Data/Data.h"
#include "Data/SavedData.h"
#include "Data/DataWriterReader.h"
#include "Dict/Dict.h"
#include "Dict/DictPersist.h"
#include "Dict/Value.h"
#include "Resource/ResourceValue.h"
#include "String/StringUtil.h"

struct FakeVector { size_t fake[4]; };
static ff::PoolAllocator<FakeVector> s_fakeVectorPool;
static ff::PoolAllocator<ff::Value> s_valuePool;

class ScopeStaticValueAlloc
{
public:
	ScopeStaticValueAlloc() : _lock(ff::GCS_VALUE) { }

private:
	ff::ScopeStaticMemAlloc _staticAlloc;
	ff::LockMutex _lock;
};

template<typename T>
static T *NewFakeVector()
{
	assert(sizeof(FakeVector) >= sizeof(T));

	FakeVector *pFake = s_fakeVectorPool.New();
	T *pVector = ::new((void*)pFake) T;

	return pVector;
}

template<typename T>
static T *NewFakeVector(T &&value)
{
	assert(sizeof(FakeVector) >= sizeof(T));

	FakeVector *pFake = s_fakeVectorPool.New();
	T *pVector = ::new((void*)pFake) T(std::move(value));

	return pVector;
}

template<typename T>
static void DeleteFakeVector(T *pVector)
{
	if (pVector)
	{
		pVector->~T();

		FakeVector *pFake = (FakeVector*)pVector;
		s_fakeVectorPool.Delete(pFake);
	}
}

ff::Value::Value()
	: _refCount(-1)
	, _type(Type::Null)
{
}

ff::Value::~Value()
{
	assert(_refCount == -1 || !_refCount);

	switch (GetType())
	{
		case Type::Object:
			ff::ReleaseRef(_object);
			break;

		case Type::String:
			InternalGetString().~String();
			break;

		case Type::Dict:
			_dict.AsDict()->~Dict();
			break;

		case Type::Data:
			ff::ReleaseRef(_data);
			break;

		case Type::SavedData:
		case Type::SavedDict:
			ff::ReleaseRef(_savedData);
			break;

		case Type::StringVector:
			DeleteFakeVector(_stringVector);
			break;

		case Type::ValueVector:
			DeleteFakeVector(_valueVector);
			break;

		case Type::IntVector:
			DeleteFakeVector(_intVector);
			break;

		case Type::DoubleVector:
			DeleteFakeVector(_doubleVector);
			break;

		case Type::FloatVector:
			DeleteFakeVector(_floatVector);
			break;

		case Type::DataVector:
			DeleteFakeVector(_dataVector);
			break;

		case Type::Resource:
			_resource.AsResource()->~SharedResourceValue();
			break;
	}
}

ff::Value *ff::Value::NewValueOneRef()
{
	Value *pVal = s_valuePool.New();
	pVal->_refCount = 1;

	return pVal;
}

void ff::Value::DeleteValue(Value *val)
{
	s_valuePool.Delete(val);
}

void ff::Value::AddRef()
{
	if (_refCount != -1)
	{
		InterlockedIncrement(&_refCount);
	}
}

void ff::Value::Release()
{
	if (_refCount != -1 && !InterlockedDecrement(&_refCount))
	{
		DeleteValue(this);
	}
}

ff::Value::Type ff::Value::GetType() const
{
	return _type;
}

bool ff::Value::IsType(Type type) const
{
	return GetType() == type;
}

bool ff::Value::IsNumberType() const
{
	switch (GetType())
	{
	case Type::Double:
	case Type::Float:
	case Type::Int:
		return true;

	default:
		return false;
	}
}

bool ff::Value::CreateNull(Value **ppValue)
{
	assertRetVal(ppValue, false);

	static Value::StaticValue s_data;
	static Value *s_val = nullptr;

	if (!s_val)
	{
		ScopeStaticValueAlloc scopeAlloc;

		if (!s_val)
		{
			s_val = s_data.AsValue();
			s_val->Value::Value();
			s_val->SetType(Type::Null);
		}
	}

	*ppValue = s_val;

	return true;
}

bool ff::Value::CreateBool(bool val, Value **ppValue)
{
	assertRetVal(ppValue, false);

	static Value::StaticValue s_data[2];
	static bool s_init = false;

	if (!s_init)
	{
		ScopeStaticValueAlloc scopeAlloc;

		if (!s_init)
		{
			Value *pVal = s_data[0].AsValue();
			pVal->Value::Value();
			pVal->SetType(Type::Bool);
			pVal->_bool = false;

			pVal = s_data[1].AsValue();
			pVal->Value::Value();
			pVal->SetType(Type::Bool);
			pVal->_bool = true;

			s_init = true;
		}
	}

	*ppValue = s_data[val ? 1 : 0].AsValue();

	return true;
}

bool ff::Value::CreateDouble(double val, Value **ppValue)
{
	assertRetVal(ppValue, false);

	if (val)
	{
		*ppValue = NewValueOneRef();
		(*ppValue)->SetType(Type::Double);
		(*ppValue)->_double = val;
	}
	else
	{
		static Value::StaticValue s_data;
		static Value *s_val = nullptr;

		if (!s_val)
		{
			ScopeStaticValueAlloc scopeAlloc;

			if (!s_val)
			{
				s_val = s_data.AsValue();
				s_val->Value::Value();
				s_val->SetType(Type::Double);
				s_val->_double = 0;
			}
		}

		*ppValue = s_val;
	}

	return true;
}

bool ff::Value::CreateFloat(float val, Value **ppValue)
{
	assertRetVal(ppValue, false);

	if (val)
	{
		*ppValue = NewValueOneRef();
		(*ppValue)->SetType(Type::Float);
		(*ppValue)->_float = val;
	}
	else
	{
		static Value::StaticValue s_data;
		static Value *s_val = nullptr;

		if (!s_val)
		{
			ScopeStaticValueAlloc scopeAlloc;

			if (!s_val)
			{
				s_val = s_data.AsValue();
				s_val->Value::Value();
				s_val->SetType(Type::Float);
				s_val->_float = 0;
			}
		}

		*ppValue = s_val;
	}

	return true;
}

bool ff::Value::CreateInt(int val, Value **ppValue)
{
	assertRetVal(ppValue, false);

	if (val < -100 || val > 200)
	{
		*ppValue = NewValueOneRef();
		(*ppValue)->SetType(Type::Int);
		(*ppValue)->_int = val;
	}
	else
	{
		static Value::StaticValue s_data[301];
		static bool s_init = false;

		if (!s_init)
		{
			ScopeStaticValueAlloc scopeAlloc;

			if (!s_init)
			{
				for (int i = 0; i < 301; i++)
				{
					Value *pVal = s_data[i].AsValue();
					pVal->Value::Value();
					pVal->SetType(Type::Int);
					pVal->_int = i - 100;
				}

				s_init = true;
			}
		}

		*ppValue = s_data[val + 100].AsValue();
	}

	return true;
}

bool ff::Value::CreateObject(IUnknown *pObj, Value **ppValue)
{
	assertRetVal(ppValue, false);

	if (pObj)
	{
		*ppValue = NewValueOneRef();
		(*ppValue)->SetType(Type::Object);
		(*ppValue)->_object = GetAddRef(pObj);
	}
	else
	{
		static Value::StaticValue s_data;
		static Value *s_val = nullptr;

		if (!s_val)
		{
			ScopeStaticValueAlloc scopeAlloc;

			if (!s_val)
			{
				s_val = s_data.AsValue();
				s_val->Value::Value();
				s_val->SetType(Type::Object);
				s_val->_object = nullptr;
			}
		}

		*ppValue = s_val;
	}

	return true;
}

bool ff::Value::CreatePoint(const PointInt &point, Value **ppValue)
{
	assertRetVal(ppValue, false);

	if (point.x || point.y)
	{
		*ppValue = NewValueOneRef();
		(*ppValue)->SetType(Type::Point);
		(*ppValue)->InternalGetPoint() = point;
	}
	else
	{
		static Value::StaticValue s_data;
		static Value *s_val = nullptr;

		if (!s_val)
		{
			ScopeStaticValueAlloc scopeAlloc;

			if (!s_val)
			{
				s_val = s_data.AsValue();
				s_val->Value::Value();
				s_val->SetType(Type::Point);
				s_val->InternalGetPoint().SetPoint(0, 0);
			}
		}

		*ppValue = s_val;
	}

	return true;
}

bool ff::Value::CreatePointF(const PointFloat &point, Value **ppValue)
{
	assertRetVal(ppValue, false);

	if (point.x || point.y)
	{
		*ppValue = NewValueOneRef();
		(*ppValue)->SetType(Type::PointF);
		(*ppValue)->InternalGetPointF() = point;
	}
	else
	{
		static Value::StaticValue s_data;
		static Value *s_val = nullptr;

		if (!s_val)
		{
			ScopeStaticValueAlloc scopeAlloc;

			if (!s_val)
			{
				s_val = s_data.AsValue();
				s_val->Value::Value();
				s_val->SetType(Type::PointF);
				s_val->InternalGetPointF().SetPoint(0, 0);
			}
		}

		*ppValue = s_val;
	}

	return true;
}

bool ff::Value::CreateRect(const RectInt &rect, Value **ppValue)
{
	assertRetVal(ppValue, false);

	if (!rect.IsNull())
	{
		*ppValue = NewValueOneRef();
		(*ppValue)->SetType(Type::Rect);
		(*ppValue)->InternalGetRect() = rect;
	}
	else
	{
		static Value::StaticValue s_data;
		static Value *s_val = nullptr;

		if (!s_val)
		{
			ScopeStaticValueAlloc scopeAlloc;

			if (!s_val)
			{
				s_val = s_data.AsValue();
				s_val->Value::Value();
				s_val->SetType(Type::Rect);
				s_val->InternalGetRect().SetRect(0, 0, 0, 0);
			}
		}

		*ppValue = s_val;
	}

	return true;
}

bool ff::Value::CreateRectF(const RectFloat &rect, Value **ppValue)
{
	assertRetVal(ppValue, false);

	if (!rect.IsNull())
	{
		*ppValue = NewValueOneRef();
		(*ppValue)->SetType(Type::RectF);
		(*ppValue)->InternalGetRectF() = rect;
	}
	else
	{
		static Value::StaticValue s_data;
		static Value *s_val = nullptr;

		if (!s_val)
		{
			ScopeStaticValueAlloc scopeAlloc;

			if (!s_val)
			{
				s_val = s_data.AsValue();
				s_val->Value::Value();
				s_val->SetType(Type::RectF);
				s_val->InternalGetRectF().SetRect(0, 0, 0, 0);
			}
		}

		*ppValue = s_val;
	}

	return true;
}

bool ff::Value::CreateString(StringRef str, Value **ppValue)
{
	assertRetVal(ppValue, false);

	if (!str.length())
	{
		static Value::StaticValue s_data;
		static Value *s_val = nullptr;

		if (!s_val)
		{
			ScopeStaticValueAlloc scopeAlloc;

			if (!s_val)
			{
				s_val = s_data.AsValue();
				s_val->Value::Value();
				s_val->SetType(Type::String);
				s_val->InternalGetString().String::String();
			}
		}

		*ppValue = s_val;
	}
	else
	{
		*ppValue = NewValueOneRef();
		(*ppValue)->SetType(Type::String);
		(*ppValue)->InternalGetString().String::String(str);
	}

	return true;
}

bool ff::Value::CreateStringVector(Vector<String> &&vec, Value **ppValue)
{
	assertRetVal(ppValue, false);

	*ppValue = NewValueOneRef();
	(*ppValue)->SetType(Type::StringVector);
	(*ppValue)->_stringVector = NewFakeVector<Vector<String>>(std::move(vec));

	return true;
}

bool ff::Value::CreateValueVector(Vector<ValuePtr> &&vec, Value **ppValue)
{
	assertRetVal(ppValue, false);

	*ppValue = NewValueOneRef();
	(*ppValue)->SetType(Type::ValueVector);
	(*ppValue)->_valueVector = NewFakeVector<Vector<ValuePtr>>(std::move(vec));

	return true;
}

bool ff::Value::CreateResource(const SharedResourceValue &res, Value **ppValue)
{
	assertRetVal(ppValue, false);

	*ppValue = NewValueOneRef();
	(*ppValue)->SetType(Type::Resource);
	::new((*ppValue)->_resource.AsResource()) ff::SharedResourceValue(res);

	return true;
}

bool ff::Value::CreateGuid(REFGUID guid, Value **ppValue)
{
	assertRetVal(ppValue, false);

	if (guid != GUID_NULL)
	{
		*ppValue = NewValueOneRef();
		(*ppValue)->SetType(Type::Guid);
		(*ppValue)->_guid = guid;
	}
	else
	{
		static Value::StaticValue s_data;
		static Value *s_val = nullptr;

		if (!s_val)
		{
			ScopeStaticValueAlloc scopeAlloc;

			if (!s_val)
			{
				s_val = s_data.AsValue();
				s_val->Value::Value();
				s_val->SetType(Type::Guid);
				s_val->_guid = GUID_NULL;
			}
		}

		*ppValue = s_val;
	}

	return true;
}

bool ff::Value::CreateIntVector(Vector<int> &&vec, Value **ppValue)
{
	assertRetVal(ppValue, false);

	*ppValue = NewValueOneRef();
	(*ppValue)->SetType(Type::IntVector);
	(*ppValue)->_intVector = NewFakeVector<Vector<int>>(std::move(vec));

	return true;
}

bool ff::Value::CreateDoubleVector(Vector<double> &&vec, Value **ppValue)
{
	assertRetVal(ppValue, false);

	*ppValue = NewValueOneRef();
	(*ppValue)->SetType(Type::DoubleVector);
	(*ppValue)->_doubleVector = NewFakeVector<Vector<double>>(std::move(vec));

	return true;
}

bool ff::Value::CreateFloatVector(Vector<float> &&vec, Value **ppValue)
{
	assertRetVal(ppValue, false);

	*ppValue = NewValueOneRef();
	(*ppValue)->SetType(Type::FloatVector);
	(*ppValue)->_floatVector = NewFakeVector<Vector<float>>(std::move(vec));

	return true;
}

bool ff::Value::CreateData(IData *pData, Value **ppValue)
{
	assertRetVal(ppValue, false);

	*ppValue = NewValueOneRef();
	(*ppValue)->SetType(Type::Data);
	(*ppValue)->_data = GetAddRef(pData);

	return true;
}

bool ff::Value::CreateDataVector(Vector<ComPtr<IData>> &&vec, Value **ppValue)
{
	assertRetVal(ppValue, false);

	*ppValue = NewValueOneRef();
	(*ppValue)->SetType(Type::DataVector);
	(*ppValue)->_dataVector = NewFakeVector<Vector<ComPtr<IData>>>(std::move(vec));

	return true;
}

bool ff::Value::CreateDict(ff::Dict &&dict, Value **ppValue)
{
	assertRetVal(ppValue, false);

	*ppValue = NewValueOneRef();
	(*ppValue)->SetType(Type::Dict);
	::new((*ppValue)->_dict.AsDict()) ff::Dict(std::move(dict));

	return true;
}

bool ff::Value::CreateSavedData(ff::ISavedData *data, Value **ppValue)
{
	assertRetVal(data && ppValue, false);

	*ppValue = NewValueOneRef();
	(*ppValue)->SetType(Type::SavedData);
	(*ppValue)->_savedData = ff::GetAddRef(data);

	return true;
}

bool ff::Value::CreateSavedDict(ff::ISavedData *data, Value **ppValue)
{
	assertRetVal(data && ppValue, false);

	*ppValue = NewValueOneRef();
	(*ppValue)->SetType(Type::SavedDict);
	(*ppValue)->_savedData = ff::GetAddRef(data);

	return true;
}

bool ff::Value::CreateSavedDict(const ff::Dict &dict, bool compress, Value **ppValue)
{
	ff::ComPtr<ff::IData> data;
	assertRetVal(ff::SaveDict(dict, &data), false);

	ff::ComPtr<ff::ISavedData> savedData;
	assertRetVal(ff::CreateLoadedDataFromMemory(data, compress, &savedData), false);

	return CreateSavedDict(savedData, ppValue);
}

bool ff::Value::AsBool() const
{
	assert(IsType(Type::Bool));
	return _bool;
}

double ff::Value::AsDouble() const
{
	assert(IsType(Type::Double));
	return _double;
}

float ff::Value::AsFloat() const
{
	assert(IsType(Type::Float));
	return _float;
}

int ff::Value::AsInt() const
{
	assert(IsType(Type::Int));
	return _int;
}

IUnknown *ff::Value::AsObject() const
{
	assert(IsType(Type::Object));
	return _object;
}

const ff::PointInt &ff::Value::AsPoint() const
{
	assert(IsType(Type::Point));
	return InternalGetPoint();
}

const ff::PointFloat &ff::Value::AsPointF() const
{
	assert(IsType(Type::PointF));
	return InternalGetPointF();
}

const ff::RectInt &ff::Value::AsRect() const
{
	assert(IsType(Type::Rect));
	return InternalGetRect();
}

const ff::RectFloat &ff::Value::AsRectF() const
{
	assert(IsType(Type::RectF));
	return InternalGetRectF();
}

const ff::Vector<ff::String> &ff::Value::AsStringVector() const
{
	assert(IsType(Type::StringVector));
	return *_stringVector;
}

const ff::Vector<ff::ValuePtr> &ff::Value::AsValueVector() const
{
	assert(IsType(Type::ValueVector));
	return *_valueVector;
}

REFGUID ff::Value::AsGuid() const
{
	assert(IsType(Type::Guid));
	return _guid;
}

const ff::Vector<int> &ff::Value::AsIntVector() const
{
	assert(IsType(Type::IntVector));
	return *_intVector;
}

const ff::Vector<double> &ff::Value::AsDoubleVector() const
{
	assert(IsType(Type::DoubleVector));
	return *_doubleVector;
}

const ff::Vector<float> &ff::Value::AsFloatVector() const
{
	assert(IsType(Type::FloatVector));
	return *_floatVector;
}

ff::IData *ff::Value::AsData() const
{
	assert(IsType(Type::Data));
	return _data;
}

ff::ISavedData *ff::Value::AsSavedData() const
{
	assert(IsType(Type::SavedData) || IsType(Type::SavedDict));
	return _savedData;
}

const ff::Vector<ff::ComPtr<ff::IData>> &ff::Value::AsDataVector() const
{
	assert(IsType(Type::DataVector));
	return *_dataVector;
}

void ff::Value::SetType(Type type)
{
	_type = type;
}

ff::PointInt &ff::Value::InternalGetPoint() const
{
	assert(sizeof(_point) >= sizeof(PointInt));
	return *(PointInt*)&_point;
}

ff::PointFloat &ff::Value::InternalGetPointF() const
{
	assert(sizeof(_pointF) >= sizeof(PointFloat));
	return *(PointFloat*)&_pointF;
}

ff::RectInt &ff::Value::InternalGetRect() const
{
	assert(sizeof(_rect) >= sizeof(RectInt));
	return *(RectInt*)&_rect;
}

ff::RectFloat &ff::Value::InternalGetRectF() const
{
	assert(sizeof(_rectF) >= sizeof(RectFloat));
	return *(RectFloat*)&_rectF;
}

ff::StringOut ff::Value::InternalGetString() const
{
	assert(sizeof(_string) >= sizeof(String));
	return *(String*)&_string;
}

ff::Dict *ff::Value::SDict::AsDict() const
{
	assert(sizeof(data) >= sizeof(ff::Dict));
	return (ff::Dict*)&data;
}

ff::SharedResourceValue *ff::Value::SResource::AsResource() const
{
	assert(sizeof(data) >= sizeof(ff::SharedResourceValue));
	return (ff::SharedResourceValue*)&data;
}

ff::Value *ff::Value::StaticValue::AsValue() const
{
	assert(sizeof(data) >= sizeof(Value));
	return (Value*)&data;
}

ff::StringRef ff::Value::AsString() const
{
	assert(IsType(Type::String));
	return InternalGetString();
}

const ff::Dict &ff::Value::AsDict() const
{
	assert(IsType(Type::Dict));
	return *_dict.AsDict();
}

const ff::SharedResourceValue &ff::Value::AsResource() const
{
	assert(IsType(Type::Resource));
	return *_resource.AsResource();
}

bool ff::Value::Convert(Type type, Value **ppValue) const
{
	assertRetVal(ppValue, false);
	*ppValue = nullptr;

	if (type == GetType())
	{
		*ppValue = GetAddRef(const_cast<Value *>(this));
		return true;
	}

	wchar_t buf[256];

	switch (GetType())
	{
	case Type::Null:
		switch (type)
		{
		case Type::String:
			CreateString(String(L"null"), ppValue);
			break;
		}
		break;

	case Type::Bool:
		switch (type)
		{
		case Type::Double:
			CreateDouble(AsBool() ? 1.0 : 0.0, ppValue);
			break;

		case Type::Float:
			CreateFloat(AsBool() ? 1.0f : 0.0f, ppValue);
			break;

		case Type::Int:
			CreateInt(AsBool() ? 1 : 0, ppValue);
			break;

		case Type::String:
			CreateString(AsBool() ? String(L"true") : String(L"false"), ppValue);
			break;
		}
		break;

	case Type::Double:
		switch (type)
		{
		case Type::Bool:
			CreateBool(AsDouble() != 0, ppValue);
			break;

		case Type::Float:
			CreateFloat((float)AsDouble(), ppValue);
			break;

		case Type::Int:
			CreateInt((int)AsDouble(), ppValue);
			break;

		case Type::String:
			_snwprintf_s(buf, _countof(buf), _TRUNCATE, L"%g", AsDouble());
			CreateString(String(buf), ppValue);
			break;
		}
		break;

	case Type::Float:
		switch (type)
		{
		case Type::Bool:
			CreateBool(AsFloat() != 0, ppValue);
			break;

		case Type::Double:
			CreateDouble((double)AsFloat(), ppValue);
			break;

		case Type::Int:
			CreateInt((int)AsFloat(), ppValue);
			break;

		case Type::String:
			_snwprintf_s(buf, _countof(buf), _TRUNCATE, L"%g", AsFloat());
			CreateString(String(buf), ppValue);
			break;
		}
		break;

	case Type::Int:
		switch (type)
		{
		case Type::Bool:
			CreateBool(AsInt() != 0, ppValue);
			break;

		case Type::Double:
			CreateDouble((double)AsInt(), ppValue);
			break;

		case Type::Float:
			CreateFloat((float)AsInt(), ppValue);
			break;

		case Type::String:
			_snwprintf_s(buf, _countof(buf), _TRUNCATE, L"%d", AsInt());
			CreateString(String(buf), ppValue);
			break;
		}
		break;

	case Type::Point:
		switch (type)
		{
		case Type::String:
			_snwprintf_s(buf, _countof(buf), _TRUNCATE, L"(%d,%d)", AsPoint().x, AsPoint().y);
			CreateString(String(buf), ppValue);
			break;
		}
		break;

	case Type::PointF:
		switch (type)
		{
		case Type::String:
			_snwprintf_s(buf, _countof(buf), _TRUNCATE, L"(%g,%g)", AsPointF().x, AsPointF().y);
			CreateString(String(buf), ppValue);
			break;
		}
		break;

	case Type::Rect:
		switch (type)
		{
		case Type::String:
			_snwprintf_s(buf, _countof(buf), _TRUNCATE, L"(%d,%d,%d,%d)",
				AsRect().left, AsRect().top, AsRect().right, AsRect().bottom);
			CreateString(String(buf), ppValue);
			break;
		}
		break;

	case Type::RectF:
		switch (type)
		{
		case Type::String:
			_snwprintf_s(buf, _countof(buf), _TRUNCATE, L"(%g,%g,%g,%g)",
				AsRectF().left, AsRectF().top, AsRectF().right, AsRectF().bottom);
			CreateString(String(buf), ppValue);
			break;
		}
		break;

	case Type::String:
		switch (type)
		{
		case Type::Bool:
			if (AsString().empty() || AsString() == L"false" || AsString() == L"no" || AsString() == L"0")
			{
				CreateBool(false, ppValue);
			}
			else if (AsString() == L"true" || AsString() == L"yes" || AsString() == L"1")
			{
				CreateBool(true, ppValue);
			}
			break;

		case Type::Double:
			{
				const wchar_t *start = AsString().c_str();
				wchar_t *end = nullptr;
				double val = wcstod(start, &end);

				if (end > start && !*end)
				{
					CreateDouble(val, ppValue);
				}
			} break;

		case Type::Float:
			{
				const wchar_t *start = AsString().c_str();
				wchar_t *end = nullptr;
				double val = wcstod(start, &end);

				if (end > start && !*end)
				{
					CreateFloat((float)val, ppValue);
				}
			} break;

		case Type::Int:
			{
				const wchar_t *start = AsString().c_str();
				wchar_t *end = nullptr;
				long val = wcstol(start, &end, 10);

				if (end > start && !*end)
				{
					CreateInt((int)val, ppValue);
				}
			} break;

		case Type::Guid:
			{
				GUID guid;
				if (StringToGuid(AsString(), guid))
				{
					CreateGuid(guid, ppValue);
				}
			} break;
		}
		break;

	case Type::Object:
		switch (type)
		{
		case Type::Bool:
			CreateBool(AsObject() != nullptr, ppValue);
			break;
		}
		break;

	case Type::Guid:
		switch (type)
		{
		case Type::String:
			CreateString(StringFromGuid(AsGuid()), ppValue);
			break;
		}
		break;

	case Type::Data:
		switch (type)
		{
		case Type::SavedData:
			{
				ff::ComPtr<ff::ISavedData> savedData;
				if (ff::CreateLoadedDataFromMemory(AsData(), false, &savedData))
				{
					CreateSavedData(savedData, ppValue);
				}
			}
			break;
		}
		break;

	case Type::SavedData:
		switch (type)
		{
		case Type::Data:
			{
				ff::ComPtr<ff::ISavedData> savedData;
				if (AsSavedData()->Clone(&savedData))
				{
					ff::ComPtr<ff::IData> data = savedData->Load();
					if (data)
					{
						CreateData(data, ppValue);
					}
				}
			}
			break;
		}
		break;

	case Type::Dict:
		switch (type)
		{
		case Type::Data:
		case Type::SavedData:
		case Type::SavedDict:
			{
				ff::ComPtr<ff::IData> data;
				ff::ComPtr<ff::ISavedData> savedData;

				if (ff::SaveDict(AsDict(), &data))
				{
					if (type == Type::Data)
					{
						CreateData(data, ppValue);
					}
					else if (ff::CreateLoadedDataFromMemory(data, true, &savedData))
					{
						if (type == Type::SavedData)
						{
							CreateSavedData(savedData, ppValue);
						}
						else
						{
							CreateSavedDict(savedData, ppValue);
						}
					}
				}
			}
			break;
		}
		break;

	case Type::SavedDict:
		switch (type)
		{
		case Type::Data:
			{
				ff::ComPtr<ff::ISavedData> savedData;
				if (AsSavedData()->Clone(&savedData))
				{
					ff::ComPtr<ff::IData> data = savedData->Load();
					if (data)
					{
						CreateData(data, ppValue);
					}
				}
			}
			break;

		case Type::SavedData:
			CreateSavedData(AsSavedData(), ppValue);
			break;

		case Type::Dict:
			{
				ff::ComPtr<ff::ISavedData> savedData;
				if (AsSavedData()->Clone(&savedData))
				{
					ff::ComPtr<ff::IData> data = savedData->Load();
					ff::ComPtr<ff::IDataReader> dataReader;
					Dict dict;

					if (data && ff::CreateDataReader(data, 0, &dataReader) && ff::LoadDict(dataReader, dict))
					{
						CreateDict(std::move(dict), ppValue);
					}
				}
			}
			break;
		}
		break;

	case Type::Resource:
		AsResource()->GetValue()->Convert(type, ppValue);
		break;

	case Type::IntVector:
		switch (type)
		{
		case Type::Point:
			if (AsIntVector().Size() == 2)
			{
				PointInt point(
					AsIntVector().GetAt(0),
					AsIntVector().GetAt(1));
				CreatePoint(point, ppValue);
			}
			break;

		case Type::Rect:
			if (AsIntVector().Size() == 4)
			{
				RectInt rect(
					AsIntVector().GetAt(0),
					AsIntVector().GetAt(1),
					AsIntVector().GetAt(2),
					AsIntVector().GetAt(3));
				CreateRect(rect, ppValue);
			}
			break;
		}
		break;

	case Type::FloatVector:
		switch (type)
		{
		case Type::PointF:
			if (AsFloatVector().Size() == 2)
			{
				PointFloat point(
					AsFloatVector().GetAt(0),
					AsFloatVector().GetAt(1));
				CreatePointF(point, ppValue);
			}
			break;

		case Type::RectF:
			if (AsFloatVector().Size() == 4)
			{
				RectFloat rect(
					AsFloatVector().GetAt(0),
					AsFloatVector().GetAt(1),
					AsFloatVector().GetAt(2),
					AsFloatVector().GetAt(3));
				CreateRectF(rect, ppValue);
			}
			break;
		}
		break;

	case Type::ValueVector:
		switch (type)
		{
		case Type::Point:
			if (AsValueVector().Size() == 2)
			{
				ValuePtr newValues[2];
				const Vector<ValuePtr> &values = AsValueVector();
				if (values[0]->Convert(Type::Int, &newValues[0]) &&
					values[1]->Convert(Type::Int, &newValues[1]))
				{
					CreatePoint(PointInt(newValues[0]->AsInt(), newValues[1]->AsInt()), ppValue);
				}
			}
			break;

		case Type::PointF:
			if (AsValueVector().Size() == 2)
			{
				ValuePtr newValues[2];
				const Vector<ValuePtr> &values = AsValueVector();
				if (values[0]->Convert(Type::Float, &newValues[0]) &&
					values[1]->Convert(Type::Float, &newValues[1]))
				{
					CreatePointF(PointFloat(newValues[0]->AsFloat(), newValues[1]->AsFloat()), ppValue);
				}
			}
			break;

		case Type::Rect:
			if (AsValueVector().Size() == 4)
			{
				ValuePtr newValues[4];
				const Vector<ValuePtr> &values = AsValueVector();
				if (values[0]->Convert(Type::Int, &newValues[0]) &&
					values[1]->Convert(Type::Int, &newValues[1]) &&
					values[2]->Convert(Type::Int, &newValues[2]) &&
					values[3]->Convert(Type::Int, &newValues[3]))
				{
					CreateRect(RectInt(
						newValues[0]->AsInt(),
						newValues[1]->AsInt(),
						newValues[2]->AsInt(),
						newValues[3]->AsInt()), ppValue);
				}
			}
			break;

		case Type::RectF:
			if (AsValueVector().Size() == 4)
			{
				ValuePtr newValues[4];
				const Vector<ValuePtr> &values = AsValueVector();
				if (values[0]->Convert(Type::Float, &newValues[0]) &&
					values[1]->Convert(Type::Float, &newValues[1]) &&
					values[2]->Convert(Type::Float, &newValues[2]) &&
					values[3]->Convert(Type::Float, &newValues[3]))
				{
					CreateRectF(RectFloat(
						newValues[0]->AsFloat(),
						newValues[1]->AsFloat(),
						newValues[2]->AsFloat(),
						newValues[3]->AsFloat()), ppValue);
				}
			}
			break;

		case Type::StringVector:
			{
				bool valid = true;
				Vector<String> newValues;
				const Vector<ValuePtr> &values = AsValueVector();
				for (ValuePtr oldValue : values)
				{
					ValuePtr newValue;
					valid = oldValue->Convert(Type::String, &newValue);
					if (valid)
					{
						newValues.Push(newValue->AsString());
					}
					else
					{
						break;
					}
				}

				if (valid)
				{
					CreateStringVector(std::move(newValues), ppValue);
				}
			}
			break;

		case Type::IntVector:
			{
				bool valid = true;
				Vector<int> newValues;
				const Vector<ValuePtr> &values = AsValueVector();
				for (ValuePtr oldValue : values)
				{
					ValuePtr newValue;
					valid = oldValue->Convert(Type::Int, &newValue);
					if (valid)
					{
						newValues.Push(newValue->AsInt());
					}
					else
					{
						break;
					}
				}

				if (valid)
				{
					CreateIntVector(std::move(newValues), ppValue);
				}
			}
			break;

		case Type::DoubleVector:
			{
				bool valid = true;
				Vector<double> newValues;
				const Vector<ValuePtr> &values = AsValueVector();
				for (ValuePtr oldValue : values)
				{
					ValuePtr newValue;
					valid = oldValue->Convert(Type::Double, &newValue);
					if (valid)
					{
						newValues.Push(newValue->AsDouble());
					}
					else
					{
						break;
					}
				}

				if (valid)
				{
					CreateDoubleVector(std::move(newValues), ppValue);
				}
			}
			break;

		case Type::FloatVector:
			{
				bool valid = true;
				Vector<float> newValues;
				const Vector<ValuePtr> &values = AsValueVector();
				for (ValuePtr oldValue : values)
				{
					ValuePtr newValue;
					valid = oldValue->Convert(Type::Float, &newValue);
					if (valid)
					{
						newValues.Push(newValue->AsFloat());
					}
					else
					{
						break;
					}
				}

				if (valid)
				{
					CreateFloatVector(std::move(newValues), ppValue);
				}
			}
			break;
		}
		break;
	}

	if (*ppValue)
	{
		return true;
	}

	return false;
}

bool ff::Value::operator==(const Value &r) const
{
	if (this == &r)
	{
		return true;
	}

	if (GetType() != r.GetType())
	{
		return false;
	}

	switch (GetType())
	{
		case Type::Null:
			return true;

		case Type::Bool:
			return AsBool() == r.AsBool();

		case Type::Double:
			return AsDouble() == r.AsDouble();

		case Type::Float:
			return AsFloat() == r.AsFloat();

		case Type::Int:
			return AsInt() == r.AsInt();

		case Type::Object:
			return AsObject() == r.AsObject();

		case Type::Point:
			return AsPoint() == r.AsPoint();

		case Type::PointF:
			return AsPointF() == r.AsPointF();

		case Type::Rect:
			return AsRect() == r.AsRect();

		case Type::RectF:
			return AsRectF() == r.AsRectF();

		case Type::String:
			return AsString() == r.AsString();

		case Type::StringVector:
			return AsStringVector() == r.AsStringVector();

		case Type::ValueVector:
			return AsValueVector() == r.AsValueVector();

		case Type::Guid:
			return AsGuid() == r.AsGuid() ? true : false;

		case Type::IntVector:
			return AsIntVector() == r.AsIntVector();

		case Type::DoubleVector:
			return AsDoubleVector() == r.AsDoubleVector();

		case Type::FloatVector:
			return AsFloatVector() == r.AsFloatVector();

		case Type::DataVector:
			return AsDataVector() == r.AsDataVector();

		case Type::Data:
			return AsData() == r.AsData();

		case Type::SavedData:
		case Type::SavedDict:
			return AsSavedData() == r.AsSavedData();

		default:
			assert(false);
			return false;
	}
}

bool ff::Value::Compare(const Value *p) const
{
	return p ? (*this == *p) : false;
}
