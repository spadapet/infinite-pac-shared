#pragma once

namespace ff
{
	class IConnectionPointHolder;
	class Module;

	// A base class for COM classes
	class UTIL_API ComBaseEx
	{
	public:
		ComBaseEx();
		virtual ~ComBaseEx();

		virtual HRESULT _QueryInterface(REFIID iid, void **ppv) = 0;
		virtual IUnknown *_GetUnknown() = 0;
		virtual IDispatch *_GetDispatch() = 0;
		virtual long _GetRefCount() const = 0;
		virtual REFGUID _GetClassID() const = 0;
		virtual const Module &_GetSourceModule() const = 0;

		// These can be overridden either in ComObject<T> or in your type
		virtual const char *_GetClassName() const;
		virtual size_t _GetTypeLibIndex() const;
		virtual HRESULT _GetClassInfo(ITypeInfo **typeInfo) const;
		virtual IConnectionPointHolder *_GetConnectionPointHolder(size_t index);

		// These can be overridden in your type
		virtual HRESULT _Construct(IUnknown *unkOuter);
		virtual void _Destruct();
		virtual void _DeleteThis();

		// Custom IDispatch
		virtual HRESULT _DispGetIDsOfNames(
			ITypeInfo *typeInfo,
			OLECHAR **names,
			UINT nameCount,
			DISPID *dispIds);

		virtual HRESULT _DispInvoke(
			IDispatch *disp,
			ITypeInfo *typeInfo,
			DISPID dispId,
			WORD flags,
			DISPPARAMS *params,
			VARIANT *result,
			EXCEPINFO *exceptionInfo,
			UINT *argumentError);

		static void DumpComObjects();

	protected:
		// Leak tracking, don't call these, they are already called in the correct places.
		// However, if you are smart, you can call _NoLeak() in your constructor to 
		// disable leak tracking for your object. But that would be dumb.
		void _TrackLeak();
		void _NoLeak();

		static void _AddRefModule(const Module &module);
		static void _ReleaseModule(const Module &module);
	};

	// The ref-count is thread safe
	class UTIL_API ComBase : public ComBaseEx
	{
	public:
		ComBase();
		virtual ~ComBase();

		virtual long _GetRefCount() const override;

		ULONG _AddRef();
		ULONG _Release();

	private:
		long _refs;
	};

	// The ref-count is not thread safe
	class UTIL_API ComBaseUnsafe : public ComBaseEx
	{
	public:
		ComBaseUnsafe();
		virtual ~ComBaseUnsafe();

		virtual long _GetRefCount() const override;

		ULONG _AddRef();
		ULONG _Release();

	private:
		long _refs;
	};

	// There is no ref-count and this object is not tracked for leaks
	class UTIL_API ComBaseNoRef : public ComBaseEx
	{
	public:
		ComBaseNoRef();
		virtual ~ComBaseNoRef();

		virtual long _GetRefCount() const override;

		ULONG _AddRef();
		ULONG _Release();
	};
}
