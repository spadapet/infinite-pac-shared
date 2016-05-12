#pragma once

namespace ff
{
	typedef void *BucketIter;
	static const BucketIter INVALID_ITER = nullptr;

	template<typename Key, typename Hash = Hasher<Key>>
	class Set
	{
	public:
		Set();
		Set(const Set<Key, Hash> &rhs);
		Set(Set<Key, Hash> &&rhs);
		~Set();

		Set<Key, Hash> &operator=(const Set<Key, Hash> &rhs);

		size_t Size() const;
		BucketIter SetKey(const Key &key); // doesn't allow duplicates
		BucketIter SetKey(Key &&key); // doesn't allow duplicates
		BucketIter Insert(const Key &key); // allows duplicates
		BucketIter Insert(Key &&key); // allows duplicates
		bool DeleteKey(const Key &key); // deletes all matching keys
		BucketIter DeletePos(BucketIter pos); // returns item after the deleted item
		void Clear();
		bool IsEmpty() const;

		bool Exists(const Key &key) const;
		BucketIter Get(const Key &key) const;
		BucketIter GetAt(size_t nIndex) const;
		BucketIter GetNext(BucketIter pos) const;

		const Key &KeyAt(BucketIter pos) const;
		hash_t HashAt(BucketIter pos) const;

		// for iteration through the hash table
		BucketIter StartIteration() const;
		BucketIter Iterate(BucketIter pos) const;

		// advanced
		void SetBucketCount(size_t nCount, bool bAllowGrow);
		size_t MemUsage() const;
		void DebugDump() const;

	protected:
		Key *NewKey(const Key &rhs);
		Key *NewKey(Key &&rhs);
		BucketIter InsertAllocatedKey(Key *pKey, bool bAllowDupes);

	private:
		struct SEntry
		{
			ff::hash_t _hash;
			Key *_key;

			bool operator<(const SEntry &rhs) const
			{
				return (_hash <= rhs._hash) && (_hash < rhs._hash || *_key < *rhs._key);
			}
		};

		typedef Vector<SEntry> BucketType;

		SEntry *GetEntry(BucketIter pos) const { return (SEntry *)pos; }
		BucketIter GetBucketIter(SEntry *pEntry) const { return (BucketIter)pEntry; }
		BucketType *GetBucket(SEntry *pEntry) const { return _buckets[GetHashBucket(pEntry->_hash)]; }
		size_t GetHashBucket(hash_t hash) const { return hash & (_buckets.Size() - 1); }
		size_t GetBucketIndex(SEntry *pEntry) const { return pEntry - GetBucket(pEntry)->Data(); }

		size_t NextBucketCount(size_t nCurCount, bool grow) const;
		bool NeedsResize(bool grow) const;
		void ResizeBuckets(size_t count);
		BucketIter InsertEntry(SEntry *pEntry, bool bAllowDupes);

		size_t _size;
		size_t _maxHold;
		size_t _minHold;
		Vector<BucketType *> _buckets;
		PoolAllocator<Key> _keyPool;

	// Imperfect C++ iterators
	public:
		template<typename IT>
		class Iterator : public std::iterator<std::input_iterator_tag, IT>
		{
			typedef Iterator<IT> MyType;
			typedef Set<Key, Hash> SetType;

		public:
			Iterator(const SetType *owner, BucketIter iter)
			{
				_owner = owner;
				_iter = iter;
			}

			Iterator(const MyType &rhs)
			{
				_owner = rhs._owner;
				_iter = rhs._iter;
			}

			const IT &operator*() const
			{
				return _owner->KeyAt(_iter);
			}

			const IT *operator->() const
			{
				return &_owner->KeyAt(_iter);
			}

			MyType &operator++()
			{
				_iter = _owner->Iterate(_iter);
				return *this;
			}

			MyType operator++(int)
			{
				MyType pre = *this;
				_iter = _owner->Iterate(_iter);
				return pre;
			}

			bool operator==(const MyType &rhs) const
			{
				return _owner == rhs._owner && _iter == rhs._iter;
			}

			bool operator!=(const MyType &rhs) const
			{
				return _owner != rhs._owner || _iter != rhs._iter;
			}

