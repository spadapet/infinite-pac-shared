#include "pch.h"
#include "COM/ComAlloc.h"
#include "Data/DataPersist.h"
#include "Dict/Dict.h"
#include "Graph/2D/2dRenderer.h"
#include "Graph/2D/Sprite.h"
#include "Graph/2D/SpriteAnimation.h"
#include "Graph/Anim/KeyFrames.h"
#include "Graph/Anim/AnimKeys.h"
#include "Graph/GraphDevice.h"
#include "Module/ModuleFactory.h"
#include "Resource/ResourcePersist.h"

static ff::StaticString PROP_FPS(L"fps");
static ff::StaticString PROP_FRAME(L"frame");
static ff::StaticString PROP_LAST_FRAME(L"lastFrame");
static ff::StaticString PROP_PARTS(L"parts");
static ff::StaticString PROP_KEYS(L"keys");

static ff::StaticString PROP_SPRITE(L"sprite");
static ff::StaticString PROP_COLOR(L"color");
static ff::StaticString PROP_OFFSET(L"offset");
static ff::StaticString PROP_SCALE(L"scale");
static ff::StaticString PROP_ROTATE(L"rotate");
static ff::StaticString PROP_HIT_BOX(L"hitbox");

namespace ff
{
	class __declspec(uuid("07643649-dd11-42d9-9e56-614844600ca5"))
		SpriteAnimation
			: public ComBase
			, public ISpriteAnimation
			, public IResourceSave
	{
	public:
		DECLARE_HEADER(SpriteAnimation);

		virtual HRESULT _Construct(IUnknown *unkOuter) override;

		// IGraphDeviceChild functions
		virtual IGraphDevice *GetDevice() const override;
		virtual bool Reset() override;

		// ISpriteAnimation
		virtual void Clear() override;
		virtual void Render(
			I2dRenderer *pRender,
			AnimTweenType type,
			float frame,
			PointFloat pos,
			const PointFloat *pScale,
			float rotate,
			const DirectX::XMFLOAT4 *pColor) override;

		virtual void SetSprite(float frame, size_t nPart, size_t nSprite, ISprite *pSprite) override;
		virtual void SetColor(float frame, size_t nPart, size_t nSprite, const DirectX::XMFLOAT4 &color) override;
		virtual void SetOffset(float frame, size_t nPart, PointFloat offset) override;
		virtual void SetScale(float frame, size_t nPart, PointFloat scale) override;
		virtual void SetRotate(float frame, size_t nPart, float rotate) override;
		virtual void SetHitBox(float frame, size_t nPart, RectFloat hitBox) override;
		virtual RectFloat GetHitBox(float frame, size_t nPart, AnimTweenType type) override;

		virtual void SetLastFrame(float frame) override;
		virtual float GetLastFrame() const override;

		virtual void SetFPS(float fps) override;
		virtual float GetFPS() const override;

		// IResourceSave
		virtual bool LoadResource(const ff::Dict &dict) override;
		virtual bool SaveResource(ff::Dict &dict) override;

	private:
		struct PartKeys
		{
			PartKeys()
			{
				_scale.SetIdentityValue(VectorKey::IdentityScale()._value);
			}

			KeyFrames<SpriteKey> _sprites[4];
			KeyFrames<VectorKey> _colors[4];
			KeyFrames<VectorKey> _offset;
			KeyFrames<VectorKey> _scale;
			KeyFrames<FloatKey> _rotate;
			KeyFrames<VectorKey> _hitBox;
		};

		void UpdateKeys();
		PartKeys &EnsurePartKeys(size_t nPart, float frame);

		ComPtr<IGraphDevice> _device;
		Vector<PartKeys *> _parts;
		float _lastFrame;
		float _fps;
		bool _keysChanged;
	};
}

BEGIN_INTERFACES(ff::SpriteAnimation)
	HAS_INTERFACE(ff::ISpriteAnimation)
	HAS_INTERFACE(ff::IGraphDeviceChild)
	HAS_INTERFACE(ff::IResourceLoad)
	HAS_INTERFACE(ff::IResourceSave)
END_INTERFACES()

static ff::ModuleStartup Register([](ff::Module &module)
{
	static ff::StaticString name(L"spriteanim");
	module.RegisterClassT<ff::SpriteAnimation>(name, __uuidof(ff::ISpriteAnimation));
});

bool ff::CreateSpriteAnimation(IGraphDevice *pDevice, ISpriteAnimation **ppAnim)
{
	return SUCCEEDED(ComAllocator<SpriteAnimation>::CreateInstance(
		pDevice, GUID_NULL, __uuidof(ISpriteAnimation), (void**)ppAnim));
}

