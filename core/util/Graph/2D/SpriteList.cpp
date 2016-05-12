#include "pch.h"
#include "COM/ComAlloc.h"
#include "Data/Data.h"
#include "Data/DataPersist.h"
#include "Data/DataWriterReader.h"
#include "Dict/Dict.h"
#include "Graph/2D/Sprite.h"
#include "Graph/2D/SpriteList.h"
#include "Graph/2D/SpriteOptimizer.h"
#include "Graph/GraphDevice.h"
#include "Graph/GraphTexture.h"
#include "Module/ModuleFactory.h"
#include "Resource/ResourcePersist.h"
#include "String/StringUtil.h"
#include "Windows/FileUtil.h"

static ff::StaticString PROP_COUNT(L"count");
static ff::StaticString PROP_DATA(L"data");
static ff::StaticString PROP_SPRITES(L"sprites");
static ff::StaticString PROP_TEXTURES(L"textures");

namespace ff
{
	class __declspec(uuid("7ddb9bd1-c9e0-4788-b0b2-3bb252515013"))
		SpriteList
			: public ComBase
			, public ISpriteList
			, public IResourceSave
	{
	public:
		DECLARE_HEADER(SpriteList);

		virtual HRESULT _Construct(IUnknown *unkOuter) override;

		// IGraphDeviceChild functions
		virtual IGraphDevice *GetDevice() const override;
		virtual bool Reset() override;

		// ISpriteList functions
		virtual ISprite *Add(
			IGraphTexture *pTexture,
			StringRef name,
			RectFloat rect,
			PointFloat handle,
			PointFloat scale,
			SpriteType type) override;
		virtual ISprite *Add(ISprite *pSprite) override;
		virtual bool Add(ISpriteList *pList) override;

		virtual size_t GetCount() override;
		virtual ISprite *Get(size_t nSprite) override;
		virtual ISprite *Get(StringRef name) override;
		virtual StringRef GetName(size_t nSprite) override;
		virtual size_t GetIndex(StringRef name) override;
		virtual bool Remove(ISprite *pSprite) override;
		virtual bool Remove(size_t nSprite) override;

		// IResourceSave
		virtual bool LoadResource(const Dict &dict) override;
		virtual bool SaveResource(Dict &dict) override;

	private:
		ComPtr<IGraphDevice> _device;
		Vector<ComPtr<ISprite>> _sprites;
	};
}

BEGIN_INTERFACES(ff::SpriteList)
	HAS_INTERFACE(ff::ISpriteList)
	HAS_INTERFACE(ff::IGraphDeviceChild)
	HAS_INTERFACE(ff::IResourceLoad)
	HAS_INTERFACE(ff::IResourceSave)
END_INTERFACES()

static ff::ModuleStartup Register([](ff::Module &module)
{
	static ff::StaticString name(L"sprites");
	module.RegisterClassT<ff::SpriteList>(name, __uuidof(ff::ISpriteList));
});

bool ff::CreateSpriteList(IGraphDevice *pDevice, ISpriteList **ppList)
{
	return SUCCEEDED(ComAllocator<SpriteList>::CreateInstance(
		pDevice, GUID_NULL, __uuidof(ISpriteList), (void**)ppList));
}

ff::SpriteList::SpriteList()
{
}

ff::SpriteList::~SpriteList()
{
	if (_device)
	{
		_device->RemoveChild(this);
	}
}

HRESULT ff::SpriteList::_Construct(IUnknown *unkOuter)
{
	assertRetVal(_device.QueryFrom(unkOuter), E_INVALIDARG);
	_device->AddChild(this);

	return __super::_Construct(unkOuter);
}

ff::IGraphDevice *ff::SpriteList::GetDevice() const
{
	return _device;
}

bool ff::SpriteList::Reset()
{
	return true;
}

ff::ISprite *ff::SpriteList::Add(
	IGraphTexture *pTexture,
	StringRef name,
	RectFloat rect,
	PointFloat handle,
	PointFloat scale,
	SpriteType type)
{
	ComPtr<ISprite> pSprite;
	assertRetVal(CreateSprite(pTexture, name, rect, handle, scale, type, &pSprite), false);

	_sprites.Push(pSprite);

	return pSprite;
}

ff::ISprite *ff::SpriteList::Add(ISprite *pSprite)
{
	assertRetVal(pSprite, nullptr);

	ComPtr<ISprite> pNewSprite;
	assertRetVal(CreateSprite(pSprite->GetSpriteData(), &pNewSprite), false);

	_sprites.Push(pNewSprite);

	return pNewSprite;
}

bool ff::SpriteList::Add(ISpriteList *pList)
{
	assertRetVal(pList, false);

	for (size_t i = 0; i < pList->GetCount(); i++)
	{
		Add(pList->Get(i));
	}

	return true;
}

size_t ff::SpriteList::GetCount()
{
	return _sprites.Size();
}

ff::ISprite *ff::SpriteList::Get(size_t nSprite)
{
	noAssertRetVal(nSprite < _sprites.Size(), nullptr);

	return _sprites[nSprite];
}

