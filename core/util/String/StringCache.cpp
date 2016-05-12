#include "pch.h"
#include "String/StringCache.h"

static ff::hash_t ParseHash(ff::StringRef key)
{
	ff::hash_t hash = 0;

	if (key.size() == 18 && key[0] == L'#' && key[1] == L'x')
	{
		int chars = 0;
		if (_snwscanf_s(key.c_str() + 2, 16, L"%I64x%n", &hash, &chars) != 1 || chars != 16)
		{
			hash = 0;
		}
	}

	return hash;
}

static ff::String HashToString(ff::hash_t hash)
{
	return ff::String::format_new(L"#x%016I64x", hash);
}

ff::StringCache::StringCache(bool threadSafe)
	: _lock(threadSafe)
{
}

ff::StringCache::~StringCache()
{
}

ff::hash_t ff::StringCache::GetHash(ff::StringRef str)
{
	return InternalGetHash(str, false);
}

ff::hash_t ff::StringCache::CacheString(ff::StringRef str)
{
	return InternalGetHash(str, true);
}

ff::hash_t ff::StringCache::InternalGetHash(ff::StringRef str, bool cacheString)
{
	ff::hash_t hash = ParseHash(str);

	if (hash)
	{
		// No need to cache these, they aren't the original string that produced the hash
		cacheString = false;
	}
	else
	{
		hash = ff::HashFunc(str);
	}

	if (cacheString)
	{
		bool exists;
		{
			ff::LockReader crit(_lock);
			exists = _atomToString.Exists(hash);
		}

		if (!exists)
		{
			ff::LockWriter crit(_lock);
			_atomToString.SetKey(hash, str);
		}
	}

	return hash;
}

ff::String ff::StringCache::GetString(ff::hash_t hash) const
{
	// See if I've ever cached the real string before
	{
		ff::LockReader crit(_lock);
		ff::BucketIter iter = _atomToString.Get(hash);

		if (iter != INVALID_ITER)
		{
			return _atomToString.ValueAt(iter);
		}
	}

	return HashToString(hash);
}

void ff::StringCache::Clear()
{
	ff::LockWriter lock(_lock);
	_atomToString.Clear();
}
