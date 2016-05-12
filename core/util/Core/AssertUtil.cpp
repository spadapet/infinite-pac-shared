#include "pch.h"
#include "Globals/Log.h"
#include "Globals/ProcessGlobals.h"
#include "Thread/ThreadDispatch.h"
#include "Thread/ThreadPool.h"
#include "Thread/ThreadUtil.h"
#include "Windows/WinUtil.h"

#ifdef _DEBUG

static bool s_showingAssertDialog = false;

bool ff::AssertCore(const wchar_t *szExp, const wchar_t *szText, const wchar_t *szFile, unsigned int nLine)
{
	wchar_t dialogText[1024];
	bool ignored = true;

	_snwprintf_s(dialogText, _countof(dialogText), _TRUNCATE,
		L"ASSERT: %s\r\nExpression: %s\r\nFile: %s\r\nLine: %u\r\n\r\nBreak?",
		szText ? szText : L"",
		szExp ? szExp : L"",
		szFile ? szFile : L"",
		nLine);

	bool mainThread =
		ff::GetMainThreadDispatch() &&
		ff::GetMainThreadDispatch()->IsCurrentThread();

#if METRO_APP
	ff::Log::GlobalTrace(dialogText);
	ignored = false;
#else
	::OutputDebugString(dialogText);

	if (::IsDebuggerPresent() ||
		ff::IsProgramShuttingDown() ||
		ff::GotQuitMessage() ||
		!mainThread ||
		s_showingAssertDialog)
	{
		ignored = false;
	}
	else
	{
		s_showingAssertDialog = true;

		if (::MessageBox(nullptr, dialogText, L"Assertion failure", MB_ICONEXCLAMATION | MB_YESNO) == IDYES)
		{
			ignored = false;
		}

		s_showingAssertDialog = false;
	}
#endif

	return ignored;
}

#endif // _DEBUG
