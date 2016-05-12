#pragma once

namespace ff
{
	// Doubly-linked list
	//
	// Uses a pool allocator for memory locality of nodes and fast allocations.
	template<typename T>
	class List
	{
	public:
		List();
		List(const List<T> &rhs);
		List(List<T> &&rhs);
		~List();

		List<T> &operator=(const List<T> &rhs);

		T &Insert(const T &obj);
		T &InsertFirst(const T &obj);
		T &InsertAfter(const T &obj, const T *afterObj);
		T &InsertBefore(const T &obj, const T *beforeObj);

		T &Insert(T &&obj);
		T &InsertFirst(T &&obj);
		T &InsertAfter(T &&obj, const T *afterObj);
		T &InsertBefore(T &&obj, const T *beforeObj);

		T &Insert();
		T &InsertFirst();
		T &InsertAfter(const T *afterObj);
		T &InsertBefore(const T *beforeObj);

		void MoveToFront(const T &obj);
		void MoveToBack(const T &obj);

		size_t Size() const;
		T PopFirst();
		T PopLast();
		T *Delete(const T &obj, bool after = true); // returns the object after or before the deleted object
		void DeleteFirst();
		void DeleteLast();
		void Clear();
		bool IsEmpty() const;

		T *GetFirst() const;
		T *GetLast() const;
		T *GetNext(const T &obj) const;
		T *GetPrev(const T &obj) const;
		T *Find(const T &obj) const;

		size_t MemUsage() const;
		Vector<T> ToVector() const;
		Vector<T *> ToPointerVector();
		Vector<const T *> ToConstPointerVector() const;
		static List<T> FromVector(const Vector<T> &vec);

	private:
		struct ListItem
		{
			ListItem();
			ListItem(ListItem &&rhs);
			ListItem(const T &rhs);
			ListItem(T &&rhs);

			T _val;
			ListItem *_next;
			ListItem *_prev;
		};

		ListItem *ItemFromObject(const T *obj) const;
		T *ObjectFromItem(ListItem *item) const;
		T &InsertAfter(ListItem *newItem, ListItem *afterItem);
		T &InsertBefore(ListItem *newItem, ListItem *beforeItem);

		ListItem *New();
		ListItem *New(const T &obj);
		ListItem *New(T &&obj);

		ListItem *_head;
		ListItem *_tail;
		PoolAllocator<ListItem> _itemPool;

	// Imperfect C++ iterators
	public:
		template<typename IT, bool Reverse>
		class Iterator : public std::iterator<std::bidirectional_iterator_tag, IT>
		{
			typedef Iterator<IT, Reverse> MyType;

		public:
			Iterator(ListItem *item)
				: _item(item)
			{
			}

			Iterator(const MyType &rhs)
				: _item(rhs._item)
			{
			}

			IT &operator*() const
			{
				return _item->_val;
			}

			IT *operator->() const
			{
				return &_item->_val;
			}

			MyType &operator++()
			{
				_item = Reverse ? _item->_prev : _item->_next;
				return *this;
			}

			MyType operator++(int)
			{
				MyType pre = *this;
				_item = Reverse ? _item->_prev : _item->_next;
				return pre;
			}

			MyType &operator--()
			{
				_item = Reverse ? _item->_next :_item->_prev;
				return *this;
			}

			MyType operator--(int)
			{
				MyType pre = *this;
				_item = Reverse ? _item->_next : _item->_prev;
				return pre;
			}

			bool operator==(const MyType &rhs) const
			{
				return _item == rhs._item;
			}

			bool operator!=(const MyType &rhs) const
			{
				return _item != rhs._item;
			}

		private:
			ListItem *_item;
		};

		typedef Iterator<T, false> iterator;
		typedef Iterator<const T, false> const_iterator;
		typedef Iterator<T, true> reverse_iterator;
		typedef Iterator<const T, true> const_reverse_iterator;

