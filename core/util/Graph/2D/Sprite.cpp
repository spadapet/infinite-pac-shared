#include "pch.h"
#include "COM/ComAlloc.h"
#include "Dict/Dict.h"
#include "Graph/2D/Sprite.h"
#include "Graph/2D/SpriteList.h"
#include "Graph/GraphDevice.h"
#include "Graph/GraphTexture.h"
#include "Module/ModuleFactory.h"
#include "Resource/ResourcePersist.h"
#include "Resource/ResourceValue.h"

static ff::StaticString PROP_NAME(L"name");
static ff::StaticString PROP_SPRITES(L"sprites");

static const ff::SpriteData &GetEmptySpriteData()
{
	static const ff::SpriteData data =
	{
		ff::String(),
		nullptr,
		ff::RectFloat(0, 0, 0, 0),
		ff::RectFloat(0, 0, 0, 0),
	};

	return data;
}

ff::RectInt ff::SpriteData::GetTextureRect() const
{
	return GetTextureRectF().ToInt();
}

ff::RectFloat ff::SpriteData::GetTextureRectF() const
{
	assertRetVal(_texture, RectFloat(0, 0, 0, 0));

	PointFloat texSize = _texture->GetSize().ToFloat();
	RectFloat texRect = _textureUV * texSize;
	texRect.Normalize();

	return texRect;
}

bool ff::SpriteData::CopyTo(IGraphTexture *texture, PointInt pos) const
{
	assertRetVal(_texture && texture, false);

	ff::RectInt rect = GetTextureRect();

	D3D11_BOX box;
	box.left = rect.left;
	box.top = rect.top;
	box.right = rect.right;
	box.bottom = rect.bottom;
	box.front = 0;
	box.back = 1;

	texture->GetDevice()->GetContext()->CopySubresourceRegion(
		texture->GetTexture(), 0, pos.x, pos.y, 0,
		_texture->GetTexture(), 0, &box);

	return true;
}

class __declspec(uuid("7f942967-3605-4b36-a880-a6252d2518b9"))
	Sprite
		: public ff::ComBase
		, public ff::ISprite
{
public:
	DECLARE_HEADER(Sprite);

	void Init(const ff::SpriteData &data);

	// ISprite functions
	virtual const ff::SpriteData &GetSpriteData() override;

private:
	ff::ComPtr<ff::IGraphTexture> _texture;
	ff::SpriteData _data;
};

BEGIN_INTERFACES(Sprite)
	HAS_INTERFACE(ff::ISprite)
END_INTERFACES()

class __declspec(uuid("778af2ef-b522-453e-b040-65dc6ea6cb93"))
	SpriteResource
		: public ff::ComBase
		, public ff::ISpriteResource
		, public ff::IResourceSave
{
public:
	DECLARE_HEADER(SpriteResource);

	bool Init(ff::SharedResourceValue spriteOrListRes, ff::StringRef name);

	// ISprite
	virtual const ff::SpriteData &GetSpriteData() override;

	// ISpriteResource
	virtual ff::SharedResourceValue GetSourceResource() override;

	// IResourceSave
	virtual bool LoadResource(const ff::Dict &dict) override;
	virtual bool SaveResource(ff::Dict &dict) override;

private:
	ff::String _name;
	ff::AutoResourceValue _spriteRes;
	ff::ComPtr<ff::ISprite> _sprite;
	const ff::SpriteData *_data;
};

BEGIN_INTERFACES(SpriteResource)
	HAS_INTERFACE(ff::ISprite)
	HAS_INTERFACE(ff::ISpriteResource)
	HAS_INTERFACE(ff::IResourceLoad)
	HAS_INTERFACE(ff::IResourceSave)
END_INTERFACES()

static ff::ModuleStartup Register([](ff::Module &module)
{
	static ff::StaticString name(L"spriteInList");
	module.RegisterClassT<Sprite>(name, __uuidof(ff::ISprite));

	static ff::StaticString nameRes(L"sprite");
	module.RegisterClassT<SpriteResource>(nameRes, __uuidof(ff::ISprite));
});

bool ff::CreateSprite(const SpriteData &data, ISprite **ppSprite)
{
	assertRetVal(ppSprite && data._texture && !data._worldRect.IsEmpty(), false);

	ComPtr<Sprite, ISprite> myObj;
	assertHrRetVal(ff::ComAllocator<Sprite>::CreateInstance(&myObj), false);
	myObj->Init(data);

	*ppSprite = myObj.Detach();
	return true;
}

