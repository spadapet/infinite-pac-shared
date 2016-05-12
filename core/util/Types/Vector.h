#pragma once

namespace ff
{
	template<typename T, size_t StackSize = 0, typename Allocator = MemAllocator<T>>
	class Vector : private Allocator
	{
		typedef Vector<T, StackSize, Allocator> MyType;
		static const bool IS_POD = std::is_pod<T>::value;

	public:
		Vector();
		Vector(const MyType &rhs);
		Vector(MyType &&rhs);
		~Vector();

		// Operators
		MyType &operator=(const MyType &rhs);
		bool operator==(const MyType &rhs) const;
		bool operator!=(const MyType &rhs) const;

		// Info about the vector
		size_t Size() const;
		size_t ByteSize() const;
		size_t Allocated() const;
		size_t BytesAllocated() const;
		bool IsEmpty() const;

		// Data size operations
		void Clear();
		void Reduce();
		void ClearAndReduce();
		void Resize(size_t size);
		void Reserve(size_t size);

		// Accessors
		const T *Data() const;
		T *Data();
		const T *Data(size_t pos, size_t num = 0) const;
		T *Data(size_t pos, size_t num = 0);
		const T *ConstData() const;
		const T *ConstData(size_t pos, size_t num = 0) const;

		const T &operator[](size_t pos) const;
		T &operator[](size_t pos);

		const T &GetAt(size_t pos, size_t num = 1) const;
		T &GetAt(size_t pos, size_t num = 1);
		const T &GetLast() const;
		T &GetLast();

		// Data setters
		void SetAt(size_t pos, T &&data);
		void SetAt(size_t pos, const T &data);
		void Push(T &&data);
		void Push(const T &data);
		void Push(const T *data, size_t count);
		T Pop();
		void SetStaticData(const T *data, size_t count);

		void Insert(size_t pos, T &&data);
		void Insert(size_t pos, const T &data);
		void Insert(size_t pos, const T *data, size_t count);
		void InsertDefault(size_t pos, size_t count = 1);
		void Delete(size_t pos, size_t count = 1);
		bool DeleteItem(const T &data);

		void SortInsert(const T &data);
		void SortInsert(const T *data, size_t count);
		template<typename LessCompare>
		void SortInsertFunc(const T &data, LessCompare compare);
		template<typename LessCompare>
		void SortInsertFunc(const T *data, size_t count, LessCompare compare);

		// Searching
		size_t Find(const T &data, size_t start = 0) const;

		size_t SortFind(const T &data, size_t start = 0) const;
		bool SortFind(const T &data, size_t *pos, size_t start = 0) const;
		template<typename LessCompare>
		size_t SortFindFunc(const T &data, LessCompare compare, size_t start = 0) const;
		template<typename LessCompare>
		bool SortFindFunc(const T &data, size_t *pos, LessCompare compare, size_t start = 0) const;

	// C++ iterators
	public:
		template<typename IT>
		class Iterator : public std::iterator<std::random_access_iterator_tag, IT>
		{
			typedef Iterator<IT> MyType;

		public:
			Iterator(IT *cur)
				: _cur(cur)
			{
			}

			Iterator(const MyType &rhs)
				: _cur(rhs._cur)
			{
			}

			IT &operator*() const
			{
				return *_cur;
			}

			IT *operator->() const
			{
				return _cur;
			}

			MyType &operator++()
			{
				_cur++;
				return *this;
			}

			MyType operator++(int)
			{
				MyType pre = *this;
				_cur++;
				return pre;
			}

			MyType &operator--()
			{
				_cur--;
				return *this;
			}

			MyType operator--(int)
			{
				MyType pre = *this;
				_cur--;
				return pre;
			}

			bool operator==(const MyType &rhs) const
			{
				return _cur == rhs._cur;
			}

			bool operator!=(const MyType &rhs) const
			{
				return _cur != rhs._cur;
			}

			bool operator<(const MyType &rhs) const
			{
				return _cur < rhs._cur;
			}

			bool operator<=(const MyType &rhs) const
			{
				return _cur <= rhs._cur;
			}

			bool operator>(const MyType &rhs) const
			{
				return _cur > rhs._cur;
			}

			bool operator>=(const MyType &rhs) const
			{
				return _cur >= rhs._cur;
			}