		iterator begin() { return iterator(_head); }
		iterator end() { return iterator(nullptr); }
		const_iterator begin() const { return const_iterator(_head); }
		const_iterator end() const { return const_iterator(nullptr); }
		const_iterator cbegin() const { return const_iterator(_head); }
		const_iterator cend() const { return const_iterator(nullptr); }

		reverse_iterator rbegin() { return reverse_iterator(_tail); }
		reverse_iterator rend() { return reverse_iterator(nullptr); }
		const_reverse_iterator rbegin() const { return const_reverse_iterator(_tail); }
		const_reverse_iterator rend() const { return const_reverse_iterator(nullptr); }
		const_reverse_iterator crbegin() const { return const_reverse_iterator(_tail); }
		const_reverse_iterator crend() const { return const_reverse_iterator(nullptr); }
	};

	template<typename T>
	List<T>::List()
		: _head(nullptr)
		, _tail(nullptr)
		, _itemPool(false)
	{
	}

	template<typename T>
	List<T>::List(const List<T> &rhs)
		: _head(nullptr)
		, _tail(nullptr)
		, _itemPool(false)
	{
		*this = rhs;
	}

	template<typename T>
	List<T>::List(List<T> &&rhs)
		: _head(rhs._head)
		, _tail(rhs._tail)
		, _itemPool(std::move(rhs._itemPool))
	{
		assert(rhs.Size() == 0);
		rhs._head = nullptr;
		rhs._tail = nullptr;
	}

	template<typename T>
	List<T>::~List()
	{
		Clear();
	}

	template<typename T>
	List<T> &List<T>::operator=(const List<T> &rhs)
	{
		if (this != &rhs)
		{
			Clear();

			for (const T *obj = rhs.GetFirst(); obj; obj = rhs.GetNext(*obj))
			{
				Insert(*obj);
			}
		}

		return *this;
	}

	template<typename T>
	T &List<T>::Insert(const T &obj)
	{
		return InsertAfter(New(obj), nullptr);
	}

	template<typename T>
	T &List<T>::InsertFirst(const T &obj)
	{
		return InsertBefore(New(obj), nullptr);
	}

	template<typename T>
	T &List<T>::InsertAfter(const T &obj, const T *afterObj)
	{
		return InsertAfter(New(obj), ItemFromObject(afterObj));
	}

	template<typename T>
	T &List<T>::InsertBefore(const T &obj, const T *beforeObj)
	{
		return InsertBefore(New(obj), ItemFromObject(beforeObj));
	}

	template<typename T>
	T &List<T>::Insert(T &&obj)
	{
		return InsertAfter(New(std::move(obj)), nullptr);
	}

	template<typename T>
	T &List<T>::InsertFirst(T &&obj)
	{
		return InsertBefore(New(std::move(obj)), nullptr);
	}

	template<typename T>
	T &List<T>::InsertAfter(T &&obj, const T *afterObj)
	{
		return InsertAfter(New(std::move(obj)), ItemFromObject(afterObj));
	}

	template<typename T>
	T &List<T>::InsertBefore(T &&obj, const T *beforeObj)
	{
		return InsertBefore(New(std::move(obj)), ItemFromObject(beforeObj));
	}

	template<typename T>
	T &List<T>::Insert()
	{
		return InsertAfter(New(), nullptr);
	}

	template<typename T>
	T &List<T>::InsertFirst()
	{
		return InsertBefore(New(), nullptr);
	}

	template<typename T>
	T &List<T>::InsertAfter(const T *afterObj)
	{
		return InsertAfter(New(), ItemFromObject(afterObj));
	}

	template<typename T>
	T &List<T>::InsertBefore(const T *beforeObj)
	{
		return InsertBefore(New(), ItemFromObject(beforeObj));
	}

