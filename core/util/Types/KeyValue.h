#pragma once

namespace ff
{
	// Kind of like std::pair or std::tuple, but the comparisons and hashing
	// are only performed on the key type.
	template<typename Key, typename Value>
	class KeyValue
	{
	public:
		KeyValue(const KeyValue<Key, Value> &rhs);
		KeyValue(KeyValue<Key, Value> &&rhs);
		KeyValue(const Key &key, const Value &val);
		KeyValue(Key &&key, Value &&val);

		KeyValue<Key, Value> &operator=(const KeyValue<Key, Value> &rhs);
		KeyValue<Key, Value> &operator=(KeyValue<Key, Value> &&rhs);
		bool operator==(const KeyValue<Key, Value> &rhs) const;
		bool operator!=(const KeyValue<Key, Value> &rhs) const;
		bool operator<(const KeyValue<Key, Value> &rhs) const;

		const Key &GetKey() const;
		const Value &GetValue() const;
		Value &GetValue();
		Value &GetEditableValue() const;

	private:
		Key _key;
		Value _val;
	};

	template<typename Key, typename Value>
	struct HashKeyValue
	{
		size_t operator()(const KeyValue<Key, Value> &rhs)
		{
			return HashFunc<Key>(rhs._key);
		}
	};
}

template<typename Key, typename Value>
ff::KeyValue<Key, Value>::KeyValue(const KeyValue<Key, Value> &rhs)
	: _key(rhs._key)
	, _val(rhs._val)
{
}

template<typename Key, typename Value>
ff::KeyValue<Key, Value>::KeyValue(KeyValue<Key, Value> &&rhs)
	: _key(std::move(rhs._key))
	, _val(std::move(rhs._val))
{
}

template<typename Key, typename Value>
ff::KeyValue<Key, Value>::KeyValue(const Key &key, const Value &val)
	: _key(key)
	, _val(val)
{
}

template<typename Key, typename Value>
ff::KeyValue<Key, Value>::KeyValue(Key &&key, Value &&val)
	: _key(std::move(key))
	, _val(std::move(val))
{
}

template<typename Key, typename Value>
ff::KeyValue<Key, Value> &ff::KeyValue<Key, Value>::operator=(const KeyValue<Key, Value> &rhs)
{
	if (this != &rhs)
	{
		_key = rhs._key;
		_val = rhs._val;
	}

	return *this;
}

template<typename Key, typename Value>
ff::KeyValue<Key, Value> &ff::KeyValue<Key, Value>::operator=(KeyValue<Key, Value> &&rhs)
{
	_key = std::move(rhs._key);
	_val = std::move(rhs._val);

	return *this;
}

template<typename Key, typename Value>
bool ff::KeyValue<Key, Value>::operator==(const KeyValue<Key, Value> &rhs) const
{
	return _key == rhs._key;
}

template<typename Key, typename Value>
bool ff::KeyValue<Key, Value>::operator!=(const KeyValue<Key, Value> &rhs) const
{
	return !(*this == rhs);
}

template<typename Key, typename Value>
bool ff::KeyValue<Key, Value>::operator<(const KeyValue<Key, Value> &rhs) const
{
	return _key < rhs._key;
}

template<typename Key, typename Value>
const Key &ff::KeyValue<Key, Value>::GetKey() const
{
	return _key;
}

template<typename Key, typename Value>
const Value &ff::KeyValue<Key, Value>::GetValue() const
{
	return _val;
}

template<typename Key, typename Value>
Value &ff::KeyValue<Key, Value>::GetEditableValue() const
{
	return const_cast<Value &>(_val);
}

template<typename Key, typename Value>
Value &ff::KeyValue<Key, Value>::GetValue()
{
	return _val;
}
