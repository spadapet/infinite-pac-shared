#pragma once

namespace ff
{
	UTIL_API GUID CreateGuid();

	template<typename T>
	HRESULT CreateInProc(REFGUID clsid, T **pp)
	{
		return ::CoCreateInstance(clsid, nullptr, CLSCTX_INPROC_SERVER, __uuidof(T), (void **)pp);
	}
}

#define DECLARE_HEADER(className) \
	className(); \
	virtual ~className(); \
	virtual HRESULT _QueryInterface(REFIID iid, void **ppv) override;

#define DECLARE_API_HEADER(className) \
	UTIL_API className(); \
	UTIL_API virtual ~className(); \
	UTIL_API virtual HRESULT _QueryInterface(REFIID iid, void **ppv) override;

#define DECLARE_HEADER_AND_TYPELIB(className, typeLibResourceId) \
	DECLARE_HEADER(className) \
	virtual size_t _GetTypeLibIndex() const override { return typeLibResourceId; }

#define DECLARE_HEADER_AND_DEFAULT_TYPELIB(className) \
	DECLARE_HEADER_AND_TYPELIB(className, 1)

#define BEGIN_INTERFACES(className) \
	HRESULT className::_QueryInterface(REFIID iid, void **ppv) { \
		if (0) { }

#define HAS_INTERFACE(ifaceName) \
	else if(iid == __uuidof(ifaceName)) *ppv = static_cast<ifaceName *>(this);

#define HAS_INTERFACE2(ifaceName, fromName) \
	else if(iid == __uuidof(ifaceName)) *ppv = static_cast<ifaceName *>(static_cast<fromName *>(this));

#define PARENT_INTERFACES(parentClass) \
	else if(iid == __uuidof(parentClass)) *ppv = static_cast<parentClass *>(this); \
	else if (SUCCEEDED(parentClass::_QueryInterface(iid, ppv))) return S_OK;

#define END_INTERFACES() \
	else { *ppv = nullptr; return E_NOINTERFACE; } \
	_GetUnknown()->AddRef(); \
	return S_OK; }

#define DECLARE_CONNECTION_POINTS() \
	virtual IConnectionPointHolder *_GetConnectionPointHolder(size_t index) override;

#define BEGIN_CONNECTION_POINTS(className) \
	ff::IConnectionPointHolder *className::_GetConnectionPointHolder(size_t index) \
	{

#define HAS_CONNECTION_POINT(varName) \
		if (index == 0) return &varName; else index--;

#define END_WITH_PARENT_CONNECTION_POINTS(parentClass) \
		return parentClass::_GetConnectionPointHolder(index); \
	}

#define END_CONNECTION_POINTS() \
		return nullptr; \
	}
