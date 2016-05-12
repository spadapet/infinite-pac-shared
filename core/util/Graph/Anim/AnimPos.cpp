#include "pch.h"
#include "Graph/Anim/AnimPos.h"

__declspec(align(16)) static const float s_identityAnimPos[] =
{
	0, 0, 0, 1, // rotate
	1, 1, 1, 1, // scale
	0, 0, 0, 0, // translate
	0, 0, 0, 0, // padding
};

__declspec(align(16)) static const float s_identityMatrix[] =
{
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1,
};

__declspec(align(16)) static const float s_identityMatrix3x3[] =
{
	1, 0, 0,
	0, 1, 0,
	0, 0, 1,
};

__declspec(align(16)) static const float s_colorWhite[] = { 1, 1, 1, 1 };
__declspec(align(16)) static const float s_colorBlack[] = { 0, 0, 0, 1 };
__declspec(align(16)) static const float s_colorTrans[] = { 0, 0, 0, 0 };

const ff::AnimPos &ff::GetIdentityAnimPos()
{
	return *(AnimPos*)s_identityAnimPos;
}

const ff::AnimPosKey &ff::GetIdentityAnimPosKey()
{
	return *(AnimPosKey*)s_identityAnimPos;
}

const DirectX::XMFLOAT4 &ff::GetColorWhite()
{
	return *(DirectX::XMFLOAT4 *)s_colorWhite;
}

const DirectX::XMFLOAT4 &ff::GetColorBlack()
{
	return *(DirectX::XMFLOAT4 *)s_colorBlack;
}

const DirectX::XMFLOAT4 &ff::GetColorNone()
{
	return *(DirectX::XMFLOAT4 *)s_colorTrans;
}

const DirectX::XMFLOAT4X4 &ff::GetIdentityMatrix()
{
	return *(DirectX::XMFLOAT4X4 *)s_identityMatrix;
}

const DirectX::XMFLOAT3X3 &ff::GetIdentityMatrix3x3()
{
	return *(DirectX::XMFLOAT3X3 *)s_identityMatrix3x3;
}

void ff::NormalizeWeights(float *pWeights, size_t nCount)
{
	if (pWeights && nCount)
	{
		float totalWeight = 0;

		for (size_t i = 0; i < nCount; i++)
		{
			if (pWeights[i] >= 0)
			{
				totalWeight += pWeights[i];
			}
		}

		if (totalWeight <= 0)
		{
			for (size_t i = 0; i < nCount; i++)
			{
				pWeights[i] = 0;
			}
		}
		else
		{
			for (size_t i = 0; i < nCount; i++)
			{
				if (pWeights[i] >= 0)
				{
					pWeights[i] /= totalWeight;
				}
				else
				{
					pWeights[i] = 0;
				}
			}
		}
	}
}

void ff::AnimPos::GetMatrix(DirectX::XMMATRIX &matrix) const
{
	matrix = DirectX::XMMatrixTransformation(
		DirectX::XMVectorZero(),
		DirectX::XMQuaternionIdentity(),
		DirectX::XMLoadFloat4(&_scale),
		DirectX::XMVectorZero(),
		DirectX::XMLoadFloat4(&_rotate),
		DirectX::XMLoadFloat4(&_translate));

}

const ff::AnimPosKey &ff::AnimPosKey::operator=(const AnimPos &rhs)
{
	assert(sizeof(*this) == sizeof(rhs));

	if (this != &rhs)
	{
		CopyMemory(this, &rhs, sizeof(rhs));
	}

	return *this;
}

void ff::AnimPosKey::Add(const AnimPos &pos, KeyType type, float weight)
{
	if (type)
	{
		if (weight > 0)
		{
			if ((type & AnimPosKey::KEY_SCALE) != 0)
			{
				// _scale.x += (pos._scale.x - 1) * weight;
				DirectX::XMStoreFloat4(&_scale,
					DirectX::XMVectorMultiplyAdd(
						DirectX::XMVectorSubtract(
							DirectX::XMLoadFloat4(&pos._scale),
							DirectX::XMVectorSplatOne()),
						DirectX::XMVectorReplicate(weight),
						DirectX::XMLoadFloat4(&_scale)));
			}

			if ((type & AnimPosKey::KEY_ROTATE) != 0)
			{
				DirectX::XMVECTOR temp = DirectX::XMQuaternionSlerp(
					DirectX::XMQuaternionIdentity(),
					DirectX::XMLoadFloat4(&pos._rotate),
					weight);

				// _rotate.x += temp.x * weight;
				// _rotate.w += (temp.w - 1) * weight;
				DirectX::XMStoreFloat4(&_rotate,
					DirectX::XMVectorMultiplyAdd(
						DirectX::XMVectorSubtract(temp, DirectX::XMVectorSet(0, 0, 0, 1)),
						DirectX::XMVectorReplicate(weight),
						DirectX::XMLoadFloat4(&_rotate)));
			}

			if ((type & AnimPosKey::KEY_TRANSLATE) != 0)
			{
				// _translate.x += pos._translate.x * weight;
				DirectX::XMStoreFloat4(&_translate,
					DirectX::XMVectorMultiplyAdd(
						DirectX::XMLoadFloat4(&pos._translate),
						DirectX::XMVectorReplicate(weight),
						DirectX::XMLoadFloat4(&_translate)));
			}
		}

		_flags = (AnimPosKey::KeyType)(_flags | type);
	}
}