ff::SpriteAnimation::SpriteAnimation()
	: _lastFrame(0)
	, _fps(1)
	, _keysChanged(false)
{
}

ff::SpriteAnimation::~SpriteAnimation()
{
	Clear();

	if (_device)
	{
		_device->RemoveChild(this);
	}
}

HRESULT ff::SpriteAnimation::_Construct(IUnknown *unkOuter)
{
	assertRetVal(_device.QueryFrom(unkOuter), E_INVALIDARG);
	_device->AddChild(this);

	return __super::_Construct(unkOuter);
}

ff::IGraphDevice *ff::SpriteAnimation::GetDevice() const
{
	return _device;
}

bool ff::SpriteAnimation::Reset()
{
	return true;
}

void ff::SpriteAnimation::Clear()
{
	for (size_t i = 0; i < _parts.Size(); i++)
	{
		delete _parts[i];
	}

	_parts.Clear();
}

void ff::SpriteAnimation::Render(
		I2dRenderer *pRender,
		AnimTweenType type,
		float frame,
		PointFloat pos,
		const PointFloat *pScale,
		float rotate,
		const DirectX::XMFLOAT4 *pColor)
{
	UpdateKeys();

	for (size_t i = 0; i < _parts.Size(); i++)
	{
		if (!_parts[i])
		{
			continue;
		}

		PartKeys &part = *_parts[i];
		ISprite *pSprites[4] = { nullptr, nullptr, nullptr, nullptr };
		DirectX::XMFLOAT4 realColors[4];
		size_t nSprites = 0;

		// Sneaky way to avoid constructor/destructor
		// (don't want them to show up in profiles anymore, this function is called A LOT)
		ComPtr<ISprite> *pSpritePtrs = (ComPtr<ISprite> *)pSprites;

		for (; nSprites < _countof(part._sprites); nSprites++)
		{
			part._sprites[nSprites].Get(frame, type, pSpritePtrs[nSprites]);

			if (!pSprites[nSprites])
			{
				break;
			}
		}

		for (size_t h = 0; h < nSprites; h++)
		{
			part._colors[h].Get(frame, type, realColors[h]);
		}

		if (nSprites)
		{
			DirectX::XMFLOAT4 vectorPos;
			DirectX::XMFLOAT4 vectorScale;
			float realRotate;

			part._offset.Get(frame, type, vectorPos);
			part._scale.Get(frame, type, vectorScale);
			part._rotate.Get(frame, type, realRotate);

			realRotate += rotate;

			DirectX::XMStoreFloat4(&vectorPos,
				DirectX::XMVectorAdd(
					DirectX::XMLoadFloat4(&vectorPos),
					DirectX::XMVectorSet(pos.x, pos.y, 0, 0)));

			if (pScale)
			{
				DirectX::XMStoreFloat4(&vectorScale,
					DirectX::XMVectorMultiply(
						DirectX::XMLoadFloat4(&vectorScale),
						DirectX::XMVectorSet(pScale->x, pScale->y, 1, 1)));
			}

			if (pColor)
			{
				for (size_t h = 0; h < nSprites; h++)
				{
					DirectX::XMStoreFloat4(&realColors[h],
						DirectX::XMColorModulate(
							DirectX::XMLoadFloat4(&realColors[h]),
							DirectX::XMLoadFloat4(pColor)));
				}
			}

			PointFloat realPos(vectorPos.x, vectorPos.y);
			PointFloat realScale(vectorScale.x, vectorScale.y);

			if (nSprites == 1)
			{
				pRender->DrawSprite(pSprites[0], &realPos, &realScale, realRotate, &realColors[0]);
				pSprites[0]->Release();
			}
			else
			{
				pRender->DrawMultiSprite(pSprites, nSprites, &realPos, &realScale, realRotate, &realColors[0], nSprites);

				for (size_t h = 0; h < nSprites; h++)
				{
					pSprites[h]->Release();
				}
			}
		}
	}
}

void ff::SpriteAnimation::SetSprite(float frame, size_t nPart, size_t nSprite, ISprite *pSprite)
{
	assertRet(nSprite >= 0 && nSprite < 4);

	PartKeys &keys = EnsurePartKeys(nPart, frame);

	keys._sprites[nSprite].Set(frame, pSprite);
}

