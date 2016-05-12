#pragma once

namespace ff
{
	class StringCache
	{
	public:
		UTIL_API StringCache(bool threadSafe = true);
		UTIL_API ~StringCache();

		UTIL_API ff::hash_t GetHash(ff::StringRef str);
		UTIL_API ff::hash_t CacheString(ff::StringRef str);
		UTIL_API ff::String GetString(ff::hash_t hash) const;
		UTIL_API void Clear();

	protected:
		ff::Map<ff::hash_t, ff::String, ff::NonHasher<ff::hash_t>> _atomToString;
		ReaderWriterLock _lock;

	private:
		ff::hash_t InternalGetHash(ff::StringRef str, bool cacheString);

		// not allowed
		StringCache(const StringCache &r);
		const StringCache &operator=(const StringCache &r);
	};
}