	template<typename T>
	void List<T>::MoveToFront(const T &obj)
	{
		ListItem *item = ItemFromObject(&obj);

		if (item != _head)
		{
			if (item == _tail)
			{
				_tail = item->_prev;
			}

			if (item->_prev != nullptr)
			{
				item->_prev->_next = item->_next;
			}

			if (item->_next != nullptr)
			{
				item->_next->_prev = item->_prev;
			}

			item->_next = _head;
			item->_prev = nullptr;

			_head->_prev = item;
			_head = item;
		}
	}

	template<typename T>
	void List<T>::MoveToBack(const T &obj)
	{
		ListItem *item = ItemFromObject(&obj);

		if (item != _tail)
		{
			if (item == _head)
			{
				_head = item->_next;
			}

			if (item->_prev != nullptr)
			{
				item->_prev->_next = item->_next;
			}

			if (item->_next != nullptr)
			{
				item->_next->_prev = item->_prev;
			}

			item->_next = nullptr;
			item->_prev = _tail;

			_tail->_next = item;
			_tail = item;
		}
	}

	template<typename T>
	size_t List<T>::Size() const
	{
		return _itemPool.GetCurAlloc();
	}

	template<typename T>
	T List<T>::PopFirst()
	{
		assert(Size());
		T obj = *GetFirst();
		DeleteFirst();
		return obj;
	}

	template<typename T>
	T List<T>::PopLast()
	{
		assert(Size());
		T obj = *GetLast();
		DeleteLast();
		return obj;
	}

	template<typename T>
	T *List<T>::Delete(const T &obj, bool after)
	{
		ListItem *item = ItemFromObject(&obj);
		ListItem *nextItem = after ? item->_next : item->_prev;

		if (item == _head)
		{
			_head = item->_next;
		}

		if (item == _tail)
		{
			_tail = item->_prev;
		}

		if (item->_prev != nullptr)
		{
			item->_prev->_next = item->_next;
		}

		if (item->_next != nullptr)
		{
			item->_next->_prev = item->_prev;
		}

		_itemPool.Delete(item);

		return nextItem ? ObjectFromItem(nextItem) : nullptr;
	}

	template<typename T>
	void List<T>::DeleteFirst()
	{
		if (GetFirst())
		{
			Delete(*GetFirst());
		}
	}

	template<typename T>
	void List<T>::DeleteLast()
	{
		if (GetLast())
		{
			Delete(*GetLast());
		}
	}

	template<typename T>
	void List<T>::Clear()
	{
		while (_head != nullptr)
		{
			Delete(_head->_val);
		}
	}

	template<typename T>
	bool List<T>::IsEmpty() const
	{
		return Size() == 0;
	}

	template<typename T>
	T *List<T>::GetFirst() const
	{
		return _head ? ObjectFromItem(_head) : nullptr;
	}

	template<typename T>
	T *List<T>::GetLast() const
	{
		return _tail ? ObjectFromItem(_tail) : nullptr;
	}

	template<typename T>
	T *List<T>::GetNext(const T &obj) const
	{
		ListItem *item = ItemFromObject(&obj);
		return item && item->_next ? ObjectFromItem(item->_next) : nullptr;
	}

	template<typename T>
	T *List<T>::GetPrev(const T &obj) const
	{
		ListItem *item = ItemFromObject(&obj);
		return item && item->_prev ? ObjectFromItem(item->_prev) : nullptr;
	}

	template<typename T>
	T *List<T>::Find(const T &obj) const
	{
		for (ListItem *item = _head; item; item = item->_next)
		{
			if (item->_val == obj)
			{
				return &item->_val;
			}
		}

		return nullptr;
	}

	template<typename T>
	size_t List<T>::MemUsage() const
	{
		return _itemPool.MemUsage();
	}

	template<typename T>
	Vector<T> List<T>::ToVector() const
	{
		Vector<T> newVector;
		newVector.Reserve(Size());

		for (const T &iter: *this)
		{
			newVector.Push(iter);
		}

		return newVector;
	}

