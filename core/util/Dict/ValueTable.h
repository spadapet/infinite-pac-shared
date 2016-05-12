#pragma once

namespace ff
{
	class Dict;
	class Value;

	// Read-only interface to a Dict that can be persisted as a resource
	class __declspec(uuid("4e7f1faf-85eb-47ef-b78d-ab17b9366ea2")) __declspec(novtable)
		IValueTable : public IUnknown
	{
	public:
		virtual ff::Value *GetValue(ff::StringRef name) const = 0;
		virtual ff::String GetString(ff::StringRef name) const = 0;
	};

	UTIL_API bool CreateValueTable(const Dict &dict, IValueTable **obj);
}
