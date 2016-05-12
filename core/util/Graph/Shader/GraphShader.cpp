#include "pch.h"
#include "COM/ComAlloc.h"
#include "Data/Data.h"
#include "Data/DataPersist.h"
#include "Data/DataWriterReader.h"
#include "Data/SavedData.h"
#include "Dict/Dict.h"
#include "Graph/DataBlob.h"
#include "Graph/GraphDevice.h"
#include "Graph/Shader/GraphShader.h"
#include "Graph/VertexFormat.h"
#include "Module/ModuleFactory.h"
#include "Resource/ResourcePersist.h"
#include "String/StringUtil.h"
#include "Windows/FileUtil.h"

static ff::StaticString PROP_DATA(L"data");

namespace ff
{
	class __declspec(uuid("d91a2232-1247-4c34-bbc3-e61e3240e051"))
		GraphShader
			: public ComBase
			, public IGraphShader
			, public IResourceSave
	{
	public:
		DECLARE_HEADER(GraphShader);

		// IGraphShader
		virtual IData *GetData() const override;
		virtual void SetData(IData *pData) override;

		// IResourceSave
		virtual bool LoadResource(const Dict &dict) override;
		virtual bool SaveResource(Dict &dict) override;

	private:
		ComPtr<IData> _data;
	};
}

BEGIN_INTERFACES(ff::GraphShader)
	HAS_INTERFACE(ff::IGraphShader)
	HAS_INTERFACE(ff::IResourceLoad)
	HAS_INTERFACE(ff::IResourceSave)
END_INTERFACES()

static ff::ModuleStartup Register([](ff::Module &module)
{
	static ff::StaticString name(L"shader");
	module.RegisterClassT<ff::GraphShader>(name);
});

bool ff::CreateGraphShader(IGraphShader **ppShader)
{
	return SUCCEEDED(ComAllocator<GraphShader>::CreateInstance(
		nullptr, GUID_NULL, __uuidof(IGraphShader), (void**)ppShader));
}

ff::GraphShader::GraphShader()
{
}

ff::GraphShader::~GraphShader()
{
}

ff::IData *ff::GraphShader::GetData() const
{
	return _data;
}

void ff::GraphShader::SetData(IData *pData)
{
	_data = pData;
}

bool ff::GraphShader::LoadResource(const Dict &dict)
{
	_data = dict.GetData(PROP_DATA);
	assertRetVal(_data, false);

	return true;
}

bool ff::GraphShader::SaveResource(ff::Dict &dict)
{
	noAssertRetVal(_data, false);
	dict.SetData(PROP_DATA, _data);

	return true;
}