			MyType operator+(ptrdiff_t count) const
			{
				MyType iter = *this;
				iter += count;
				return iter;
			}

			MyType operator-(ptrdiff_t count) const
			{
				MyType iter = *this;
				iter -= count;
				return iter;
			}

			ptrdiff_t operator-(const MyType &rhs) const
			{
				return _cur - rhs._cur;
			}

			MyType &operator+=(ptrdiff_t count)
			{
				_cur += count;
				return *this;
			}

			MyType &operator-=(ptrdiff_t count)
			{
				_cur -= count;
				return *this;
			}

			const IT &operator[](ptrdiff_t pos) const
			{
				return _cur[pos];
			}

			IT &operator[](ptrdiff_t pos)
			{
				return _cur[pos];
			}

		private:
			IT *_cur;
		};

		typedef Iterator<T> iterator;
		typedef Iterator<const T> const_iterator;
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

		iterator begin() { return iterator(_data); }
		iterator end() { return iterator(_data + _size); }
		const_iterator begin() const { return const_iterator(_data); }
		const_iterator end() const { return const_iterator(_data + _size); }
		const_iterator cbegin() const { return const_iterator(_data); }
		const_iterator cend() const { return const_iterator(_data + _size); }

		reverse_iterator rbegin() { return reverse_iterator(end()); }
		reverse_iterator rend() { return reverse_iterator(begin()); }
		const_reverse_iterator rbegin() const { return const_reverse_iterator(cend()); }
		const_reverse_iterator rend() const { return const_reverse_iterator(cbegin()); }
		const_reverse_iterator crbegin() const { return const_reverse_iterator(cend()); }
		const_reverse_iterator crend() const { return const_reverse_iterator(cbegin()); }

	private:
		bool IsStaticData() const;
		void EnsureEditable();
		void ReleaseData();

		T *_data;
		size_t _alloc;
		size_t _size;
		char _stack[StackSize ? (sizeof(T) * StackSize) : 1];
	};
}

template<typename T, size_t StackSize, typename Allocator>
ff::Vector<T, StackSize, Allocator>::Vector()
	: _data((T *)_stack)
	, _alloc(StackSize)
	, _size(0)
{
	// Don't use stack data for types that need special alignment
	assert(StackSize == 0 || __alignof(T) <= sizeof(size_t));
}

template<typename T, size_t StackSize, typename Allocator>
ff::Vector<T, StackSize, Allocator>::Vector(const MyType &rhs)
	: _data((T *)_stack)
	, _alloc(StackSize)
	, _size(0)
{
	// Don't use stack data for types that need special alignment
	assert(StackSize == 0 || __alignof(T) <= sizeof(size_t));

	*this = rhs;
}

template<typename T, size_t StackSize, typename Allocator>
ff::Vector<T, StackSize, Allocator>::Vector(MyType &&rhs)
	: _data((T *)_stack)
	, _alloc(rhs._alloc)
	, _size(rhs._size)
{
	if (rhs._data == (T *)rhs._stack)
	{
		std::memcpy(_data, rhs._data, rhs.ByteSize());
	}
	else
	{
		_data = rhs._data;
	}

	rhs._data = (T *)rhs._stack;
	rhs._alloc = StackSize;
	rhs._size = 0;
}

template<typename T, size_t StackSize, typename Allocator>
ff::Vector<T, StackSize, Allocator>::~Vector()
{
	if (!IS_POD && !IsStaticData())
	{
		for (size_t i = 0; i < _size; i++)
		{
			_data[i].~T();
		}
	}

	ReleaseData();
}

template<typename T, size_t StackSize, typename Allocator>
ff::Vector<T, StackSize, Allocator> &ff::Vector<T, StackSize, Allocator>::operator=(const MyType &rhs)
{
	if (this != &rhs)
	{
		Clear();
		Push(rhs._data, rhs._size);
	}

	return *this;
}

template<typename T, size_t StackSize, typename Allocator>
bool ff::Vector<T, StackSize, Allocator>::operator==(const MyType &rhs) const
{
	if (this != &rhs)
	{
		if (_size != rhs._size)
		{
			return false;
		}

		for (size_t i = 0; i < _size; i++)
		{
			if (_data[i] != rhs._data[i])
			{
				return false;
			}
		}
	}

	return true;
}

