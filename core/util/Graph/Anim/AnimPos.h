#pragma once

namespace ff
{
	struct AnimPos;
	struct AnimPosKey;
	struct MaterialInfo;

	UTIL_API const AnimPos &GetIdentityAnimPos();
	UTIL_API const AnimPosKey &GetIdentityAnimPosKey();
	UTIL_API const DirectX::XMFLOAT4 &GetColorWhite();
	UTIL_API const DirectX::XMFLOAT4 &GetColorBlack();
	UTIL_API const DirectX::XMFLOAT4 &GetColorNone();
	UTIL_API const DirectX::XMFLOAT4X4 &GetIdentityMatrix();
	UTIL_API const DirectX::XMFLOAT3X3 &GetIdentityMatrix3x3();

	// Makes all the weights add up to one (ignoring negative weights)
	void NormalizeWeights(float *pWeights, size_t nCount);

	// Defines a transformation, and has some padding variables to keep things aligned(16)
	struct AnimPos
	{
		DirectX::XMFLOAT4 _rotate;
		DirectX::XMFLOAT4 _scale;
		DirectX::XMFLOAT4 _translate;
		float _frame; // padding
		float _padding[2];
		DWORD _flags; // padding

		UTIL_API void GetMatrix(DirectX::XMMATRIX &matrix) const;
	};

	// The same as AnimPos, but it makes use of the flags variable to keep track of
	// which parts of the transformation are valid.
	struct AnimPosKey : AnimPos
	{
		enum KeyType
		{
			KEY_NONE = 0,
			KEY_SCALE = 1,
			KEY_ROTATE = 2,
			KEY_TRANSLATE = 4,
			KEY_ANY = 7,

			KEY_FORCE_DWORD = 0x7fffffff,
		};

		const AnimPosKey &operator=(const AnimPos &rhs);
		void Add(const AnimPos &pos, KeyType type, float weight);

		KeyType GetType() const { return (KeyType)_flags; }
		bool HasType(KeyType type) const { return (_flags & type) != 0; }
		void AddType(KeyType type) { _flags |= type; }
		void ClearType() { _flags = 0; }
	};
}