		private:
			const SetType *_owner;
			BucketIter _iter;
		};

		typedef Iterator<Key> const_iterator;

		const_iterator begin() const { return const_iterator(this, StartIteration()); }
		const_iterator end() const { return const_iterator(this, nullptr); }
		const_iterator cbegin() const { return const_iterator(this, StartIteration()); }
		const_iterator cend() const { return const_iterator(this, nullptr); }
	};
}

template<typename Key, typename Hash>
ff::Set<Key, Hash>::Set()
	: _size(0)
	, _maxHold(0)
	, _minHold(0)
	, _keyPool(false)
{
}

template<typename Key, typename Hash>
ff::Set<Key, Hash>::Set(const Set<Key, Hash> &rhs)
	: _size(0)
	, _maxHold(0)
	, _minHold(0)
	, _keyPool(false)
{
	*this = rhs;
}

template<typename Key, typename Hash>
ff::Set<Key, Hash>::Set(Set<Key, Hash> &&rhs)
	: _size(rhs._size)
	, _maxHold(rhs._maxHold)
	, _minHold(rhs._minHold)
	, _buckets(std::move(rhs._buckets))
	, _keyPool(std::move(rhs._keyPool))
{
	rhs._size = 0;
	rhs._maxHold = 0;
	rhs._minHold = 0;
}

template<typename Key, typename Hash>
ff::Set<Key, Hash>::~Set()
{
	_maxHold = _minHold;
	Clear();
}

template<typename Key, typename Hash>
ff::Set<Key, Hash> &ff::Set<Key, Hash>::operator=(const Set<Key, Hash> &rhs)
{
	if (this != &rhs)
	{
		Clear();
		ResizeBuckets(rhs._buckets.Size());

		for (BucketIter pos = rhs.StartIteration(); pos != INVALID_ITER; pos = rhs.Iterate(pos))
		{
			Insert(rhs.KeyAt(pos));
		}
	}

	return *this;
}

template<typename Key, typename Hash>
size_t ff::Set<Key, Hash>::Size() const
{
	return _size;
}

template<typename Key, typename Hash>
ff::BucketIter ff::Set<Key, Hash>::SetKey(const Key &key)
{
	SEntry entry;
	entry._hash = Hash()(key);
	entry._key = NewKey(key);

	return InsertEntry(&entry, false);
}

template<typename Key, typename Hash>
ff::BucketIter ff::Set<Key, Hash>::SetKey(Key &&key)
{
	SEntry entry;
	entry._hash = Hash()(key);
	entry._key = NewKey(std::move(key));

	return InsertEntry(&entry, false);
}

template<typename Key, typename Hash>
ff::BucketIter ff::Set<Key, Hash>::Insert(const Key &key)
{
	SEntry entry;
	entry._hash = Hash()(key);
	entry._key = NewKey(key);

	return InsertEntry(&entry, true);
}

template<typename Key, typename Hash>
ff::BucketIter ff::Set<Key, Hash>::Insert(Key &&key)
{
	SEntry entry;
	entry._hash = Hash()(key);
	entry._key = NewKey(std::move(key));

	return InsertEntry(&entry, true);
}

template<typename Key, typename Hash>
bool ff::Set<Key, Hash>::DeleteKey(const Key &key)
{
	BucketIter pos = Get(key);

	if (pos != INVALID_ITER)
	{
		SEntry *pEntry = GetEntry(pos);
		BucketType *pBucket = GetBucket(pEntry);
		size_t index = GetBucketIndex(pEntry);
		size_t count = 1;

		for (size_t next = index + count;
			next < pBucket->Size() &&
				pEntry->_hash == pBucket->GetAt(next)._hash &&
				!(key < *pBucket->GetAt(next)._key);
			count++, next++)
		{
			// find all duplicate keys to delete
		}

		for (size_t i = index; i < index + count; i++)
		{
			_keyPool.Delete(pBucket->GetAt(i)._key);
		}

		pBucket->Delete(index, count);

		_size -= count;

		return true;
	}

	return false;
}

