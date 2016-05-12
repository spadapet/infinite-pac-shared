#include "pch.h"
#include "COM/ComObject.h"
#include "Dict/Dict.h"
#include "Resource/ResourcePersist.h"
#include "String/StringUtil.h"

static ff::StaticString RES_TYPE(L"res:type");

bool ff::SaveResource(IUnknown *obj, Dict &dict)
{
	ff::ComPtr<ff::IResourceSave> res;
	assertRetVal(res.QueryFrom(obj), false);

	ff::ComPtr<ff::IComObject> comObj;
	assertRetVal(comObj.QueryFrom(obj), false);

	assertRetVal(res->SaveResource(dict), false);
	dict.SetGuid(RES_TYPE, comObj->GetComClassID());

	return true;
}