	template<typename T>
	Vector<T *> List<T>::ToPointerVector()
	{
		Vector<T *> newVector;
		newVector.Reserve(Size());

		for (T &iter: *this)
		{
			newVector.Push(&iter);
		}

		return newVector;
	}

	template<typename T>
	Vector<const T *> List<T>::ToConstPointerVector() const
	{
		Vector<const T *> newVector;
		newVector.Reserve(Size());

		for (const T &iter: *this)
		{
			newVector.Push(&iter);
		}

		return newVector;
	}

	template<typename T>
	List<T> List<T>::FromVector(const Vector<T> &vec)
	{
		List<T> newList;
		for (const T &iter: vec)
		{
			newList.Insert(iter);
		}

		return newList;
	}

	template<typename T>
	typename List<T>::ListItem *List<T>::ItemFromObject(const T *obj) const
	{
		return const_cast<ListItem *>((const ListItem *)obj);
	}

	template<typename T>
	T *List<T>::ObjectFromItem(ListItem *item) const
	{
		return (T *)item;
	}

	template<typename T>
	T &List<T>::InsertAfter(ListItem *newItem, ListItem *afterItem)
	{
		if (afterItem == nullptr)
		{
			afterItem = _tail;
		}

		if (afterItem == nullptr)
		{
			_head = newItem;
			_tail = newItem;
			newItem->_next = nullptr;
			newItem->_prev = nullptr;
		}
		else if (afterItem == _tail)
		{
			newItem->_next = nullptr;
			newItem->_prev = _tail;
			_tail->_next = newItem;
			_tail = newItem;
		}
		else
		{
			newItem->_prev = afterItem;
			newItem->_next = afterItem->_next;
			afterItem->_next->_prev = newItem;
			afterItem->_next = newItem;
		}

		return *ObjectFromItem(newItem);
	}

	template<typename T>
	T &List<T>::InsertBefore(ListItem *newItem, ListItem *beforeItem)
	{
		if (beforeItem == nullptr)
		{
			beforeItem = _head;
		}

		if (beforeItem == nullptr)
		{
			_head = newItem;
			_tail = newItem;
			newItem->_next = nullptr;
			newItem->_prev = nullptr;
		}
		else if (beforeItem == _head)
		{
			newItem->_next = _head;
			newItem->_prev = nullptr;
			_head->_prev = newItem;
			_head = newItem;
		}
		else
		{
			newItem->_next = beforeItem;
			newItem->_prev = beforeItem->_prev;
			beforeItem->_prev->_next = newItem;
			beforeItem->_prev = newItem;
		}

		return *ObjectFromItem(newItem);
	}

	template<typename T>
	typename List<T>::ListItem *List<T>::New()
	{
		return _itemPool.New();
	}

	template<typename T>
	typename List<T>::ListItem *List<T>::New(const T &obj)
	{
		return _itemPool.New(ListItem(obj));
	}

	template<typename T>
	typename List<T>::ListItem *List<T>::New(T &&obj)
	{
		return _itemPool.New(ListItem(std::move(obj)));
	}

	template<typename T>
	List<T>::ListItem::ListItem()
		: _next(nullptr)
		, _prev(nullptr)
	{
	}

	template<typename T>
	List<T>::ListItem::ListItem(ListItem &&rhs)
		: _val(std::move(rhs._val))
		, _next(rhs._next)
		, _prev(rhs._prev)
	{
		rhs._next = nullptr;
		rhs._prev = nullptr;
	}

	template<typename T>
	List<T>::ListItem::ListItem(const T &rhs)
		: _val(rhs)
		, _next(nullptr)
		, _prev(nullptr)
	{
	}

	template<typename T>
	List<T>::ListItem::ListItem(T &&rhs)
		: _val(std::move(rhs))
		, _next(nullptr)
		, _prev(nullptr)
	{
	}
}
