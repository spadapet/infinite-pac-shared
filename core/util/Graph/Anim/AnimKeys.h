#pragma once

namespace ff
{
	class ISprite;

	enum AnimTweenType
	{
		POSE_TWEEN_LINEAR_CLAMP,
		POSE_TWEEN_LINEAR_LOOP,
		POSE_TWEEN_SPLINE_CLAMP,
		POSE_TWEEN_SPLINE_LOOP,
	};

	struct FloatKey
	{
		float _value;
		float _tangent;
		float _frame;
		float _padding;

		typedef float ValueType;

		bool operator<(const FloatKey &rhs) const
		{
			return _frame < rhs._frame;
		}

		static const FloatKey &Identity();

		static void InitTangents(FloatKey *pKeys, size_t nKeys, float tension);
		static void Interpolate(const FloatKey &lhs, const FloatKey &rhs, float time, bool bSpline, float &output);
	};

	// Acts like DirectX::XMFLOAT4, but is exportable
	struct EXMFLOAT4
	{
		float x, y, z, w;

		const EXMFLOAT4 operator=(const DirectX::XMFLOAT4 &rhs)
		{
			CopyMemory(&x, &rhs.x, sizeof(*this));
			return *this;
		}

		operator DirectX::XMFLOAT4&() const
		{
			return *(DirectX::XMFLOAT4*)&x;
		}

		DirectX::XMFLOAT4 *operator&()
		{
			return (DirectX::XMFLOAT4*)&x;
		}

		const DirectX::XMFLOAT4 *operator&() const
		{
			return (const DirectX::XMFLOAT4*)&x;
		}
	};

	struct VectorKey
	{
		EXMFLOAT4 _value;
		EXMFLOAT4 _tangent;
		float _frame;
		float _padding[3];

		typedef DirectX::XMFLOAT4 ValueType;

		bool operator<(const VectorKey &rhs) const
		{
			return _frame < rhs._frame;
		}

		UTIL_API static const VectorKey &Identity();
		UTIL_API static const VectorKey &IdentityScale();
		UTIL_API static const VectorKey &IdentityTranslate();
		UTIL_API static const VectorKey &IdentityWhite();
		UTIL_API static const VectorKey &IdentityClear();

		UTIL_API static void InitTangents(VectorKey *pKeys, size_t nKeys, float tension);
		UTIL_API static void Interpolate(const VectorKey &lhs, const VectorKey &rhs, float time, bool bSpline, DirectX::XMFLOAT4 &output);
	};

	struct QuaternionKey
	{
		EXMFLOAT4 _value;
		EXMFLOAT4 _tangent;
		float _frame;
		float _padding[3];

		typedef DirectX::XMFLOAT4 ValueType;

		bool operator<(const QuaternionKey &rhs) const
		{
			return _frame < rhs._frame;
		}

		static const QuaternionKey &Identity();
		static void InitTangents(QuaternionKey *pKeys, size_t nKeys, float tension);
		static void Interpolate(const QuaternionKey &lhs, const QuaternionKey &rhs, float time, bool bSpline, DirectX::XMFLOAT4 &output);
	};

	struct SpriteKey
	{
		ComPtr<ISprite> _value;
		float _frame;

		typedef ComPtr<ISprite> ValueType;

		bool operator<(const SpriteKey &rhs) const
		{
			return _frame < rhs._frame;
		}

		static const SpriteKey &Identity();
		static void InitTangents(SpriteKey *pKeys, size_t nKeys, float tension);
		static void Interpolate(const SpriteKey &lhs, const SpriteKey &rhs, float time, bool bSpline, ComPtr<ISprite> &output);
	};
}