template<typename Key, typename Hash>
ff::BucketIter ff::Set<Key, Hash>::DeletePos(BucketIter pos)
{
	SEntry *pNextEntry = nullptr;

	if (pos != INVALID_ITER)
	{
		SEntry *pEntry = GetEntry(pos);
		size_t index = GetBucketIndex(pEntry);
		BucketType *pBucket = GetBucket(pEntry);

		_keyPool.Delete(pEntry->_key);
		pBucket->Delete(index);

		_size--;

		// Find the next entry, either in the same bucket or the next one
		if (index < pBucket->Size())
		{
			pNextEntry = &pBucket->GetAt(index);
		}
		else
		{
			size_t bucket = GetHashBucket(pEntry->_hash) + 1;

			while (bucket < _buckets.Size() && (!_buckets[bucket] || !_buckets[bucket]->Size()))
			{
				bucket++;
			}

			if (bucket < _buckets.Size())
			{
				pNextEntry = &_buckets[bucket]->GetAt(0);
			}
		}
	}

	return GetBucketIter(pNextEntry);
}

template<typename Key, typename Hash>
void ff::Set<Key, Hash>::Clear()
{
	bool bFastClear = (_maxHold == INVALID_SIZE);

	for (size_t i = 0; i < _buckets.Size(); i++)
	{
		BucketType *pBucket = _buckets[i];

		if (pBucket)
		{
			_size -= pBucket->Size();

			for (size_t h = 0; h < pBucket->Size(); h++)
			{
				_keyPool.Delete(pBucket->GetAt(h)._key);
			}

			if (bFastClear)
			{
				pBucket->Clear();
			}
			else
			{
				_buckets[i] = nullptr;
				delete pBucket;
			}
		}
	}

	assert(!_size);

	if (!_size && !bFastClear)
	{
		_buckets.Clear();
	}
}

template<typename Key, typename Hash>
bool ff::Set<Key, Hash>::IsEmpty() const
{
	return _size == 0;
}

template<typename Key, typename Hash>
bool ff::Set<Key, Hash>::Exists(const Key &key) const
{
	return Get(key) != INVALID_ITER;
}

template<typename Key, typename Hash>
ff::BucketIter ff::Set<Key, Hash>::Get(const Key &key) const
{
	if (_size)
	{
		hash_t hash = Hash()(key);
		size_t bucket = GetHashBucket(hash);
		BucketType *pBucket = _buckets[bucket];

		if (pBucket)
		{
			SEntry entry;
			entry._hash = hash;
			entry._key = const_cast<Key*>(&key);

			size_t index;
			if (pBucket->SortFind(entry, &index))
			{
				return GetBucketIter(&pBucket->GetAt(index));
			}
		}
	}

	return GetBucketIter(nullptr);
}

template<typename Key, typename Hash>
ff::BucketIter ff::Set<Key, Hash>::GetAt(size_t nIndex) const
{
	if (nIndex < _size)
	{
		for (size_t i = 0, nStart = 0; i < _buckets.Size(); i++)
		{
			BucketType *pBucket = _buckets.GetAt(i);

			if (pBucket)
			{
				if (nIndex < nStart + pBucket->Size())
				{
					SEntry *pEntry = &pBucket->GetAt(nIndex - nStart);
					return GetBucketIter(pEntry);
				}
				else
				{
					nStart += pBucket->Size();
				}
			}
		}
	}

	assertRetVal(false, GetBucketIter(nullptr));
}

template<typename Key, typename Hash>
ff::BucketIter ff::Set<Key, Hash>::GetNext(BucketIter pos) const
{
	if (pos != INVALID_ITER)
	{
		SEntry *pEntry = GetEntry(pos);
		BucketType *pBucket = GetBucket(pEntry);
		size_t index = GetBucketIndex(pEntry);

		if (index + 1 < pBucket->Size() &&
			pEntry->_hash == pBucket->GetAt(index + 1)._hash &&
			!(*pEntry->_key < *pBucket->GetAt(index + 1)._key))
		{
			return GetBucketIter(&pBucket->GetAt(index + 1));
		}
	}

	return GetBucketIter(nullptr);
}

