#pragma once

#define COM_FUNC virtual HRESULT STDMETHODCALLTYPE
#define COM_FUNC_VOID virtual void STDMETHODCALLTYPE
#define COM_FUNC_RET(returnType) virtual returnType STDMETHODCALLTYPE

namespace ff
{
	class AppGlobals;

	template class UTIL_API std::function<HRESULT(IUnknown *pParent, REFGUID clsid, REFGUID iid, void **ppObj)>;
	typedef std::function<HRESULT(IUnknown *pParent, REFGUID clsid, REFGUID iid, void **ppObj)> ClassFactoryFunc;
	typedef std::function<bool(AppGlobals *globals, IUnknown **obj)> ParentFactoryFunc;

	template<typename T, typename I = T>
	inline T *GetAddRef(T *obj)
	{
		if (obj != nullptr)
		{
			static_cast<I *>(obj)->AddRef();
		}

		return obj;
	}

	template<typename T>
	inline void ReleaseRef(T *&obj)
	{
		if (obj != nullptr)
		{
			obj->Release();
			obj = nullptr;
		}
	}
}

inline bool operator<(REFGUID l, REFGUID r)
{
	return memcmp(&l, &r, sizeof(GUID)) < 0;
}

inline bool operator<=(REFGUID l, REFGUID r)
{
	return memcmp(&l, &r, sizeof(GUID)) <= 0;
}

inline bool operator>(REFGUID l, REFGUID r)
{
	return memcmp(&l, &r, sizeof(GUID)) > 0;
}

inline bool operator>=(REFGUID l, REFGUID r)
{
	return memcmp(&l, &r, sizeof(GUID)) >= 0;
}

inline ff::hash_t ShortenGuid(REFGUID id)
{
	// Only use this for comparing types, the values aren't good for hash tables
	const ff::hash_t *p = (const ff::hash_t *)&id;
	return p[0] ^ p[1];
}
