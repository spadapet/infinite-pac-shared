#pragma once

namespace ff
{
	class IDataWriter;

	class Log
	{
	public:
		UTIL_API Log();
		UTIL_API ~Log();

		UTIL_API bool GetConsoleOutput() const;
		UTIL_API void SetConsoleOutput(bool console);

		UTIL_API IDataWriter *AddFile(StringRef path, bool bAppend = false);
		UTIL_API bool AddWriter(IDataWriter *writer);
		UTIL_API bool RemoveWriter(IDataWriter *writer);
		UTIL_API void RemoveAllWriters();

		UTIL_API void Trace(const wchar_t *szText);
		UTIL_API void TraceF(const wchar_t *szFormat, ...);
		UTIL_API void TraceV(const wchar_t *szFormat, va_list args);
		UTIL_API void TraceLine(const wchar_t *szText);
		UTIL_API void TraceLineF(const wchar_t *szFormat, ...);
		UTIL_API void TraceLineV(const wchar_t *szFormat, va_list args);
		UTIL_API void TraceSpaces(size_t nSpaces);

		UTIL_API static void GlobalTrace(const wchar_t *szText);
		UTIL_API static void GlobalTraceF(const wchar_t *szFormat, ...);
		UTIL_API static void GlobalTraceV(const wchar_t *szFormat, va_list args);

		UTIL_API static void DebugTrace(const wchar_t *szText);
		UTIL_API static void DebugTraceF(const wchar_t *szFormat, ...);
		UTIL_API static void DebugTraceV(const wchar_t *szFormat, va_list args);

	private:
		Mutex _cs;
		bool _console;
		std::vector<ComPtr<IDataWriter>> _writers;
	};
}
