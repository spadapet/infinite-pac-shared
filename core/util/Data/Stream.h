#pragma once

namespace ff
{
	class IData;
	class IDataReader;
	class IDataVector;

	// For standard COM IStreams:
	UTIL_API bool CreateWriteStream(IDataVector *pData, size_t nPos, IStream **ppStream);
	UTIL_API bool CreateWriteStream(IDataVector **ppData, IStream **ppStream); // shortcut
	UTIL_API bool CreateReadStream(IData *pData, size_t nPos, IStream **ppStream);
	UTIL_API bool CreateReadStream(IDataReader *reader, IStream **obj);
	UTIL_API bool CreateReadStream(IDataReader *reader, IMFByteStream **obj);
}
