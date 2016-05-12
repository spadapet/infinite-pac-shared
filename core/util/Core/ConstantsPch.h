#pragma once

#if defined(UTIL_DLL)
#define UTIL_API __declspec(dllexport)
#define UTIL_IMPORT __declspec(dllimport)
#define UTIL_EXPORT __declspec(dllexport)
#else
#define UTIL_API __declspec(dllimport)
#define UTIL_IMPORT __declspec(dllimport)
# define UTIL_EXPORT __declspec(dllexport)
#endif

namespace ff
{
	static const double PI_D = 3.1415926535897932384626433832795;
	static const float PI_F = 3.1415926535897932384626433832795f;
	static const float RAD_TO_DEG_F = 360.0f / (2.0f * PI_F);
	static const float DEG_TO_RAD_F = (2.0f * PI_F) / 360.0f;

	// Help iteration with size_t
	const size_t INVALID_SIZE = (size_t)-1;
	const DWORD INVALID_DWORD = (DWORD)-1;
}