template<typename T, size_t StackSize, typename Allocator>
bool ff::Vector<T, StackSize, Allocator>::operator!=(const MyType &rhs) const
{
	return !(*this == rhs);
}

template<typename T, size_t StackSize, typename Allocator>
const T *ff::Vector<T, StackSize, Allocator>::Data() const
{
	assert(_size);
	return _data;
}

template<typename T, size_t StackSize, typename Allocator>
const T *ff::Vector<T, StackSize, Allocator>::ConstData() const
{
	assert(_size);
	return _data;
}

template<typename T, size_t StackSize, typename Allocator>
T *ff::Vector<T, StackSize, Allocator>::Data()
{
	assert(_size);
	EnsureEditable();
	return _data;
}

template<typename T, size_t StackSize, typename Allocator>
const T *ff::Vector<T, StackSize, Allocator>::Data(size_t pos, size_t num) const
{
	assert(pos >= 0 && pos + num <= _size);
	return _data + pos;
}

template<typename T, size_t StackSize, typename Allocator>
const T *ff::Vector<T, StackSize, Allocator>::ConstData(size_t pos, size_t num) const
{
	assert(pos >= 0 && pos + num <= _size);
	return _data + pos;
}

template<typename T, size_t StackSize, typename Allocator>
T *ff::Vector<T, StackSize, Allocator>::Data(size_t pos, size_t num)
{
	assert(pos >= 0 && pos + num <= _size);
	EnsureEditable();
	return _data + pos;
}

template<typename T, size_t StackSize, typename Allocator>
size_t ff::Vector<T, StackSize, Allocator>::Size() const
{
	return _size;
}

template<typename T, size_t StackSize, typename Allocator>
size_t ff::Vector<T, StackSize, Allocator>::ByteSize() const
{
	return _size * sizeof(T);
}