template<typename Key, typename Hash>
const Key &ff::Set<Key, Hash>::KeyAt(BucketIter pos) const
{
	assert(pos != INVALID_ITER);
	return *GetEntry(pos)->_key;
}

template<typename Key, typename Hash>
ff::hash_t ff::Set<Key, Hash>::HashAt(BucketIter pos) const
{
	assert(pos != INVALID_ITER);
	return GetEntry(pos)->_hash;
}

template<typename Key, typename Hash>
ff::BucketIter ff::Set<Key, Hash>::StartIteration() const
{
	if (_size)
	{
		for (size_t i = 0; i < _buckets.Size(); i++)
		{
			BucketType *pBucket = _buckets[i];

			if (pBucket && pBucket->Size())
			{
				return GetBucketIter(&pBucket->GetAt(0));
			}
		}
	}

	return GetBucketIter(nullptr);
}

template<typename Key, typename Hash>
ff::BucketIter ff::Set<Key, Hash>::Iterate(BucketIter pos) const
{
	assert(pos != INVALID_ITER);

	if (pos != INVALID_ITER)
	{
		SEntry *pEntry = GetEntry(pos);
		size_t index = GetBucketIndex(pEntry);
		BucketType *pBucket = GetBucket(pEntry);

		if (index + 1 < pBucket->Size())
		{
			return GetBucketIter(&pBucket->GetAt(index + 1));
		}
		else
		{
			size_t bucket = GetHashBucket(pEntry->_hash) + 1;

			while (bucket < _buckets.Size() && (!_buckets[bucket] || !_buckets[bucket]->Size()))
			{
				bucket++;
			}

			if (bucket < _buckets.Size())
			{
				return GetBucketIter(&_buckets[bucket]->GetAt(0));
			}
		}
	}

	return GetBucketIter(nullptr);
}

template<typename Key, typename Hash>
size_t ff::Set<Key, Hash>::NextBucketCount(size_t nCurCount, bool grow) const
{
	size_t nBuckets = grow ? (nCurCount * 2) : (nCurCount / 2);
	nBuckets = std::max<size_t>(nBuckets, 16);
	nBuckets = std::min<size_t>(nBuckets, 16384);

	return nBuckets;
}

template<typename Key, typename Hash>
bool ff::Set<Key, Hash>::NeedsResize(bool grow) const
{
	return
		!_buckets.Size() ||
		( grow && _size > _maxHold) ||
		(!grow && _size < _minHold);
}

template<typename Key, typename Hash>
void ff::Set<Key, Hash>::SetBucketCount(size_t nCount, bool bAllowGrow)
{
	nCount = NearestPowerOfTwo(nCount);
	nCount = NextBucketCount(nCount, true);
	nCount = NextBucketCount(nCount, false);

	ResizeBuckets(nCount);

	// prevent shrinking upon the next deletion:
	_minHold = 0;
	_maxHold = bAllowGrow ? _maxHold : INVALID_SIZE;
}

template<typename Key, typename Hash>
size_t ff::Set<Key, Hash>::MemUsage() const
{
	size_t nMem = _keyPool.MemUsage() + _buckets.ByteSize();

	for (size_t i = 0; i < _buckets.Size(); i++)
	{
		if (_buckets[i])
		{
			nMem += _buckets[i]->ByteSize();
		}
	}

	return nMem;
}

template<typename Key, typename Hash>
void ff::Set<Key, Hash>::DebugDump() const
{
	Log::DebugTraceF(L"Size %lu, with %lu buckets.\n", Size(), _buckets.Size());
	Log::DebugTraceF(L"---------------------------\n");

	for (size_t i = 0; i < _buckets.Size(); i++)
	{
		if (_buckets[i])
		{
			Log::DebugTraceF(L"Bucket:%lu, Size:%lu, Alloc:%lu\n", i, _buckets[i]->Size(), _buckets[i]->Allocated());
		}
		else
		{
			Log::DebugTraceF(L"Bucket:%lu, <null>\n", i);
		}
	}
}

