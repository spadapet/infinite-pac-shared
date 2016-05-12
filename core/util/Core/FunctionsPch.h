#pragma once

namespace ff
{
	inline size_t PreviousSize(size_t size)
	{
		return (size && size != INVALID_SIZE) ? size - 1 : INVALID_SIZE;
	}

	inline bool EqualEnough(float f1, float f2)
	{
		return std::abs(f1 - f2) < 0.0001220703125f;
	}

	template<typename T>
	void ZeroObject(T &t)
	{
		memset(&t, 0, sizeof(T));
	}

	template<typename T>
	int CompareObjects(const T &lhs, const T &rhs)
	{
		return memcmp(&lhs, &rhs, sizeof(T));
	}

	template<typename T>
	void CopyObject(T &dest, const T &src)
	{
		if (&dest != &src)
		{
			memcpy(&dest, &src, sizeof(T));
		}
	}

	template<typename T>
	T Clamp(T value, T minValue, T maxValue)
	{
		if (value > maxValue)
		{
			return maxValue;
		}
		else if (value < minValue)
		{
			return minValue;
		}

		return value;
	}

	inline static size_t NearestPowerOfTwo(size_t num)
	{
		num = num ? num - 1 : 0;

		num |= num >> 1;
		num |= num >> 2;
		num |= num >> 4;
		num |= num >> 8;
		num |= num >> 16;
#ifdef _WIN64
		num |= num >> 32;
#endif

		return num + 1;
	}

	template<typename T>
	void MakeUnshared(std::shared_ptr<T> &obj)
	{
		if (obj == nullptr)
		{
			obj = std::make_shared<T>();
		}
		else if (!obj.unique())
		{
			std::shared_ptr<T> origObj(obj);
			obj = std::make_shared<T>(*origObj.get());
		}
	}

#ifdef _WIN64
	inline long InterlockedAccess(const long &num)
	{
		return InterlockedOr(const_cast<long *>(&num), 0);
	}
#else
#include <intrin.h>
	inline long InterlockedAccess(const long &num)
	{
		return _InterlockedOr(const_cast<long *>(&num), 0);
	}
#endif
}