bool ff::CreateSprite(
	IGraphTexture *pTexture,
	StringRef name,
	RectFloat rect,
	PointFloat handle,
	PointFloat scale,
	SpriteType type,
	ISprite **ppSprite)
{
	assertRetVal(ppSprite && pTexture, false);

	PointInt sizeTex = pTexture->GetSize();
	assertRetVal(sizeTex.x && sizeTex.y, false);

	SpriteData data;
	data._name = name;
	data._texture = pTexture;
	data._type = type;

	data._textureUV.SetRect(
		rect.left / sizeTex.x,
		rect.top / sizeTex.y,
		rect.right / sizeTex.x,
		rect.bottom / sizeTex.y);

	data._worldRect.SetRect(
		-handle.x * scale.x,
		-handle.y * scale.y,
		(-handle.x + rect.Width()) * scale.x,
		(-handle.y + rect.Height()) * scale.y);

	return CreateSprite(data, ppSprite);
}

bool ff::CreateSprite(
	ISprite *pParentSprite,
	RectFloat rect,
	PointFloat handle,
	PointFloat scale,
	SpriteType type,
	ISprite **ppSprite)
{
	assertRetVal(pParentSprite && ppSprite, false);

	const SpriteData &parentData = pParentSprite->GetSpriteData();
	RectFloat parentRect = parentData.GetTextureRectF();

	rect.Offset(parentRect.TopLeft());
	assertRetVal(rect.IsInside(parentRect), false);

	return CreateSprite(parentData._texture, GetEmptyString(), rect, handle, scale, type, ppSprite);
}

bool ff::CreateSprite(IGraphTexture *pTexture, ISprite **ppSprite)
{
	assertRetVal(ppSprite && pTexture, false);

	return CreateSprite(pTexture, GetEmptyString(),
		RectInt(PointInt(0, 0), pTexture->GetSize()).ToFloat(),
		PointFloat(0, 0), // handle
		PointFloat(1, 1), // scale
		SpriteType::Unknown,
		ppSprite);
}

bool ff::CreateSpriteResource(SharedResourceValue spriteOrListRes, ISprite **obj)
{
	return ff::CreateSpriteResource(spriteOrListRes, ff::GetEmptyString(), obj);
}

bool ff::CreateSpriteResource(SharedResourceValue spriteOrListRes, StringRef name, ISprite **obj)
{
	assertRetVal(obj, false);

	ff::ComPtr<SpriteResource, ISprite> myObj;
	assertHrRetVal(ff::ComAllocator<SpriteResource>::CreateInstance(&myObj), false);
	assertRetVal(myObj->Init(spriteOrListRes, name), false);

	*obj = myObj.Detach();
	return true;
}

bool ff::CreateSpriteResource(SharedResourceValue spriteOrListRes, size_t index, ISprite **obj)
{
	String name = String::format_new(L"%lu", index);
	return CreateSpriteResource(spriteOrListRes, name, obj);
}

Sprite::Sprite()
{
}

Sprite::~Sprite()
{
}

const ff::SpriteData &Sprite::GetSpriteData()
{
	return _data;
}

void Sprite::Init(const ff::SpriteData &data)
{
	_texture = data._texture;
	_data = data;
}

SpriteResource::SpriteResource()
	: _data(nullptr)
{
}

SpriteResource::~SpriteResource()
{
}

bool SpriteResource::Init(ff::SharedResourceValue spriteOrListRes, ff::StringRef name)
{
	assertRetVal(spriteOrListRes, false);
	_spriteRes.Init(spriteOrListRes);
	_name = name;

	return true;
}

const ff::SpriteData &SpriteResource::GetSpriteData()
{
	if (_data)
	{
		return *_data;
	}

	ff::Value *value = _spriteRes.GetValue();
	if (value)
	{
		if (value->IsType(ff::Value::Type::Object))
		{
			ff::ComPtr<ff::ISprite> sprite;
			ff::ComPtr<ff::ISpriteList> sprites;

			if (sprites.QueryFrom(value->AsObject()))
			{
				_sprite = sprites->Get(_name);
			}
			else if (sprite.QueryFrom(value->AsObject()))
			{
				_sprite = sprite;
			}
		}

		if (!value->IsType(ff::Value::Type::Null))
		{
			assert(_sprite);
			if (_sprite)
			{
				// no need to cache empty sprite data, it's temporary
				const ff::SpriteData &data = _sprite->GetSpriteData();
				if (&data != &GetEmptySpriteData())
				{
					_data = &data;
				}
			}
		}
	}

	return _data ? *_data : GetEmptySpriteData();
}

ff::SharedResourceValue SpriteResource::GetSourceResource()
{
	return _spriteRes.GetResourceValue();
}

bool SpriteResource::LoadResource(const ff::Dict &dict)
{
	_spriteRes.Init(dict.GetResource(PROP_SPRITES));
	_name = dict.GetString(PROP_NAME);

	return true;
}

bool SpriteResource::SaveResource(ff::Dict &dict)
{
	dict.SetResource(PROP_SPRITES, _spriteRes.GetResourceValue());
	dict.SetString(PROP_NAME, _name);

	return true;
}
