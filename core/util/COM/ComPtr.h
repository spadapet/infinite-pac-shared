#pragma once

namespace ff
{
	// T = class/interface type
	// I = optionally the main interface to use for calling AddRef/Release
	template<typename T, typename I = T>
	class ComPtr : public SmartPtr<T, I>
	{
	public:
		ComPtr();
		ComPtr(const ComPtr<T, I> &rhs);
		ComPtr(const SmartPtr<T, I> &rhs);
		ComPtr(ComPtr<T, I> &&rhs);
		ComPtr(SmartPtr<T, I> &&rhs);
		ComPtr(T *obj);

		bool QueryFrom(IUnknown *obj);
		bool QueryFrom(const IUnknown *obj);
#if METRO_APP
		bool QueryFrom(::Platform::Object ^obj);
#endif
		ComPtr<T, I> &operator=(const ComPtr<T, I> &rhs);

		using SmartPtr<T, I>::operator=;
		using SmartPtr<T, I>::operator==;
		using SmartPtr<T, I>::operator!=;

		ComPtr<T, I> *This(); // in case the automatic casts get in the way
	};

	template<typename T, typename I>
	ComPtr<T, I>::ComPtr()
	{
	}

	template<typename T, typename I>
	ComPtr<T, I>::ComPtr(const ComPtr<T, I> &rhs)
		: SmartPtr(rhs)
	{
	}

	template<typename T, typename I>
	ComPtr<T, I>::ComPtr(const SmartPtr<T, I> &rhs)
		: SmartPtr(rhs)
	{
	}

	template<typename T, typename I>
	ComPtr<T, I>::ComPtr(ComPtr<T, I> &&rhs)
		: SmartPtr(std::move(rhs))
	{
	}

	template<typename T, typename I>
	ComPtr<T, I>::ComPtr(SmartPtr<T, I> &&rhs)
		: SmartPtr(std::move(rhs))
	{
	}

	template<typename T, typename I>
	ComPtr<T, I>::ComPtr(T *obj)
		: SmartPtr(obj)
	{
	}

	template<typename T, typename I>
	bool ComPtr<T, I>::QueryFrom(IUnknown *obj)
	{
		T *tobj = nullptr;
		if (obj != nullptr && SUCCEEDED(obj->QueryInterface(__uuidof(T), (void **)&tobj)))
		{
			Attach(tobj);
			return true;
		}
		else
		{
			Release();
			return false;
		}
	}

	template<typename T, typename I>
	bool ComPtr<T, I>::QueryFrom(const IUnknown *obj)
	{
		return QueryFrom(const_cast<IUnknown *>(obj));
	}

#if METRO_APP
	template<typename T, typename I>
	bool ComPtr<T, I>::QueryFrom(::Platform::Object ^obj)
	{
		return QueryFrom((IUnknown *)obj);
	}
#endif

	template<typename T, typename I>
	ComPtr<T, I> &ComPtr<T, I>::operator=(const ComPtr<T, I> &rhs)
	{
		__super::operator=(rhs);
		return *this;
	}

	template<typename T, typename I>
	ComPtr<T, I> *ComPtr<T, I>::This()
	{
		return this;
	}
}

namespace std
{
	template<typename T, typename I>
	void swap(ff::ComPtr<T, I> &lhs, ff::ComPtr<T, I> &rhs)
	{
		lhs.Swap(rhs);
	}
}
