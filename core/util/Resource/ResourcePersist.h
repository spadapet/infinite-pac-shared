#pragma once

namespace ff
{
	class Dict;
	class IResources;

	class __declspec(uuid("b7c8edb2-dc92-4b56-9a61-d5d36c029d90")) __declspec(novtable)
		IResourceLoad : public IUnknown
	{
	public:
		virtual bool LoadResource(const Dict &dict) = 0;
	};

	class __declspec(uuid("1d364bfa-33e0-480d-9831-a1006ca908d1")) __declspec(novtable)
		IResourceSave : public IResourceLoad
	{
	public:
		virtual bool SaveResource(Dict &dict) = 0;
	};

	UTIL_API bool SaveResource(IUnknown *obj, Dict &dict);
}