template<typename T, size_t StackSize, typename Allocator>
bool ff::Vector<T, StackSize, Allocator>::IsEmpty() const
{
	return !_size;
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::Clear()
{
	Resize(0);
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::ClearAndReduce()
{
	Clear();
	Reduce();
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::Resize(size_t size)
{
	if (size < _size)
	{
		Delete(size, _size - size);
	}
	else if (size > _size)
	{
		InsertDefault(_size, size - _size);
	}
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::Reserve(size_t size)
{
	if (size > _alloc)
	{
		size_t oldSize = _size;
		size_t alloc = std::max<size_t>(_alloc, 8);

		if (alloc < size)
		{
			const size_t MAX_GROW_EXPONENTIAL = 1024;
			alloc += (alloc > MAX_GROW_EXPONENTIAL) ? MAX_GROW_EXPONENTIAL : alloc;
			alloc = std::max<size_t>(alloc, size);
		}

		T *data = Allocator::Malloc(alloc);

		std::memcpy(data, _data, sizeof(T) * _size);
		ReleaseData();

		_data = data;
		_alloc = alloc;
		_size = oldSize;
	}
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::Reduce()
{
	if (_size < _alloc && _data != (T *)_stack)
	{
		if (_size == 0)
		{
			ReleaseData();
		}
		else
		{
			T *data = (_size <= StackSize)
				? (T *)_stack
				: Allocator::Malloc(_size);

			std::memcpy(data, _data, sizeof(T) * _size);

			size_t size = _size;
			ReleaseData();

			_data = data;
			_size = size;
			_alloc = (_data == (T *)_stack) ? StackSize : _size;
		}
	}
}

template<typename T, size_t StackSize, typename Allocator>
size_t ff::Vector<T, StackSize, Allocator>::Allocated() const
{
	return _alloc;
}

template<typename T, size_t StackSize, typename Allocator>
size_t ff::Vector<T, StackSize, Allocator>::BytesAllocated() const
{
	return _alloc * sizeof(T);
}

template<typename T, size_t StackSize, typename Allocator>
const T &ff::Vector<T, StackSize, Allocator>::operator[](size_t pos) const
{
	assert(pos >= 0 && pos < _size);
	return _data[pos];
}

template<typename T, size_t StackSize, typename Allocator>
T &ff::Vector<T, StackSize, Allocator>::operator[](size_t pos)
{
	assert(pos >= 0 && pos < _size);
	EnsureEditable();
	return _data[pos];
}

template<typename T, size_t StackSize, typename Allocator>
const T &ff::Vector<T, StackSize, Allocator>::GetAt(size_t pos, size_t num) const
{
	assert(pos >= 0 && pos < _size && pos + num <= _size);
	return _data[pos];
}

template<typename T, size_t StackSize, typename Allocator>
T &ff::Vector<T, StackSize, Allocator>::GetAt(size_t pos, size_t num)
{
	assert(pos >= 0 && pos < _size && pos + num <= _size);
	EnsureEditable();
	return _data[pos];
}

template<typename T, size_t StackSize, typename Allocator>
const T &ff::Vector<T, StackSize, Allocator>::GetLast() const
{
	assert(_size != 0);
	return _data[_size - 1];
}

template<typename T, size_t StackSize, typename Allocator>
T &ff::Vector<T, StackSize, Allocator>::GetLast()
{
	assert(_size);
	EnsureEditable();
	return _data[_size - 1];
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::SetAt(size_t pos, T &&data)
{
	assert(pos >= 0 && pos < _size);
	EnsureEditable();

	if (IS_POD)
	{
		std::memcpy(_data + pos, &data, sizeof(T));
	}
	else
	{
		_data[pos].~T();
		::new(_data + pos) T(std::move(data));
	}
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::SetAt(size_t pos, const T &data)
{
	assert(pos >= 0 && pos < _size);
	EnsureEditable();

	if (IS_POD)
	{
		std::memcpy(_data + pos, &data, sizeof(T));
	}
	else
	{
		_data[pos] = data;
	}
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::Push(T &&data)
{
	Insert(_size, std::move(data));
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::Push(const T &data)
{
	Insert(_size, &data, 1);
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::Push(const T *data, size_t count)
{
	Insert(_size, data, count);
}

template<typename T, size_t StackSize, typename Allocator>
T ff::Vector<T, StackSize, Allocator>::Pop()
{
	assert(_size);
	EnsureEditable();
	T ret(std::move(_data[_size - 1]));
	Resize(_size - 1);
	return ret;
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::SetStaticData(const T *data, size_t count)
{
	assert(data);
	ClearAndReduce();
	_data = const_cast<T *>(data);
	_alloc = 0;
	_size = count;
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::Insert(size_t pos, T &&data)
{
	assert(pos >= 0 && pos <= _size);

	Reserve(_size + 1);

	if (pos < _size)
	{
		std::memmove(_data + pos + 1, _data + pos, sizeof(T) * (_size - pos));
	}

	if (IS_POD)
	{
		std::memcpy(_data + pos, &data, sizeof(T));
	}
	else
	{
		::new(_data + pos) T(std::move(data));
	}

	_size++;
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::Insert(size_t pos, const T &data)
{
	Insert(pos, &data, 1);
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::Insert(size_t pos, const T *data, size_t count)
{
	assert(data || !count);
	assert(pos >= 0 && count >= 0 && pos <= _size);

	if (count == 0)
	{
		// nothing to do
	}
	else if (data >= _data + _alloc || data + count <= _data)
	{
		// copying data from OUTSIDE the existing array
		Reserve(_size + count);

		if (pos < _size)
		{
			std::memmove(_data + pos + count, _data + pos, sizeof(T) * (_size - pos));
		}

		if (IS_POD)
		{
			std::memcpy(_data + pos, data, sizeof(T) * count);
		}
		else
		{
			for (size_t i = 0; i < count; i++)
			{
				::new(_data + pos + i) T(data[i]);
			}
		}

		_size += count;
	}
	else
	{
		// copying data from INSIDE the existing array, take the easy way
		// out and make another copy of the data first
		T *dataCopy = Allocator::Malloc(count);

		if (IS_POD)
		{
			std::memcpy(dataCopy, data, sizeof(T) * count);
		}
		else
		{
			for (size_t i = 0; i < count; i++)
			{
				::new(dataCopy + i) T(data[i]);
			}
		}

		Insert(pos, dataCopy, count);

		Allocator::Free(dataCopy);
	}
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::InsertDefault(size_t pos, size_t count)
{
	assert(pos >= 0 && count >= 0 && pos <= _size);

	if (count != 0)
	{
		Reserve(_size + count);

		if (pos < _size)
		{
			std::memmove(_data + pos + count, _data + pos, sizeof(T) * (_size - pos));
		}

		if (!IS_POD)
		{
			for (size_t i = 0; i < count; i++)
			{
				::new(_data + pos + i) T;
			}
		}

		_size += count;
	}
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::Delete(size_t pos, size_t count)
{
	assert(pos >= 0 && count >= 0 && pos + count <= _size);

	if (count != 0)
	{
		EnsureEditable();

		if (!IS_POD)
		{
			for (size_t i = 0; i < count; i++)
			{
				_data[pos + i].~T();
			}
		}

		size_t move = _size - pos - count;
		if (move != 0)
		{
			std::memmove(_data + pos, _data + pos + count, sizeof(T) * move);
		}

		_size -= count;
	}
}

template<typename T, size_t StackSize, typename Allocator>
bool ff::Vector<T, StackSize, Allocator>::DeleteItem(const T &data)
{
	size_t i = Find(data);
	if (i != INVALID_SIZE)
	{
		Delete(i);
		return true;
	}

	return false;
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::SortInsert(const T &data)
{
	SortInsertFunc(&data, 1, std::less<T>());
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::SortInsert(const T *data, size_t count)
{
	SortInsertFunc(data, count, std::less<T>());
}

template<typename T, size_t StackSize, typename Allocator>
template<typename LessCompare>
void ff::Vector<T, StackSize, Allocator>::SortInsertFunc(const T &data, LessCompare compare)
{
	SortInsertFunc(&data, 1, compare);
}

template<typename T, size_t StackSize, typename Allocator>
template<typename LessCompare>
void ff::Vector<T, StackSize, Allocator>::SortInsertFunc(const T *data, size_t count, LessCompare compare)
{
	Reserve(_size + count);

	for (size_t i = 0; i < count; i++)
	{
		size_t pos;
		SortFindFunc(data[i], &pos, compare);
		Insert(pos, data + i, 1);
	}
}

template<typename T, size_t StackSize, typename Allocator>
size_t ff::Vector<T, StackSize, Allocator>::Find(const T &data, size_t start) const
{
	if (&data >= _data + start && &data < _data + _size)
	{
		return &data - _data;
	}

	for (size_t i = start; i < _size; i++)
	{
		if (_data[i] == data)
		{
			return i;
		}
	}

	return INVALID_SIZE;
}

template<typename T, size_t StackSize, typename Allocator>
size_t ff::Vector<T, StackSize, Allocator>::SortFind(const T &data, size_t start) const
{
	return SortFindFunc(data, std:less<T>(), start);
}

template<typename T, size_t StackSize, typename Allocator>
bool ff::Vector<T, StackSize, Allocator>::SortFind(const T &data, size_t *pos, size_t start) const
{
	return SortFindFunc(data, pos, std::less<T>(), start);
}

template<typename T, size_t StackSize, typename Allocator>
template<typename LessCompare>
size_t ff::Vector<T, StackSize, Allocator>::SortFindFunc(const T &data, LessCompare compare, size_t start) const
{
	size_t pos;
	return SortFindFunc(data, &pos, compare, start) ? pos : INVALID_SIZE;
}

template<typename T, size_t StackSize, typename Allocator>
template<typename LessCompare>
bool ff::Vector<T, StackSize, Allocator>::SortFindFunc(const T &data, size_t *pos, LessCompare compare, size_t start) const
{
	assert(start <= _size);

	auto found = std::lower_bound<const_iterator, const T &, LessCompare>(_data + start, end(), data, compare);
	if (pos != nullptr)
	{
		*pos = found - _data;
	}

	return found != end() && !compare(data, *found);
}

template<typename T, size_t StackSize, typename Allocator>
bool ff::Vector<T, StackSize, Allocator>::IsStaticData() const
{
	return !_alloc;
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::EnsureEditable()
{
	if (!_alloc)
	{
		Reserve(1);
	}
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::ReleaseData()
{
	if (_data != (T *)_stack)
	{
		if (_alloc)
		{
			Allocator::Free(_data);
		}

		_data = (T *)_stack;
	}

	_alloc = StackSize;
	_size = 0;
}
