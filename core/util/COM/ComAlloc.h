#pragma once

#include "COM/ComObject.h"

namespace ff
{
	template<typename T, typename TClass = ComObject<T>, typename TAllocator = MemAllocator<TClass>>
	struct ComAllocator
	{
		typedef ComAllocator<T, TClass, TAllocator> MyType;

		static HRESULT CreateInstance(IUnknown *pParent, T **ppObj)
		{
			assertRetVal(ppObj, E_INVALIDARG);
			*ppObj = nullptr;

			HRESULT hr = S_OK;
			TClass *obj = GetAddRef(TAllocator().NewOne());
			if (obj != nullptr && SUCCEEDED(obj->_Construct(pParent)))
			{
				*ppObj = obj;
				return S_OK;
			}
			else if (obj != nullptr)
			{
				ReleaseRef(obj);
				return E_FAIL;
				
			}

			return E_OUTOFMEMORY;
		}

		static HRESULT CreateInstance(T **ppObj)
		{
			return CreateInstance(nullptr, ppObj);
		}

		static HRESULT CreateInstance(IUnknown *pParent, REFGUID clsid, REFGUID iid, void **ppObj)
		{
			T *pObj = nullptr;

			HRESULT hr = (clsid == GUID_NULL || clsid == __uuidof(T))
				? CreateInstance(pParent, &pObj)
				: CLASS_E_CLASSNOTAVAILABLE;

			TClass *pComObj = (TClass *)pObj;

			if (SUCCEEDED(hr))
			{
				if (clsid == iid)
				{
					*ppObj = (T *)pObj;
					return S_OK;
				}
				else
				{
					hr = pComObj->QueryInterface(iid, ppObj);
				}
			}

			ReleaseRef(pComObj);
			return hr;
		}

		static HRESULT ComClassFactory(IUnknown *pParent, REFGUID clsid, REFGUID iid, void **ppObj)
		{
			return CreateInstance(pParent, clsid, iid, ppObj);
		}
	};
}
