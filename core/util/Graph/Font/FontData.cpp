#include "pch.h"
#include "COM/ComAlloc.h"
#include "Data/Data.h"
#include "Data/DataPersist.h"
#include "Data/DataWriterReader.h"
#include "Data/SavedData.h"
#include "Graph/Font/FontData.h"
#include "Module/ModuleFactory.h"
#include "Windows/FileUtil.h"
#include "Windows/Handles.h"

namespace ff
{
	class __declspec(uuid("635b4c4f-4d55-4b5d-aa00-a197fc4cfb7a"))
		FontData : public ComBase, public IFontData
	{
	public:
		DECLARE_HEADER(FontData);

		// IFontData functions
		virtual bool SetFile(StringRef path) override;
		virtual bool SetData(IData *pData) override;

		// IData functions
		virtual const BYTE *GetMem() override;
		virtual size_t GetSize() override;
		virtual IDataFile *GetFile() override;
		virtual bool IsStatic() override;

	private:
		bool RegisterFontFamily();

		ComPtr<IData> _data;
#if !METRO_APP
		FontResourceHandle _font;
#endif
	};
}

BEGIN_INTERFACES(ff::FontData)
	HAS_INTERFACE(ff::IData)
	HAS_INTERFACE(ff::IFontData)
END_INTERFACES()

static ff::ModuleStartup Register([](ff::Module &module)
{
	static ff::StaticString name(L"ttf");
	module.RegisterClassT<ff::FontData>(name, __uuidof(ff::IFontData));
});

bool ff::CreateFontData(IFontData **ppData)
{
	return SUCCEEDED(ComAllocator<FontData>::CreateInstance(
		nullptr, GUID_NULL, __uuidof(IFontData), (void**)ppData));
}

ff::FontData::FontData()
{
}

ff::FontData::~FontData()
{
}

bool ff::FontData::SetFile(StringRef path)
{
	assertRetVal(!_data, false);
	assertRetVal(ReadWholeFile(path, &_data), false);

	assertRetVal(RegisterFontFamily(), false);

	return true;
}

bool ff::FontData::SetData(IData *pData)
{
	assertRetVal(!_data && pData, false);
	_data = pData;

	assertRetVal(RegisterFontFamily(), false);

	return true;
}

const BYTE *ff::FontData::GetMem()
{
	return _data ? _data->GetMem() : nullptr;
}

size_t ff::FontData::GetSize()
{
	return _data ? _data->GetSize() : 0;
}

ff::IDataFile *ff::FontData::GetFile()
{
	return nullptr;
}

bool ff::FontData::IsStatic()
{
	return false;
}

bool ff::FontData::RegisterFontFamily()
{
#if !METRO_APP
	assertRetVal(!_font && _data, false);

	DWORD nCount = 0;
	_font = AddFontMemResourceEx(const_cast<LPBYTE>(GetMem()), (DWORD)GetSize(), nullptr, &nCount);
	assertRetVal(_font && nCount, false);
#endif

	return true;
}
