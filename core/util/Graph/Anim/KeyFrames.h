#pragma once

namespace ff
{
	class KeyFramesBase
	{
	public:
		virtual size_t GetKeyCount() const = 0;
		virtual size_t FindKey(float frame) const = 0;
		virtual float GetKeyFrame(size_t key) const = 0;
	};

	template<typename KeyType, typename ValueType = KeyType::ValueType>
	class KeyFrames : public KeyFramesBase
	{
	public:
		KeyFrames();
		~KeyFrames();

		void Clear();
		void Get(float frame, AnimTweenType type, ValueType &value);
		void Set(float frame, const ValueType &value);
		void SetIdentityValue(const ValueType &value);

		virtual size_t GetKeyCount() const override;
		virtual float GetKeyFrame(size_t key) const override;
		virtual size_t FindKey(float frame) const override;
		const KeyType &GetKey(size_t nKey) const;

		void SetLastFrame(float frame);
		float GetLastFrame() const;

	private:
		void UpdateKeys();

		ValueType _identityValue;
		Vector<KeyType> _keys;
		float _lastFrame;
		bool _keysChanged;
	};
}

template<typename KeyType, typename ValueType>
ff::KeyFrames<KeyType, ValueType>::KeyFrames()
	: _lastFrame(0)
	, _keysChanged(false)
{
	_identityValue = KeyType::Identity()._value;
}

template<typename KeyType, typename ValueType>
ff::KeyFrames<KeyType, ValueType>::~KeyFrames()
{
}

template<typename KeyType, typename ValueType>
void ff::KeyFrames<KeyType, ValueType>::Clear()
{
	_keys.Clear();
	_lastFrame = 0;
	_keysChanged = true;
}

template<typename KeyType, typename ValueType>
void ff::KeyFrames<KeyType, ValueType>::Get(float frame, AnimTweenType type, ValueType &value)
{
	if (_keys.Size())
	{
		UpdateKeys();

		bool bClamp = (type == POSE_TWEEN_LINEAR_CLAMP || type == POSE_TWEEN_SPLINE_CLAMP);
		bool bSpline = (type == POSE_TWEEN_SPLINE_CLAMP || type == POSE_TWEEN_SPLINE_LOOP);

		if (_lastFrame == 0)
		{
			frame = 0;
		}
		else if (bClamp)
		{
			frame = std::max<float>(frame, 0);
			frame = std::min<float>(frame, _lastFrame);
		}
		else if (frame < 0) // loop
		{
			frame = _lastFrame + fmod(frame, _lastFrame);
		}
		else if (frame >= _lastFrame) // loop
		{
			frame = fmod(frame, _lastFrame);
		}

		size_t nKey = INVALID_SIZE;
		KeyType keyFrame;
		keyFrame._frame = frame;

		if (_keys.SortFind(keyFrame, &nKey))
		{
			value = _keys[nKey]._value;
		}
		else
		{
			const KeyType &prevKey = nKey ? _keys[nKey - 1] : _keys[0];
			const KeyType &nextKey = (nKey < _keys.Size()) ? _keys[nKey] : _keys.GetLast();

			float frame1 = prevKey._frame;
			float frame2 = nextKey._frame;
			float time = (keyFrame._frame - frame1) / (frame2 - frame1);

			KeyType::Interpolate(prevKey, nextKey, time, bSpline, value);
		}
	}
	else
	{
		value = _identityValue;
	}
}

template<typename KeyType, typename ValueType>
void ff::KeyFrames<KeyType, ValueType>::Set(float frame, const ValueType &value)
{
	KeyType key = KeyType::Identity();
	key._value = value;
	key._frame = frame;

	size_t nInsert = INVALID_SIZE;

	if (_keys.SortFind(key, &nInsert))
	{
		_keys[nInsert] = key;
	}
	else
	{
		_keys.Insert(nInsert, key);
	}

	_lastFrame = std::max(_lastFrame, frame);
	_keysChanged = true;
}

template<typename KeyType, typename ValueType>
void ff::KeyFrames<KeyType, ValueType>::SetIdentityValue(const ValueType &value)
{
	_identityValue = value;
}

template<typename KeyType, typename ValueType>
size_t ff::KeyFrames<KeyType, ValueType>::GetKeyCount() const
{
	return _keys.Size();
}

template<typename KeyType, typename ValueType>
float ff::KeyFrames<KeyType, ValueType>::GetKeyFrame(size_t key) const
{
	assertRetVal(key >= 0 && key < _keys.Size(), 0);
	return _keys[key]._frame;
}

template<typename KeyType, typename ValueType>
size_t ff::KeyFrames<KeyType, ValueType>::FindKey(float frame) const
{
	KeyType keyFrame;
	keyFrame._frame = frame;

	size_t key;
	if (_keys.SortFind(keyFrame, &key))
	{
		return key;
	}

	return ff::INVALID_SIZE;
}

template<typename KeyType, typename ValueType>
const KeyType &ff::KeyFrames<KeyType, ValueType>::GetKey(size_t nKey) const
{
	assert(nKey >= 0 && nKey < _keys.Size());
	return _keys[nKey];
}

template<typename KeyType, typename ValueType>
void ff::KeyFrames<KeyType, ValueType>::SetLastFrame(float frame)
{
	_lastFrame = std::max<float>(0, frame);
	_keysChanged = true;
}

template<typename KeyType, typename ValueType>
float ff::KeyFrames<KeyType, ValueType>::GetLastFrame() const
{
	return _lastFrame;
}

template<typename KeyType, typename ValueType>
void ff::KeyFrames<KeyType, ValueType>::UpdateKeys()
{
	if (_keysChanged)
	{
		_keysChanged = false;

		if (_keys.Size())
		{
			// There MUST be an initial key and final key, or else spline interpolation will be screwed up

			if (_keys[0]._frame > 0)
			{
				KeyType key = KeyType::Identity();
				key._value = _identityValue;
				key._frame = 0;

				_keys.Insert(0, key);
			}

			KeyType &lastKey = _keys.GetLast();

			if (lastKey._frame < _lastFrame)
			{
				if (lastKey._value != _identityValue)
				{
					KeyType key = KeyType::Identity();
					key._value = _identityValue;
					key._frame = _lastFrame;

					_keys.Push(key);
				}
				else
				{
					lastKey._frame = _lastFrame;
				}
			}

			KeyType::InitTangents(_keys.Data(), _keys.Size(), 0);
		}
	}
}
