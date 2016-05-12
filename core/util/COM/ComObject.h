#pragma once

#include "COM/ComConnectionPoint.h"

namespace ff
{
	class Module;
	const Module &GetThisModule();

	// This is an interface that all of my COM objects will support
	class __declspec(uuid("802c66b2-fd91-4d34-b63a-d86be245ba3d")) __declspec(novtable)
		IComObject : public IDispatch
	{
	public:
		virtual REFGUID GetComClassID() const = 0;
		virtual const Module &GetComSourceModule() const = 0;
		virtual String GetComClassName() const = 0;
		virtual IConnectionPointHolder *GetComConnectionPointHolder(size_t index) = 0;
	};

	// A class to create COM objects of type T
	template<typename T>
	class ComObject
		: public T
		, public IComObject
		, public IConnectionPointContainer
#if !METRO_APP
		, public IProvideClassInfo
#endif
	{
	public:
		ComObject()
			: T()
		{
			_AddRefModule(GetThisModule());
		}

		virtual ~ComObject()
		{
			_ReleaseModule(GetThisModule());
		}

		// ComBase
		virtual IUnknown *_GetUnknown() override
		{
			return static_cast<IUnknown *>(static_cast<IComObject *>(this));
		}

		// ComBase
		virtual IDispatch *_GetDispatch() override
		{
			return static_cast<IDispatch *>(static_cast<IComObject *>(this));
		}

		// ComBase
		virtual REFGUID _GetClassID() const override
		{
			return __uuidof(T);
		}

		// ComBase
		virtual const Module &_GetSourceModule() const override
		{
			return GetThisModule();
		}

		// ComBase
		virtual const char *_GetClassName() const override
		{
			return typeid(T).name() + 6; // skip "class "
		}

		// IComObject
		virtual REFGUID GetComClassID() const override
		{
			return __uuidof(T);
		}

		// IComObject
		virtual const Module &GetComSourceModule() const override
		{
			return GetThisModule();
		}

		// IComObject
		virtual String GetComClassName() const override
		{
			return String::from_acp(_GetClassName());
		}

		// IComObject
		virtual IConnectionPointHolder *GetComConnectionPointHolder(size_t index) override
		{
			return _GetConnectionPointHolder(index);
		}

		// IUnknown
		COM_FUNC QueryInterface(REFIID iid, void **ppv) override
		{
			assertRetVal(ppv, E_INVALIDARG);

			if (iid == __uuidof(IUnknown))
			{
				*ppv = _GetUnknown();
			}
			else if (SUCCEEDED(_QueryInterface(iid, ppv)))
			{
				return S_OK;
			}
			else if (iid == __uuidof(T))
			{
				*ppv = this;
			}
			else if (iid == __uuidof(IComObject))
			{
				*ppv = static_cast<IComObject *>(this);
			}
			else if (iid == __uuidof(IConnectionPointContainer) && _GetConnectionPointHolder(0) != nullptr)
			{
				*ppv = static_cast<IConnectionPointContainer *>(this);
			}
#if !METRO_APP
			else if (iid == __uuidof(IDispatch) && _GetTypeLibIndex() != INVALID_SIZE)
			{
				*ppv = _GetDispatch();
			}
			else if (iid == __uuidof(IProvideClassInfo) && _GetTypeLibIndex() != INVALID_SIZE)
			{
				*ppv = static_cast<IProvideClassInfo *>(this);
			}
#endif
			else
			{
				return E_NOINTERFACE;
			}

			AddRef();

			return S_OK;
		}

		// IUnknown
		COM_FUNC_RET(ULONG) AddRef() override
		{
			return _AddRef();
		}

		// IUnknown
		COM_FUNC_RET(ULONG) Release() override
		{
			ULONG ref = _Release();
			if (ref == 0)
			{
				_AddRef();
				_Destruct();
				_Release();
				_DeleteThis();
			}

			return ref;
		}

		// IDispatch
		COM_FUNC GetTypeInfoCount(UINT *infoCount) override
		{
			assertRetVal(infoCount, E_INVALIDARG);
			*infoCount = 1;
			return S_OK;
		}

		// IDispatch
		COM_FUNC GetTypeInfo(UINT index, LCID lcid, ITypeInfo **typeInfo) override
		{
			assertRetVal(index == 0, DISP_E_BADINDEX);
			return _GetClassInfo(typeInfo);
		}

		// IDispatch
		COM_FUNC GetIDsOfNames(
			REFIID iid,
			OLECHAR **names,
			UINT nameCount,
			LCID lcid,
			DISPID *dispIds) override
		{
#if METRO_APP
			return E_FAIL;
#else
			ComPtr<ITypeInfo> typeInfo;
			return SUCCEEDED(GetTypeInfo(0, lcid, &typeInfo))
				? _DispGetIDsOfNames(typeInfo, names, nameCount, dispIds)
				: E_FAIL;
#endif
		}

		// IDispatch
		COM_FUNC Invoke(
			DISPID dispId,
			REFIID iid,
			LCID lcid,
			WORD flags,
			DISPPARAMS *params,
			VARIANT *result,
			EXCEPINFO *exceptionInfo,
			UINT *argumentError) override
		{
#if METRO_APP
			return E_FAIL;
#else
			ComPtr<ITypeInfo> typeInfo;
			return SUCCEEDED(GetTypeInfo(0, lcid, &typeInfo))
				? _DispInvoke(_GetDispatch(), typeInfo, dispId, flags, params, result, exceptionInfo, argumentError)
				: E_FAIL;
#endif
		}

		// IProvideClassInfo
		COM_FUNC GetClassInfo(ITypeInfo **typeInfo)
		{
			return _GetClassInfo(typeInfo);
		}

		COM_FUNC EnumConnectionPoints(IEnumConnectionPoints **value)
		{
			assertRetVal(value, E_POINTER);

			ComPtr<ConnectionPointEnum> myValue;
			assertRetVal(ConnectionPointEnum::Create(this, &myValue), E_FAIL);

			*value = myValue.Detach();
			return S_OK;
		}

		COM_FUNC FindConnectionPoint(REFIID iid, IConnectionPoint **value)
		{
			assertRetVal(value, E_POINTER);

			for (size_t i = 0; ; i++)
			{
				IConnectionPointHolder *holder = _GetConnectionPointHolder(i);
				if (holder == nullptr || holder->GetConnectionInterface() == iid)
				{
					if (holder != nullptr && (*value = GetAddRef(holder->GetConnectionPoint(this))) != nullptr)
					{
						return S_OK;
					}

					break;
				}
			}

			return CONNECT_E_NOCONNECTION;
		}
	};
}
