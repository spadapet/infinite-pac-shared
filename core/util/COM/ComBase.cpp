#include "pch.h"
#include "Module/Module.h"

#ifdef _DEBUG
#define COUNT_COM_OBJECTS
#endif

#ifdef COUNT_COM_OBJECTS
// STATIC_DATA (pod)
static long s_nComObjects = 0;

typedef ff::Map<ff::ComBaseEx *, size_t> GlobalComBaseMap;

// STATIC_DATA (object)
static GlobalComBaseMap s_allComObjects;
static GlobalComBaseMap &GetGlobalComObjectList()
{
	return s_allComObjects;
}
#endif

ff::ComBaseEx::ComBaseEx()
{
}

ff::ComBaseEx::~ComBaseEx()
{
}

HRESULT ff::ComBaseEx::_Construct(IUnknown *unkOuter)
{
	return S_OK;
}

void ff::ComBaseEx::_Destruct()
{
}

void ff::ComBaseEx::_DeleteThis()
{
	delete this;
}

void ff::ComBaseEx::_TrackLeak()
{
#ifdef COUNT_COM_OBJECTS
	LockMutex crit(GCS_COM_BASE);
	GetGlobalComObjectList().SetKey(this, InterlockedIncrement(&s_nComObjects));
#endif
}

void ff::ComBaseEx::_NoLeak()
{
#ifdef COUNT_COM_OBJECTS
	LockMutex crit(GCS_COM_BASE);
	BucketIter hp = GetGlobalComObjectList().Get(this);

	if (hp != INVALID_ITER)
	{
		GetGlobalComObjectList().DeletePos(hp);
	}
#endif
}

// static
void ff::ComBaseEx::_AddRefModule(const Module &module)
{
	module.AddRef();
}

// static
void ff::ComBaseEx::_ReleaseModule(const Module &module)
{
	module.Release();
}

const char *ff::ComBaseEx::_GetClassName() const
{
	return typeid(this).name();
}

size_t ff::ComBaseEx::_GetTypeLibIndex() const
{
	// Zero is for no type lib
	return 0;
}

HRESULT ff::ComBaseEx::_GetClassInfo(ITypeInfo **typeInfo) const
{
	assertRetVal(typeInfo, E_INVALIDARG);
	ITypeLib *typeLib = _GetSourceModule().GetTypeLib(_GetTypeLibIndex());
	return typeLib != nullptr ? typeLib->GetTypeInfoOfGuid(_GetClassID(), typeInfo) : E_FAIL;
}

ff::IConnectionPointHolder *ff::ComBaseEx::_GetConnectionPointHolder(size_t index)
{
	return nullptr;
}

HRESULT ff::ComBaseEx::_DispGetIDsOfNames(
	ITypeInfo *typeInfo,
	OLECHAR **names,
	UINT nameCount,
	DISPID *dispIds)
{
#if METRO_APP
	return E_FAIL;
#else
	return ::DispGetIDsOfNames(typeInfo, names, nameCount, dispIds);
#endif
}

HRESULT ff::ComBaseEx::_DispInvoke(
	IDispatch *disp,
	ITypeInfo *typeInfo,
	DISPID dispId,
	WORD flags,
	DISPPARAMS *params,
	VARIANT *result,
	EXCEPINFO *exceptionInfo,
	UINT *argumentError)
{
#if METRO_APP
	return E_FAIL;
#else
	return ::DispInvoke(disp, typeInfo, dispId, flags, params, result, exceptionInfo, argumentError);
#endif
}

void ff::ComBaseEx::DumpComObjects()
{
#ifdef COUNT_COM_OBJECTS
	LockMutex crit(GCS_COM_BASE);

	bool bLeaked = false;
	bool bDump = !GetGlobalComObjectList().IsEmpty();

	for(auto &kv: GetGlobalComObjectList())
	{
		ComBaseEx *obj = kv.GetKey();
		if(obj->_GetRefCount())
		{
			bLeaked = true;
			break;
		}
	}

	if (bDump)
	{
		if(bLeaked && IsDebuggerPresent())
		{
			__debugbreak();
		}

		OutputDebugString(L"Leaked COM Objects\n");
		OutputDebugString(L"------------------\n");

		wchar_t str[512] = L"";
		Vector<ComBaseEx *> sortedObjects;
		sortedObjects.Reserve(GetGlobalComObjectList().Size());

		for(auto &kv: GetGlobalComObjectList())
		{
			sortedObjects.Push(kv.GetKey());
		}

		std::sort(sortedObjects.begin(), sortedObjects.end(),
			[](ComBaseEx *lhs, ComBaseEx *rhs)
			{
				const GlobalComBaseMap &map = GetGlobalComObjectList();
				size_t alloc1 = map.ValueAt(map.Get(lhs));
				size_t alloc2 = map.ValueAt(map.Get(rhs));
				return alloc1 < alloc2;
			});

		for(ComBaseEx *obj: sortedObjects)
		{
			ff::BucketIter iter = GetGlobalComObjectList().Get(obj);
			size_t val = GetGlobalComObjectList().ValueAt(iter);

			_snwprintf_s(str, _countof(str), _TRUNCATE, 
				L"%Iu: 0x%Ix, %S, refs=%d\n",
				val, (size_t)obj, obj->_GetClassName(), obj->_GetRefCount());

			OutputDebugString(str);
		}

		OutputDebugString(L"------------------\n");
	}
#endif
}

ff::ComBase::ComBase()
	: _refs(0)
{
	_TrackLeak();
}

ff::ComBase::~ComBase()
{
	assert(!_refs);
	_NoLeak();
}

long ff::ComBase::_GetRefCount() const
{
	return InterlockedAccess(_refs);
}

ULONG ff::ComBase::_AddRef()
{
	return InterlockedIncrement(&_refs);
}

ULONG ff::ComBase::_Release()
{
	long ref = InterlockedDecrement(&_refs);
	assert(_refs >= 0);

	return ref;
}

ff::ComBaseUnsafe::ComBaseUnsafe()
	: _refs(0)
{
	_TrackLeak();
}

ff::ComBaseUnsafe::~ComBaseUnsafe()
{
	assert(!_refs);
	_NoLeak();
}

long ff::ComBaseUnsafe::_GetRefCount() const
{
	return _refs;
}

ULONG ff::ComBaseUnsafe::_AddRef()
{
	assert(_refs >= 0);
	return ++_refs;
}

ULONG ff::ComBaseUnsafe::_Release()
{
	assert(_refs > 0);
	return --_refs;
}

ff::ComBaseNoRef::ComBaseNoRef()
{
}

ff::ComBaseNoRef::~ComBaseNoRef()
{
}

long ff::ComBaseNoRef::_GetRefCount() const
{
	return 1;
}

ULONG ff::ComBaseNoRef::_AddRef()
{
	return 1;
}

ULONG ff::ComBaseNoRef::_Release()
{
	return 1;
}
