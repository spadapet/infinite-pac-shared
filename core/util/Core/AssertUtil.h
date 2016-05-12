#pragma once

#undef assert
#undef verify

#ifdef _DEBUG

namespace ff
{
	UTIL_API bool AssertCore(const wchar_t *exp, const wchar_t *text, const wchar_t *file, unsigned int line);
}

// The one true assert macro (all others call this)
#define assertSz(exp, txt) \
{ \
	if(!(exp) && !ff::AssertCore(TEXT(#exp), txt, _T(__FILE__), __LINE__)) \
	{ \
		__debugbreak(); \
	} \
}

#else // !_DEBUG

#define assertSz(exp, txt) ((void)0)

#endif

#define assert(exp) assertSz(exp, nullptr)
#define assertRet(exp) { if(!(exp)) { assertSz(false, TEXT(#exp)); return; } }
#define assertRetVal(exp, val) { if(!(exp)) { assertSz(false, TEXT(#exp)); return (val); } }
#define assertSzRet(exp, txt) { if(!(exp)) { assertSz(false, txt); return; } }
#define assertSzRetVal(exp, txt, val) { if(!(exp)) { assertSz(false, txt); return (val); } }
#define assertHr(exp) assertSz(SUCCEEDED(exp), nullptr)
#define assertHrRet(exp) { if(FAILED(exp)) { assertSz(false, TEXT(#exp)); return; } }
#define assertHrRetVal(exp, val) { if(FAILED(exp)) { assertSz(false, TEXT(#exp)); return (val); } }
#define assertHrSzRet(exp, txt) { if(FAILED(exp)) { assertSz(false, txt); return; } }
#define assertHrSzRetVal(exp, txt, val) { if(FAILED(exp)) { assertSz(false, txt); return (val); } }

#define noAssertRet(exp) { if(!(exp)) return; }
#define noAssertRetVal(exp, val) { if(!(exp)) return (val); }
#define noAssertHrRet(exp) { if(FAILED(exp)) return; }
#define noAssertHrRetVal(exp, val) { if(FAILED(exp)) return (val); }

#ifdef _DEBUG
#define verify(exp) assertSz(exp, nullptr)
#define verifySz(exp, txt) assertSz(exp, txt)
#define verifyHr(exp) assertSz(SUCCEEDED(exp), nullptr)
#else
#define verify(exp) (void)(exp)
#define verifySz(exp, txt) (void)(exp)
#define verifyHr(exp) (void)(exp)
#endif
