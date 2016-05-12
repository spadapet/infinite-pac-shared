#pragma once

namespace ff
{
	template<typename Key, typename Value, typename Hash = Hasher<Key>>
	class Map
	{
		typedef KeyValue<Key, Value> KeyValueType;

		struct HashMapKey
		{
			hash_t operator()(const KeyValueType &keyValue)
			{
				return Hash()(keyValue.GetKey());
			}
		};

		typedef Set<KeyValueType, HashMapKey> SetType;

	public:
		Map();
		Map(const Map<Key, Value, Hash> &rhs);
		Map(Map<Key, Value, Hash> &&rhs);
		~Map();

		Map<Key, Value, Hash> &operator=(const Map<Key, Value, Hash> &rhs);

		size_t Size() const;
		BucketIter SetKey(const Key &key, const Value &val); // doesn't allow duplicates
		BucketIter SetKey(Key &&key, Value &&val); // doesn't allow duplicates
		BucketIter Insert(const Key &key, const Value &val); // allows duplicates
		BucketIter Insert(Key &&key, Value &&val); // allows duplicates
		bool DeleteKey(const Key &key);
		BucketIter DeletePos(BucketIter pos); // returns item after the deleted item
		void Clear();
		bool IsEmpty() const;

		bool Exists(const Key &key) const;
		BucketIter Get(const Key &key) const;
		BucketIter GetAt(size_t nIndex) const;
		BucketIter GetNext(BucketIter pos) const;

		const Key &KeyAt(BucketIter pos) const;
		const Value &ValueAt(BucketIter pos) const;
		Value &ValueAt(BucketIter pos);
		hash_t HashAt(BucketIter pos) const;

		// for iteration through the hash table
		BucketIter StartIteration() const;
		BucketIter Iterate(BucketIter pos) const;

		// advanced
		void SetBucketCount(size_t nCount, bool bAllowGrow);
		size_t MemUsage() const;
		void DebugDump() const;

		typename SetType::const_iterator begin() const { return _set.begin(); }
		typename SetType::const_iterator end() const { return _set.end(); }
		typename SetType::const_iterator cbegin() const { return _set.cbegin(); }
		typename SetType::const_iterator cend() const { return _set.cend(); }

	private:
		SetType _set;
	};
}

template<typename Key, typename Value, typename Hash>
ff::Map<Key, Value, Hash>::Map()
{
}

template<typename Key, typename Value, typename Hash>
ff::Map<Key, Value, Hash>::Map(const Map<Key, Value, Hash> &rhs)
	: _set(rhs._set)
{
}

template<typename Key, typename Value, typename Hash>
ff::Map<Key, Value, Hash>::Map(Map<Key, Value, Hash> &&rhs)
	: _set(std::move(rhs._set))
{
}

template<typename Key, typename Value, typename Hash>
ff::Map<Key, Value, Hash>::~Map()
{
}

template<typename Key, typename Value, typename Hash>
ff::Map<Key, Value, Hash> &ff::Map<Key, Value, Hash>::operator=(const Map<Key, Value, Hash> &rhs)
{
	_set = rhs._set;
	return *this;
}

template<typename Key, typename Value, typename Hash>
ff::BucketIter ff::Map<Key, Value, Hash>::SetKey(const Key &key, const Value &val)
{
	return _set.SetKey(KeyValueType(key, val));
}

template<typename Key, typename Value, typename Hash>
ff::BucketIter ff::Map<Key, Value, Hash>::SetKey(Key &&key, Value &&val)
{
	return _set.SetKey(KeyValueType(std::move(key), std::move(val)));
}

template<typename Key, typename Value, typename Hash>
ff::BucketIter ff::Map<Key, Value, Hash>::Insert(const Key &key, const Value &val)
{
	return _set.Insert(KeyValueType(key, val));
}

template<typename Key, typename Value, typename Hash>
ff::BucketIter ff::Map<Key, Value, Hash>::Insert(Key &&key, Value &&val)
{
	return _set.Insert(KeyValueType(std::move(key), std::move(val)));
}

template<typename Key, typename Value, typename Hash>
bool ff::Map<Key, Value, Hash>::DeleteKey(const Key &key)
{
	// Since the value won't be accessed, it's OK to do this dangerous cast
	const KeyValueType &keyValue = *(const KeyValueType *)&key;
	return _set.DeleteKey(keyValue);
}

template<typename Key, typename Value, typename Hash>
ff::BucketIter ff::Map<Key, Value, Hash>::DeletePos(BucketIter pos)
{
	return _set.DeletePos(pos);
}

template<typename Key, typename Value, typename Hash>
void ff::Map<Key, Value, Hash>::Clear()
{
	_set.Clear();
}

template<typename Key, typename Value, typename Hash>
bool ff::Map<Key, Value, Hash>::IsEmpty() const
{
	return _set.IsEmpty();
}

template<typename Key, typename Value, typename Hash>
ff::BucketIter ff::Map<Key, Value, Hash>::Get(const Key &key) const
{
	// Since the value won't be accessed, it's OK to do this dangerous cast
	const KeyValueType &keyValue = *(const KeyValueType *)&key;
	return _set.Get(keyValue);
}

template<typename Key, typename Value, typename Hash>
ff::BucketIter ff::Map<Key, Value, Hash>::GetNext(BucketIter pos) const
{
	return _set.GetNext(pos);
}

template<typename Key, typename Value, typename Hash>
ff::BucketIter ff::Map<Key, Value, Hash>::GetAt(size_t nIndex) const
{
	return _set.GetAt(nIndex);
}

template<typename Key, typename Value, typename Hash>
size_t ff::Map<Key, Value, Hash>::Size() const
{
	return _set.Size();
}

template<typename Key, typename Value, typename Hash>
const Key &ff::Map<Key, Value, Hash>::KeyAt(BucketIter pos) const
{
	return _set.KeyAt(pos).GetKey();
}

template<typename Key, typename Value, typename Hash>
const Value &ff::Map<Key, Value, Hash>::ValueAt(BucketIter pos) const
{
	return _set.KeyAt(pos).GetValue();
}

template<typename Key, typename Value, typename Hash>
Value &ff::Map<Key, Value, Hash>::ValueAt(BucketIter pos)
{
	// Only the key must not be changed, the value doesn't matter
	return const_cast<Value &>(_set.KeyAt(pos).GetValue());
}

template<typename Key, typename Value, typename Hash>
ff::hash_t ff::Map<Key, Value, Hash>::HashAt(BucketIter pos) const
{
	return _set.HashAt(pos);
}

template<typename Key, typename Value, typename Hash>
bool ff::Map<Key, Value, Hash>::Exists(const Key &key) const
{
	return Get(key) != INVALID_ITER;
}

template<typename Key, typename Value, typename Hash>
ff::BucketIter ff::Map<Key, Value, Hash>::StartIteration() const
{
	return _set.StartIteration();
}

template<typename Key, typename Value, typename Hash>
ff::BucketIter ff::Map<Key, Value, Hash>::Iterate(BucketIter pos) const
{
	return _set.Iterate(pos);
}

template<typename Key, typename Value, typename Hash>
void ff::Map<Key, Value, Hash>::SetBucketCount(size_t nCount, bool bAllowGrow)
{
	return _set.SetBucketCount(nCount, bAllowGrow);
}

template<typename Key, typename Value, typename Hash>
size_t ff::Map<Key, Value, Hash>::MemUsage() const
{
	return _set.MemUsage();
}

template<typename Key, typename Value, typename Hash>
void ff::Map<Key, Value, Hash>::DebugDump() const
{
	_set.DebugDump();
}
