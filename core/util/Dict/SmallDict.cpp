#include "pch.h"
#include "Dict/SmallDict.h"
#include "Dict/Value.h"
#include "Globals/ProcessGlobals.h"

ff::SmallDict::SmallDict()
	: _data(nullptr)
{
}

ff::SmallDict::SmallDict(const SmallDict &rhs)
	: _data(nullptr)
{
	*this = rhs;
}

ff::SmallDict::SmallDict(SmallDict &&rhs)
	: _data(rhs._data)
{
	rhs._data = nullptr;
}

ff::SmallDict::~SmallDict()
{
	Clear();
}

const ff::SmallDict &ff::SmallDict::operator=(const SmallDict &rhs)
{
	if (this != &rhs)
	{
		Clear();

		size_t size = rhs.Size();
		if (size)
		{
			Reserve(size, false);
			_data->size = size;
			_data->atomizer = rhs._data->atomizer;

			::memcpy(_data->entries, rhs._data->entries, size * sizeof(Entry));

			for (size_t i = 0; i < size; i++)
			{
				_data->entries[i].value->AddRef();
			}
		}
	}

	return *this;
}

size_t ff::SmallDict::Size() const
{
	return _data ? _data->size : 0;
}

size_t ff::SmallDict::Allocated() const
{
	return _data ? _data->allocated : 0;
}

ff::String ff::SmallDict::KeyAt(size_t index) const
{
	assertRetVal(index < Size(), ff::GetEmptyString());

	hash_t hash = _data->entries[index].hash;
	return _data->atomizer->GetString(hash);
}

ff::hash_t ff::SmallDict::KeyHashAt(size_t index) const
{
	assertRetVal(index < Size(), 0);
	return _data->entries[index].hash;
}

ff::Value *ff::SmallDict::ValueAt(size_t index) const
{
	assertRetVal(index < Size(), nullptr);
	return _data->entries[index].value;
}

ff::Value *ff::SmallDict::GetValue(ff::StringRef key) const
{
	size_t i = IndexOf(key);
	return (i != INVALID_SIZE) ? _data->entries[i].value : nullptr;
}

size_t ff::SmallDict::IndexOf(ff::StringRef key) const
{
	size_t size = Size();
	noAssertRetVal(size, INVALID_SIZE);

	hash_t hash = _data->atomizer->GetHash(key);
	Entry *end = _data->entries + size;

	for (Entry *entry = _data->entries; entry != end; entry++)
	{
		if (entry->hash == hash)
		{
			return entry - _data->entries;
		}
	}

	return INVALID_SIZE;
}

void ff::SmallDict::Add(ff::StringRef key, Value *value)
{
	assertRet(value);
	value->AddRef();

	size_t size = Size();
	Reserve(size + 1);

	hash_t hash = _data->atomizer->CacheString(key);

	_data->entries[size].hash = hash;
	_data->entries[size].value = value;
	_data->size++;
}

void ff::SmallDict::Set(ff::StringRef key, Value *value)
{
	if (value == nullptr)
	{
		Remove(key);
		return;
	}

	size_t index = IndexOf(key);
	if (index != INVALID_SIZE)
	{
		SetAt(index, value);
		return;
	}

	Add(key, value);
}

void ff::SmallDict::SetAt(size_t index, Value *value)
{
	assertRet(index < Size());
	if (value)
	{
		value->AddRef();
		_data->entries[index].value->Release();
		_data->entries[index].value = value;
	}
	else
	{
		RemoveAt(index);
	}
}

void ff::SmallDict::Remove(ff::StringRef key)
{
	size_t size = Size();
	if (size)
	{
		hash_t hash = _data->atomizer->GetHash(key);

		for (size_t i = PreviousSize(size); i != INVALID_SIZE; i = PreviousSize(i))
		{
			if (_data->entries[i].hash == hash)
			{
				RemoveAt(i);
			}
		}
	}
}

void ff::SmallDict::RemoveAt(size_t index)
{
	size_t size = Size();
	assertRet(index < size);

	_data->entries[index].value->Release();
	::memmove(_data->entries + index, _data->entries + index + 1, (size - index - 1) * sizeof(Entry));
	_data->size--;
}

void ff::SmallDict::Reserve(size_t newAllocated, bool allowEmptySpace)
{
	size_t oldAllocated = Allocated();
	if (newAllocated > oldAllocated)
	{
		if (allowEmptySpace)
		{
			newAllocated = std::max<size_t>(NearestPowerOfTwo(newAllocated), 4);
		}

		size_t byteSize = sizeof(Data) + newAllocated * sizeof(Entry) - sizeof(Entry);
		_data = (Data *)_aligned_realloc(_data, byteSize, __alignof(Data));
		_data->allocated = newAllocated;
		_data->size = oldAllocated ? _data->size : 0;
		_data->atomizer = oldAllocated ? _data->atomizer : &ProcessGlobals::Get()->GetStringCache();
	}
}

void ff::SmallDict::Clear()
{
	size_t size = Size();
	for (size_t i = 0; i < size; i++)
	{
		_data->entries[i].value->Release();
	}

	_aligned_free(_data);
	_data = nullptr;
}
