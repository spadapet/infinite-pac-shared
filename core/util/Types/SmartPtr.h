#pragma once

namespace ff
{
	// T = class/interface type
	// I = optionally the main interface to use for calling AddRef/Release
	template<typename T, typename I = T>
	class SmartPtr
	{
	public:
		SmartPtr();
		SmartPtr(const SmartPtr<T, I> &rhs);
		SmartPtr(SmartPtr<T, I> &&rhs);
		SmartPtr(T *obj);
		~SmartPtr();

		void Release();
		bool IsValid() const;

		void Attach(T *rhs);
		T *Detach();
		I *DetachInterface();
		void Swap(SmartPtr<T, I> &rhs);

		SmartPtr<T, I> &operator=(const SmartPtr<T, I> &rhs);
		SmartPtr<T, I> &operator=(SmartPtr<T, I> &&rhs);
		SmartPtr<T, I> &operator=(T *obj);
		bool operator==(const SmartPtr<T, I> &rhs) const;
		bool operator!=(const SmartPtr<T, I> &rhs) const;
		bool operator==(T *obj) const;
		bool operator!=(T *obj) const;
		bool operator==(std::nullptr_t obj) const;
		bool operator!=(std::nullptr_t obj) const;

		operator T *() const;
		T &operator*() const;
		T *operator->() const;
		T **operator&();
		bool operator!() const;

		SmartPtr<T, I> *This(); // in case the automatic casts get in the way
		T *Object() const;
		I *Interface() const;
		T **Address();
		T *const *Address() const;

	private:
		static I *GetI(T *obj);
		T *_obj;
	};
}

template<typename T, typename I>
ff::SmartPtr<T, I>::SmartPtr()
	: _obj(nullptr)
{
}

template<typename T, typename I>
ff::SmartPtr<T, I>::SmartPtr(const SmartPtr<T, I> &rhs)
	: _obj(nullptr)
{
	*this = rhs;
}

template<typename T, typename I>
ff::SmartPtr<T, I>::SmartPtr(SmartPtr<T, I> &&rhs)
	: _obj(rhs._obj)
{
	rhs._obj = nullptr;
}

template<typename T, typename I>
ff::SmartPtr<T, I>::SmartPtr(T *obj)
	: _obj(nullptr)
{
	*this = obj;
}

template<typename T, typename I>
ff::SmartPtr<T, I>::~SmartPtr()
{
	Release();
}

template<typename T, typename I>
void ff::SmartPtr<T, I>::Release()
{
	Attach(nullptr);
}

template<typename T, typename I>
bool ff::SmartPtr<T, I>::IsValid() const
{
	return _obj != nullptr;
}

template<typename T, typename I>
void ff::SmartPtr<T, I>::Attach(T *rhs)
{
	if (_obj)
	{
		GetI(_obj)->Release();
	}

	_obj = rhs;
}

template<typename T, typename I>
T *ff::SmartPtr<T, I>::Detach()
{
	T *obj = _obj;
	_obj = nullptr;
	return obj;
}

template<typename T, typename I>
I *ff::SmartPtr<T, I>::DetachInterface()
{
	return GetI(Detach());
}

template<typename T, typename I>
void ff::SmartPtr<T, I>::Swap(SmartPtr<T, I> &rhs)
{
	std::swap(_obj, rhs._obj);
}

template<typename T, typename I>
ff::SmartPtr<T, I> &ff::SmartPtr<T, I>::operator=(const SmartPtr<T, I> &rhs)
{
	return *this = rhs._obj;
}

template<typename T, typename I>
ff::SmartPtr<T, I> &ff::SmartPtr<T, I>::operator=(SmartPtr<T, I> &&rhs)
{
	Attach(rhs.Detach());
	return *this;
}

template<typename T, typename I>
ff::SmartPtr<T, I> &ff::SmartPtr<T, I>::operator=(T *obj)
{
	if (_obj != obj)
	{
		if (obj)
		{
			GetI(obj)->AddRef();
		}

		Attach(obj);
	}

	return *this;
}

template<typename T, typename I>
bool ff::SmartPtr<T, I>::operator==(const SmartPtr<T, I> &rhs) const
{
	return _obj == rhs._obj;
}

template<typename T, typename I>
bool ff::SmartPtr<T, I>::operator!=(const SmartPtr<T, I> &rhs) const
{
	return _obj != rhs._obj;
}

template<typename T, typename I>
bool ff::SmartPtr<T, I>::operator==(T *obj) const
{
	return _obj == obj;
}

template<typename T, typename I>
bool ff::SmartPtr<T, I>::operator!=(T *obj) const
{
	return _obj != obj;
}

template<typename T, typename I>
bool ff::SmartPtr<T, I>::operator==(std::nullptr_t obj) const
{
	return _obj == nullptr;
}

template<typename T, typename I>
bool ff::SmartPtr<T, I>::operator!=(std::nullptr_t obj) const
{
	return _obj != nullptr;
}

template<typename T, typename I>
ff::SmartPtr<T, I>::operator T *() const
{
	return _obj;
}

template<typename T, typename I>
T &ff::SmartPtr<T, I>::operator*() const
{
	return *_obj;
}

template<typename T, typename I>
T *ff::SmartPtr<T, I>::operator->() const
{
	return _obj;
}

template<typename T, typename I>
T **ff::SmartPtr<T, I>::operator&()
{
	assert(!_obj);
	return &_obj;
}

template<typename T, typename I>
bool ff::SmartPtr<T, I>::operator!() const
{
	return !_obj;
}

template<typename T, typename I>
ff::SmartPtr<T, I> *ff::SmartPtr<T, I>::This()
{
	return this;
}

template<typename T, typename I>
T *ff::SmartPtr<T, I>::Object() const
{
	return _obj;
}

template<typename T, typename I>
I *ff::SmartPtr<T, I>::Interface() const
{
	return GetI(_obj);
}

template<typename T, typename I>
T **ff::SmartPtr<T, I>::Address()
{
	return &_obj;
}

template<typename T, typename I>
T *const *ff::SmartPtr<T, I>::Address() const
{
	return &_obj;
}

template<typename T, typename I>
I *ff::SmartPtr<T, I>::GetI(T *obj)
{
	return static_cast<I *>(obj);
}

namespace std
{
	template<typename T, typename I>
	void swap(ff::SmartPtr<T, I> &lhs, ff::SmartPtr<T, I> &rhs)
	{
		lhs.Swap(rhs);
	}
}
