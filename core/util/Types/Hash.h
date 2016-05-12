#pragma once

namespace ff
{
	typedef uint64_t hash_t;

	// Able to hash any bytes in memory, but it's better to use the hashing templates
	UTIL_API hash_t HashBytes(const void *data, size_t length);

	// Generic function for hashing any type, it should be plain-old-data to work well
	template<typename T>
	hash_t HashFunc(const T &value)
	{
		return HashBytes(&value, sizeof(value));
	}

	template<>
	inline hash_t HashFunc<std::type_index>(const std::type_index &value)
	{
		return HashFunc<size_t>(value.hash_code());
	}

	inline hash_t HashFunc(const wchar_t *value)
	{
		return HashBytes(value, value ? wcslen(value) * sizeof(wchar_t) : 0);
	}

	inline hash_t HashFunc(wchar_t *value)
	{
		return HashBytes(value, value ? wcslen(value) * sizeof(wchar_t) : 0);
	}

	inline hash_t HashFunc(const char *value)
	{
		return HashBytes(value, value ? strlen(value) * sizeof(char) : 0);
	}

	inline hash_t HashFunc(char *value)
	{
		return HashBytes(value, value ? strlen(value) * sizeof(char) : 0);
	}

	// For when an object is really needed and a function pointer won't do
	template<typename T>
	struct Hasher
	{
		hash_t operator()(const T &value)
		{
			return HashFunc<T>(value);
		}
	};

	// For when an object is its own hash
	template<typename T>
	struct NonHasher
	{
		hash_t operator()(const T &value)
		{
			return static_cast<hash_t>(value);
		}
	};
}