void ff::SpriteAnimation::SetColor(float frame, size_t nPart, size_t nSprite, const DirectX::XMFLOAT4 &color)
{
	assertRet(nSprite >= 0 && nSprite < 4);

	PartKeys &keys = EnsurePartKeys(nPart, frame);

	keys._colors[nSprite].Set(frame, color);
}

void ff::SpriteAnimation::SetOffset(float frame, size_t nPart, PointFloat offset)
{
	PartKeys &keys = EnsurePartKeys(nPart, frame);

	keys._offset.Set(frame, DirectX::XMFLOAT4(offset.x, offset.y, 0, 0));
}

void ff::SpriteAnimation::SetScale(float frame, size_t nPart, PointFloat scale)
{
	PartKeys &keys = EnsurePartKeys(nPart, frame);

	keys._scale.Set(frame, DirectX::XMFLOAT4(scale.x, scale.y, 0, 0));
}

void ff::SpriteAnimation::SetRotate(float frame, size_t nPart, float rotate)
{
	PartKeys &keys = EnsurePartKeys(nPart, frame);

	keys._rotate.Set(frame, rotate);
}

void ff::SpriteAnimation::SetHitBox(float frame, size_t nPart, RectFloat hitBox)
{
	PartKeys &keys = EnsurePartKeys(nPart, frame);

	keys._hitBox.Set(frame, *(const DirectX::XMFLOAT4*)&hitBox);
}

ff::RectFloat ff::SpriteAnimation::GetHitBox(float frame, size_t nPart, AnimTweenType type)
{
	UpdateKeys();

	RectFloat hitBox(0, 0, 0, 0);
	assertRetVal(nPart >= 0 && nPart < _parts.Size(), hitBox);

	_parts[nPart]->_hitBox.Get(frame, type, *(DirectX::XMFLOAT4*)&hitBox);

	return hitBox;
}

void ff::SpriteAnimation::SetLastFrame(float frame)
{
	_lastFrame = std::max<float>(0, frame);
	_keysChanged = true;
}

float ff::SpriteAnimation::GetLastFrame() const
{
	return _lastFrame;
}

void ff::SpriteAnimation::SetFPS(float fps)
{
	_fps = std::abs(fps);
}

float ff::SpriteAnimation::GetFPS() const
{
	return _fps;
}

void ff::SpriteAnimation::UpdateKeys()
{
	if (_keysChanged)
	{
		_keysChanged = false;

		for (size_t i = 0; i < _parts.Size(); i++)
		{
			if (_parts[i])
			{
				PartKeys &part = *_parts[i];

				part._sprites[0].SetLastFrame(_lastFrame);
				part._sprites[1].SetLastFrame(_lastFrame);
				part._sprites[2].SetLastFrame(_lastFrame);
				part._sprites[3].SetLastFrame(_lastFrame);

				part._colors[0].SetLastFrame(_lastFrame);
				part._colors[1].SetLastFrame(_lastFrame);
				part._colors[2].SetLastFrame(_lastFrame);
				part._colors[3].SetLastFrame(_lastFrame);

				part._offset.SetLastFrame(_lastFrame);
				part._scale.SetLastFrame(_lastFrame);
				part._rotate.SetLastFrame(_lastFrame);
				part._hitBox.SetLastFrame(_lastFrame);
			}
		}
	}
}

ff::SpriteAnimation::PartKeys &ff::SpriteAnimation::EnsurePartKeys(size_t nPart, float frame)
{
	while (nPart >= _parts.Size())
	{
		_parts.Push(nullptr);
	}

	if (!_parts[nPart])
	{
		_parts[nPart] = new PartKeys();

		_parts[nPart]->_scale.SetIdentityValue(VectorKey::IdentityScale()._value);

		_parts[nPart]->_colors[0].SetIdentityValue(VectorKey::IdentityWhite()._value);
		_parts[nPart]->_colors[1].SetIdentityValue(VectorKey::IdentityWhite()._value);
		_parts[nPart]->_colors[2].SetIdentityValue(VectorKey::IdentityWhite()._value);
		_parts[nPart]->_colors[3].SetIdentityValue(VectorKey::IdentityWhite()._value);
	}

	_lastFrame = std::max(_lastFrame, frame);
	_keysChanged = true;

	return *_parts[nPart];
}

