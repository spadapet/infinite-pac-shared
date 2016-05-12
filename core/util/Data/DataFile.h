#pragma once

namespace ff
{
	class File;
	class IDataFile;

	UTIL_API bool CreateDataFile(StringRef path, bool bTempFile, IDataFile **ppDataFile);
	UTIL_API bool CreateTempDataFile(IDataFile **ppDataFile);

	class __declspec(uuid("d169c657-afba-41db-8b62-8f91e52b572f")) __declspec(novtable)
		IDataFile : public IUnknown
	{
	public:
		virtual bool OpenWrite(File &handle, bool bAppend = false) = 0;
		virtual bool OpenRead(File &handle) = 0;

		virtual bool OpenReadMemMapped() = 0;
		virtual bool CloseMemMapped() = 0;
		virtual const BYTE *GetMem() const = 0;

		virtual ff::StringRef GetPath() const = 0;
		virtual size_t GetSize() const = 0;
	};
}
