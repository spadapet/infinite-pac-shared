#include "pch.h"
#include "Graph/2D/SpritePos.h"

__declspec(align(16)) static float s_identitySpritePos[] =
{
	1, 1, 1, 1, // color
	0, 0, // rotate, frame
	1, 1, // scale
	0, 0, // translate
	0, 0, // padding, flags
};

__declspec(align(16)) static float s_identityXMFloat3x3[] =
{
	1, 0, 0,
	0, 1, 0,
	0, 0, 1,
};

const ff::SpritePos &ff::GetIdentitySpritePos()
{
	return *(SpritePos*)s_identitySpritePos;
}

const ff::SpritePosKey &ff::GetIdentitySpritePosKey()
{
	return *(SpritePosKey*)s_identitySpritePos;
}

const DirectX::XMFLOAT3X3 &ff::XMFloat3x3Identity()
{
	return *(DirectX::XMFLOAT3X3*)s_identityXMFloat3x3;
}

const ff::SpritePosKey &ff::SpritePosKey::operator=(const SpritePos &rhs)
{
	assert(sizeof(*this) == sizeof(rhs));

	if (this != &rhs)
	{
		CopyMemory(this, &rhs, sizeof(rhs));
	}

	return *this;
}