template<typename Key, typename Hash>
void ff::Set<Key, Hash>::ResizeBuckets(size_t count)
{
	if (count == _buckets.Size())
	{
		return;
	}

	// remember all current entries
	Vector<SEntry> entries;
	entries.Reserve(_size);

	for (size_t i = 0; i < _buckets.Size(); i++)
	{
		BucketType *pBucket = _buckets[i];

		if (pBucket && pBucket->Size())
		{
			entries.Push(pBucket->Data(), pBucket->Size());
		}
	}

	// delete extra buckets
	while (count < _buckets.Size())
	{
		delete _buckets.Pop();
	}

	// clear existing buckets
	for (size_t i = 0; i < _buckets.Size(); i++)
	{
		if (_buckets[i])
		{
			_buckets[i]->Clear();
		}
	}

	size_t oldSize = _size;
	_size = 0;
	_minHold = 0;
	_maxHold = INVALID_SIZE;

	// create new buckets
	while (count > _buckets.Size())
	{
		_buckets.Push(nullptr);
	}

	// add all entries back
	for (size_t i = 0; i < entries.Size(); i++)
	{
		InsertEntry(&entries[i], true);
	}

	// make sure that bucket entry arrays aren't too empty
	for (size_t i = 0; i < _buckets.Size(); i++)
	{
		BucketType *pBucket = _buckets[i];

		if (pBucket && !entries.Size())
		{
			delete pBucket;
			_buckets[i] = nullptr;
		}
		else if (pBucket && pBucket->Allocated() / 2 > pBucket->Size())
		{
			pBucket->Reduce();
		}
	}

	_maxHold = (_buckets.Size() / 3) * _buckets.Size();

	_minHold = (_buckets.Size() > 32)
		? (_buckets.Size() / 8) * (_buckets.Size() / 2)
		: 0;

	assert(_size == oldSize);
}

template<typename Key, typename Hash>
Key *ff::Set<Key, Hash>::NewKey(const Key &rhs)
{
	return _keyPool.New(rhs);
}

template<typename Key, typename Hash>
Key *ff::Set<Key, Hash>::NewKey(Key &&rhs)
{
	return _keyPool.New(std::move(rhs));
}

template<typename Key, typename Hash>
ff::BucketIter ff::Set<Key, Hash>::InsertAllocatedKey(Key *pKey, bool bAllowDupes)
{
	SEntry entry;
	entry._hash = Hash()(*pKey);
	entry._key = pKey;

	return InsertEntry(&entry, bAllowDupes);
}

template<typename Key, typename Hash>
ff::BucketIter ff::Set<Key, Hash>::InsertEntry(SEntry *pEntry, bool bAllowDupes)
{
	if (NeedsResize(true))
	{
		ResizeBuckets(NextBucketCount(_buckets.Size(), true));
	}

	size_t bucket = GetHashBucket(pEntry->_hash);
	BucketType *pBucket = _buckets[bucket];

	if (!pBucket)
	{
		pBucket = new BucketType;
		_buckets[bucket] = pBucket;
	}

	size_t pos = INVALID_SIZE;
	if (pBucket->SortFind(*pEntry, &pos) && !bAllowDupes)
	{
		// Delete dupes of the existing entry
		size_t dupeCount = 0;

		for (size_t next = pos + 1;
			next < pBucket->Size() &&
				pEntry->_hash == pBucket->GetAt(next)._hash &&
				!(*pEntry->_key < *pBucket->GetAt(next)._key);
			dupeCount++, next++)
		{
			// find all duplicate keys to delete
		}

		if (dupeCount != 0)
		{
			for (size_t i = pos + 1; i < pos + 1 + dupeCount; i++)
			{
				_keyPool.Delete(pBucket->GetAt(i)._key);
			}

			pBucket->Delete(pos + 1, dupeCount);
			_size -= dupeCount;
		}

		// Update the existing entry

		SEntry &oldEntry = pBucket->GetAt(pos);
		*oldEntry._key = *pEntry->_key;
		_keyPool.Delete(pEntry->_key);

		return GetBucketIter(&oldEntry);
	}
	else
	{
		pBucket->Insert(pos, *pEntry);

		_size++;

		return GetBucketIter(&pBucket->GetAt(pos));
	}
}