ff::ISprite *ff::SpriteList::Get(StringRef name)
{
	size_t nIndex = GetIndex(name);

	if (nIndex == INVALID_SIZE && name.size() && isdigit(name[0]))
	{
		int nValue = 0;
		int nChars = 0;

		// convert the string to an integer
		if (_snwscanf_s(name.c_str(), name.size(), L"%d%n", &nValue, &nChars) == 1 &&
			(size_t)nChars == name.size())
		{
			return Get((size_t)nValue);
		}
	}

	return (nIndex != INVALID_SIZE) ? _sprites[nIndex] : nullptr;
}

ff::StringRef ff::SpriteList::GetName(size_t nSprite)
{
	assertRetVal(nSprite < GetCount(), ff::GetEmptyString());
	return _sprites[nSprite]->GetSpriteData()._name;
}

size_t ff::SpriteList::GetIndex(StringRef name)
{
	assertRetVal(name.size(), INVALID_SIZE);
	size_t nLen = name.size();

	for (size_t i = 0; i < _sprites.Size(); i++)
	{
		const SpriteData &data = _sprites[i]->GetSpriteData();

		if (data._name.size() == nLen && data._name == name)
		{
			return i;
		}
	}

	return INVALID_SIZE;
}

bool ff::SpriteList::Remove(ISprite *pSprite)
{
	for (size_t i = 0; i < _sprites.Size(); i++)
	{
		if (_sprites[i] == pSprite)
		{
			_sprites.Delete(i);
			return true;
		}
	}

	assertRetVal(false, false);
}

bool ff::SpriteList::Remove(size_t nSprite)
{
	assertRetVal(nSprite >= 0 && nSprite < _sprites.Size(), false);
	_sprites.Delete(nSprite);

	return true;
}

bool ff::SpriteList::LoadResource(const Dict &dict)
{
	Dict dataDict = dict.GetDict(PROP_DATA);
	size_t count = dataDict.GetSize(PROP_COUNT);
	ComPtr<IData> spritesData = dataDict.GetData(PROP_SPRITES);
	ValuePtr textureVectorValue = dataDict.GetValue(PROP_TEXTURES);
	assertRetVal(spritesData && textureVectorValue->IsType(Value::Type::ValueVector), false);

	Vector<ComPtr<IGraphTexture>> textures;
	for (Value *value : textureVectorValue->AsValueVector())
	{
		ComPtr<IGraphTexture> texture;
		assertRetVal(value->IsType(Value::Type::Object), false);
		assertRetVal(texture.QueryFrom(value->AsObject()), false);
		textures.Push(texture);
	}

	ComPtr<IDataReader> reader;
	assertRetVal(ff::CreateDataReader(spritesData, 0, &reader), false);

	for (size_t i = 0; i < count; i++)
	{
		SpriteData data;
		DWORD textureIndex;
		assertRetVal(ff::LoadData(reader, textureIndex), false);
		assertRetVal(ff::LoadData(reader, data._name), false);
		assertRetVal(ff::LoadData(reader, data._textureUV), false);
		assertRetVal(ff::LoadData(reader, data._worldRect), false);
		assertRetVal(ff::LoadData(reader, data._type), false);
		assertRetVal(textureIndex < textures.Size(), false);
		data._texture = textures[textureIndex];

		ComPtr<ISprite> sprite;
		assertRetVal(ff::CreateSprite(data, &sprite), false);
		assertRetVal(Add(sprite), false);
	}

	return true;
}

bool ff::SpriteList::SaveResource(ff::Dict &dict)
{
	// Find all unique textures
	Vector<IGraphTexture *> textures;
	for (ISprite *sprite : _sprites)
	{
		const SpriteData &spriteData = sprite->GetSpriteData();
		assertRetVal(spriteData._texture, false);

		if (textures.Find(spriteData._texture) == ff::INVALID_SIZE)
		{
			textures.Push(spriteData._texture);
		}
	}

	// Save textures to a vector of resource dicts
	ValuePtr textureVectorValue;
	{
		Vector<ValuePtr> textureVector;
		for (IGraphTexture *texture : textures)
		{
			Dict textureDict;
			assertRetVal(ff::SaveResource(texture, textureDict), false);

			ValuePtr textureValue;
			assertRetVal(ff::Value::CreateDict(std::move(textureDict), &textureValue), false);

			textureVector.Push(textureValue);
		}

		assertRetVal(ff::Value::CreateValueVector(std::move(textureVector), &textureVectorValue), false);
	}

	// Save each sprite
	ComPtr<IDataVector> spriteDataVector;
	{
		ComPtr<IDataWriter> writer;
		assertRetVal(ff::CreateDataWriter(&spriteDataVector, &writer), false);

		for (ISprite *sprite : _sprites)
		{
			const SpriteData &spriteData = sprite->GetSpriteData();
			DWORD textureIndex = (DWORD)textures.Find(spriteData._texture);
			assertRetVal(ff::SaveData(writer, textureIndex), false);
			assertRetVal(ff::SaveData(writer, spriteData._name), false);
			assertRetVal(ff::SaveData(writer, spriteData._textureUV), false);
			assertRetVal(ff::SaveData(writer, spriteData._worldRect), false);
			assertRetVal(ff::SaveData(writer, spriteData._type), false);
		}
	}

	Dict dataDict;
	dataDict.SetSize(PROP_COUNT, _sprites.Size());
	dataDict.SetData(PROP_SPRITES, spriteDataVector);
	dataDict.SetValue(PROP_TEXTURES, textureVectorValue);

	dict.SetDict(PROP_DATA, dataDict);

	return true;
}