bool ff::SpriteAnimation::LoadResource(const Dict &dict)
{
	ValuePtr partsValue = dict.GetValue(PROP_PARTS);
	if (partsValue && partsValue->IsType(Value::Type::ValueVector))
	{
		for (size_t i = 0; i < partsValue->AsValueVector().Size(); i++)
		{
			Value *partValue = partsValue->AsValueVector().GetAt(i);

			ValuePtr partDictValue;
			if (!partValue->Convert(Value::Type::Dict, &partDictValue))
			{
				continue;
			}

			const Dict &partDict = partDictValue->AsDict();
			Value *keysValue = partDict.GetValue(PROP_KEYS);
			if (!keysValue || !keysValue->IsType(Value::Type::ValueVector))
			{
				continue;
			}

			for (size_t h = 0; h < keysValue->AsValueVector().Size(); h++)
			{
				Value *keyValue = keysValue->AsValueVector().GetAt(h);

				ValuePtr keyDictValue;
				if (!keyValue->Convert(Value::Type::Dict, &keyDictValue))
				{
					continue;
				}

				const Dict &keyDict = keyDictValue->AsDict();
				float frame = keyDict.GetFloat(PROP_FRAME);
				Value *spriteValue = keyDict.GetValue(PROP_SPRITE);
				Value *colorValue = keyDict.GetValue(PROP_COLOR);
				Value *offsetValue = keyDict.GetValue(PROP_OFFSET);
				Value *scaleValue = keyDict.GetValue(PROP_SCALE);
				Value *rotateValue = keyDict.GetValue(PROP_ROTATE);
				Value *hitboxValue = keyDict.GetValue(PROP_HIT_BOX);

				if (spriteValue && spriteValue->IsType(Value::Type::ValueVector))
				{
					for (size_t j = 0; j < 4 && j < spriteValue->AsValueVector().Size(); j++)
					{
						ValuePtr resVal;
						ComPtr<ISprite> sprite;
						if (spriteValue->AsValueVector().GetAt(j)->Convert(Value::Type::Resource, &resVal) &&
							ff::CreateSpriteResource(resVal->AsResource(), &sprite))
						{
							SetSprite(frame, i, j, sprite);
						}
					}
				}

				if (colorValue && colorValue->IsType(Value::Type::ValueVector))
				{
					for (size_t j = 0; j < 4 && j < colorValue->AsValueVector().Size(); j++)
					{
						ValuePtr rectVal;
						if (colorValue->AsValueVector().GetAt(j)->Convert(Value::Type::RectF, &rectVal))
						{
							SetColor(frame, i, j, *(const DirectX::XMFLOAT4 *)&rectVal->AsRectF());
						}
					}
				}

				ValuePtr offsetValue2;
				if (offsetValue && offsetValue->Convert(Value::Type::PointF, &offsetValue2))
				{
					SetOffset(frame, i, offsetValue2->AsPointF());
				}

				ValuePtr scaleValue2;
				if (scaleValue && scaleValue->Convert(Value::Type::PointF, &scaleValue2))
				{
					SetScale(frame, i, scaleValue2->AsPointF());
				}

				ValuePtr rotateValue2;
				if (rotateValue && rotateValue->Convert(Value::Type::Float, &rotateValue2))
				{
					SetRotate(frame, i, rotateValue2->AsFloat() * ff::DEG_TO_RAD_F);
				}

				ValuePtr hitboxValue2;
				if (hitboxValue && hitboxValue->Convert(Value::Type::RectF, &hitboxValue2))
				{
					SetHitBox(frame, i, hitboxValue2->AsRectF());
				}
			}
		}
	}

	_lastFrame = dict.GetFloat(PROP_LAST_FRAME);
	_fps = dict.GetFloat(PROP_FPS, 1);
	_keysChanged = true;

	return true;
}

