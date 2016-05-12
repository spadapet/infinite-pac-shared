#include "pch.h"
#include "Data/DataPersist.h"
#include "Globals/ProcessGlobals.h"
#include "Graph/2D/Sprite.h"
#include "Graph/Anim/AnimKeys.h"
#include "Graph/GraphDevice.h"
#include "Graph/GraphTexture.h"

__declspec(align(16)) static float s_identityZeros[] =
{
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
};

__declspec(align(16)) static float s_identityScale[] =
{
	1, 1, 1, 1,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
};

__declspec(align(16)) static float s_identityQuat[] =
{
	0, 0, 0, 1,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
};

// static
const ff::FloatKey &ff::FloatKey::Identity()
{
	return *(FloatKey*)s_identityZeros;
}

// static
void ff::FloatKey::InitTangents(FloatKey *pKeys, size_t nKeys, float tension)
{
	tension = (1.0f - tension) / 2.0f;

	for (size_t i = 0; i < nKeys; i++)
	{
		const FloatKey &keyBefore = pKeys[i ? i - 1 : nKeys - 1];
		const FloatKey &keyAfter = pKeys[i + 1 < nKeys ? i + 1 : 0];

		pKeys[i]._tangent = tension * (keyAfter._value - keyBefore._value);
	}
}

// static
void ff::FloatKey::Interpolate(const FloatKey &lhs, const FloatKey &rhs, float time, bool bSpline, float &output)
{
	if (bSpline)
	{
		float time2 = time * time;
		float time3 = time2 * time;

		// Q(s) = (2s^3 - 3s^2 + 1)v1 + (-2s^3 + 3s^2)v2 + (s^3 - 2s^2 + s)t1 + (s^3 - s^2)t2

		output =
			(2 * time3 - 3 * time2 + 1) * lhs._value +
			(-2 * time3 + 3 * time2) * rhs._value +
			(time3 - 2 * time2 + time) * lhs._tangent +
			(time3 - time2) * rhs._tangent;
	}
	else
	{
		output = (rhs._value - lhs._value) * time + lhs._value;
	}
}

// static
const ff::VectorKey &ff::VectorKey::Identity()
{
	return *(VectorKey*)s_identityZeros;
}

// static
const ff::VectorKey &ff::VectorKey::IdentityScale()
{
	return *(VectorKey*)s_identityScale;
}

// static
const ff::VectorKey &ff::VectorKey::IdentityTranslate()
{
	return *(VectorKey*)s_identityZeros;
}

// static
const ff::VectorKey &ff::VectorKey::IdentityWhite()
{
	return *(VectorKey*)s_identityScale;
}

// static
const ff::VectorKey &ff::VectorKey::IdentityClear()
{
	return *(VectorKey*)s_identityZeros;
}

// static
void ff::VectorKey::InitTangents(VectorKey *pKeys, size_t nKeys, float tension)
{
	tension = (1.0f - tension) / 2.0f;

	for (size_t i = 0; i < nKeys; i++)
	{
		const VectorKey &keyBefore = pKeys[i ? i - 1 : nKeys - 1];
		const VectorKey &keyAfter = pKeys[i + 1 < nKeys ? i + 1 : 0];

		DirectX::XMStoreFloat4(&pKeys[i]._tangent,
			DirectX::XMVectorMultiply(
				DirectX::XMVectorReplicate(tension),
				DirectX::XMVectorSubtract(
					DirectX::XMLoadFloat4(&keyAfter._value),
					DirectX::XMLoadFloat4(&keyBefore._value))));
	}
}

// static
void ff::VectorKey::Interpolate(const VectorKey &lhs, const VectorKey &rhs, float time, bool bSpline, DirectX::XMFLOAT4 &output)
{
	if (bSpline)
	{
		DirectX::XMStoreFloat4(&output,
			DirectX::XMVectorHermite(
				DirectX::XMLoadFloat4(&lhs._value),
				DirectX::XMLoadFloat4(&lhs._tangent),
				DirectX::XMLoadFloat4(&rhs._value),
				DirectX::XMLoadFloat4(&rhs._tangent),
				time));
	}
	else
	{
		DirectX::XMStoreFloat4(&output,
			DirectX::XMVectorLerp(
				DirectX::XMLoadFloat4(&lhs._value),
				DirectX::XMLoadFloat4(&rhs._value),
				time));
	}
}

// static
const ff::QuaternionKey &ff::QuaternionKey::Identity()
{
	return *(QuaternionKey*)s_identityQuat;
}

// static
void ff::QuaternionKey::InitTangents(QuaternionKey *pKeys, size_t nKeys, float tension)
{
	for (size_t i = 0; i < nKeys; i++)
	{
		QuaternionKey &keyCur = pKeys[i];
		QuaternionKey &keyBefore = pKeys[i ? i - 1 : nKeys - 1];
		QuaternionKey &keyNext = pKeys[(i + 1) % nKeys];
		QuaternionKey &keyAfterNext = pKeys[(i + 2) % nKeys];

		DirectX::XMVECTOR keyCurTangent;
		DirectX::XMVECTOR keyNextTangent;
		DirectX::XMVECTOR keyNextValue;

		DirectX::XMQuaternionSquadSetup(
			&keyCurTangent, // A
			&keyNextTangent, // B
			&keyNextValue, // C
			DirectX::XMLoadFloat4(&keyBefore._value), // Q0
			DirectX::XMLoadFloat4(&keyCur._value), // Q1
			DirectX::XMLoadFloat4(&keyNext._value), // Q2
			DirectX::XMLoadFloat4(&keyAfterNext._value)); // Q3

		DirectX::XMStoreFloat4(&keyCur._tangent, keyCurTangent);
		DirectX::XMStoreFloat4(&keyNext._tangent, keyNextTangent);
		DirectX::XMStoreFloat4(&keyNext._value, keyNextValue);
	}
}

// static
void ff::QuaternionKey::Interpolate(const QuaternionKey &lhs, const QuaternionKey &rhs, float time, bool bSpline, DirectX::XMFLOAT4 &output)
{
	if (bSpline)
	{
		DirectX::XMStoreFloat4(&output,
			DirectX::XMQuaternionSquad(
				DirectX::XMLoadFloat4(&lhs._value),
				DirectX::XMLoadFloat4(&lhs._tangent),
				DirectX::XMLoadFloat4(&rhs._tangent),
				DirectX::XMLoadFloat4(&rhs._value),
				time));
	}
	else
	{
		DirectX::XMStoreFloat4(&output,
			DirectX::XMQuaternionSlerp(
				DirectX::XMLoadFloat4(&lhs._value),
				DirectX::XMLoadFloat4(&rhs._value),
				time));
	}
}

// static
const ff::SpriteKey &ff::SpriteKey::Identity()
{
	return *(SpriteKey*)s_identityZeros;
}

// static
void ff::SpriteKey::InitTangents(SpriteKey *pKeys, size_t nKeys, float tension)
{
}

// static
void ff::SpriteKey::Interpolate(const SpriteKey &lhs, const SpriteKey &rhs, float time, bool bSpline, ComPtr<ISprite> &output)
{
	output = lhs._value;
}
