#pragma once

namespace ff
{
	class Dict;
	class Log;
	class IData;
	class IDataReader;

	UTIL_API bool SaveDict(const Dict &dict, IData **data);
	UTIL_API bool LoadDict(IDataReader *reader, Dict &dict);
	UTIL_API void DumpDict(ff::StringRef name, const Dict &dict, Log *log, bool debugOnly);
	UTIL_API void DebugDumpDict(const Dict &dict);
}