bool ff::SpriteAnimation::SaveResource(Dict &dict)
{
	dict.SetFloat(PROP_LAST_FRAME, _lastFrame);
	dict.SetFloat(PROP_FPS, _fps);

	Vector<ValuePtr> parts;
	for (const PartKeys *part : _parts)
	{
		if (!part)
		{
			ValuePtr nullValue;
			Value::CreateNull(&nullValue);
			parts.Push(nullValue);
			continue;
		}

		enum class KeyType
		{
			Sprite,
			Color,
			Point,
			Vector,
			Radians,
		};

		struct KeyInfo
		{
			KeyType type;
			String name;
			const KeyFramesBase &keys;
		};

		std::array<KeyInfo, 6> infos =
		{
			KeyInfo{ KeyType::Sprite, PROP_SPRITE, part->_sprites[0] },
			KeyInfo{ KeyType::Color, PROP_COLOR, part->_colors[0] },
			KeyInfo{ KeyType::Point, PROP_OFFSET, part->_offset },
			KeyInfo{ KeyType::Point, PROP_SCALE, part->_scale },
			KeyInfo{ KeyType::Radians, PROP_ROTATE, part->_rotate },
			KeyInfo{ KeyType::Vector, PROP_HIT_BOX, part->_hitBox },
		};

		Set<float> keyFrames;
		for (const KeyInfo &info : infos)
		{
			for (size_t i = 0; i < info.keys.GetKeyCount(); i++)
			{
				keyFrames.SetKey(info.keys.GetKeyFrame(i));
			}
		}

		Vector<ValuePtr> keys;
		for (float frame : keyFrames)
		{
			Dict keyDict;
			keyDict.SetFloat(PROP_FRAME, frame);

			for (const KeyInfo &info : infos)
			{
				size_t keyIndex = info.keys.FindKey(frame);
				if (keyIndex == INVALID_SIZE)
				{
					// ignore
				}
				else if (info.type == KeyType::Sprite)
				{
					const KeyFrames<SpriteKey> *keys = (const KeyFrames<SpriteKey> *)&info.keys;
					Vector<ValuePtr> spriteVec;

					for (size_t i = 0; i < 4; i++)
					{
						keyIndex = keys[i].FindKey(frame);
						if (keyIndex != INVALID_SIZE)
						{
							const SpriteKey &spriteKey = keys[i].GetKey(keyIndex);

							ComPtr<ISpriteResource> res;
							assertRetVal(res.QueryFrom(spriteKey._value), false);

							ValuePtr resValue;
							Value::CreateResource(res->GetSourceResource(), &resValue);
							spriteVec.Push(resValue);
						}
					}

					ValuePtr spriteValue;
					Value::CreateValueVector(std::move(spriteVec), &spriteValue);
					keyDict.SetValue(info.name, spriteValue);
				}
				else if (info.type == KeyType::Color)
				{
					const KeyFrames<VectorKey> *keys = (const KeyFrames<VectorKey> *)&info.keys;
					Vector<ValuePtr> colorVec;

					for (size_t i = 0; i < 4; i++)
					{
						keyIndex = keys[i].FindKey(frame);
						if (keyIndex != INVALID_SIZE)
						{
							const VectorKey &vecKey = keys[i].GetKey(keyIndex);

							Vector<float> vec;
							vec.Push(vecKey._value.x);
							vec.Push(vecKey._value.y);
							vec.Push(vecKey._value.z);
							vec.Push(vecKey._value.w);

							ValuePtr vecValue;
							Value::CreateFloatVector(std::move(vec), &vecValue);
							colorVec.Push(vecValue);
						}
					}

					ValuePtr colorValue;
					Value::CreateValueVector(std::move(colorVec), &colorValue);
					keyDict.SetValue(info.name, colorValue);
				}
				else if (info.type == KeyType::Point)
				{
					const KeyFrames<VectorKey> &keys = (const KeyFrames<VectorKey> &)info.keys;
					const VectorKey &vecKey = keys.GetKey(keyIndex);
					keyDict.SetPointF(info.name, PointFloat(vecKey._value.x, vecKey._value.y));
				}
				else if (info.type == KeyType::Vector)
				{
					const KeyFrames<VectorKey> &keys = (const KeyFrames<VectorKey> &)info.keys;
					const VectorKey &vecKey = keys.GetKey(keyIndex);
					keyDict.SetRectF(info.name, RectFloat(vecKey._value.x, vecKey._value.y, vecKey._value.z, vecKey._value.w));
				}
				else if (info.type == KeyType::Radians)
				{
					const KeyFrames<FloatKey> &keys = (const KeyFrames<FloatKey> &)info.keys;
					const FloatKey &floatKey = keys.GetKey(keyIndex);
					keyDict.SetFloat(info.name, floatKey._value * ff::RAD_TO_DEG_F);
				}
			}

			ValuePtr keyDictValue;
			Value::CreateDict(std::move(keyDict), &keyDictValue);
			keys.Push(keyDictValue);
		}

		Dict partDict;

		ValuePtr keysValue;
		Value::CreateValueVector(std::move(keys), &keysValue);
		partDict.SetValue(PROP_KEYS, keysValue);

		ValuePtr partDictValue;
		Value::CreateDict(std::move(partDict), &partDictValue);
		parts.Push(partDictValue);
	}

	ValuePtr partsValue;
	Value::CreateValueVector(std::move(parts), &partsValue);
	dict.SetValue(PROP_PARTS, partsValue);

	return true;
}
