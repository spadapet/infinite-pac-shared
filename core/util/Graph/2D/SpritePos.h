#pragma once

namespace ff
{
	struct SpritePos;
	struct SpritePosKey;

	UTIL_API const SpritePos &GetIdentitySpritePos();
	UTIL_API const SpritePosKey &GetIdentitySpritePosKey();
	UTIL_API const DirectX::XMFLOAT3X3 &XMFloat3x3Identity();

	// Defines a transformation, and has some padding variables
	struct SpritePos
	{
		DirectX::XMFLOAT4 _color;
		float _rotate;
		float _frame; // padding
		PointFloat _scale;
		PointFloat _translate;
		float _padding; // padding
		DWORD _flags; // padding
	};

	// The same as SpritePos, but it makes use of the flags variable to keep track of
	// which parts of the transformation are valid.
	struct SpritePosKey : SpritePos
	{
		enum KeyType
		{
			KEY_NONE = 0,
			KEY_SCALE = 1,
			KEY_ROTATE = 2,
			KEY_TRANSLATE = 4,
			KEY_COLOR = 8,
			KEY_ANY = 15,

			KEY_FORCE_DWORD = 0x7fffffff,
		};

		const SpritePosKey &operator=(const SpritePos &rhs);

		KeyType GetType() const { return (KeyType)_flags; }
		bool HasType(KeyType type) const { return (_flags & type) != 0; }
		void AddType(KeyType type) { _flags |= type; }
		void ClearType() { _flags = 0; }
	};
}
