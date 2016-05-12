#include "pch.h"

static const ff::hash_t MAGIC_HASH_INIT = 0x9e3779b9;

inline static ff::hash_t CreateHashResult(DWORD b, DWORD c)
{
	static_assert(sizeof(ff::hash_t) == 8, "Hash type must be 64-bits");
	return (static_cast<ff::hash_t>(b) << 32) | static_cast<ff::hash_t>(c);
}

inline static DWORD RotateBits(DWORD val, DWORD count)
{
	return (val << count) | (val >> (32 - count));
}

inline static void HashMix(DWORD &a, DWORD &b, DWORD &c)
{
	a -= c; a ^= RotateBits(c, 4); c += b;
	b -= a; b ^= RotateBits(a, 6); a += c;
	c -= b; c ^= RotateBits(b, 8); b += a;
	a -= c; a ^= RotateBits(c, 16); c += b;
	b -= a; b ^= RotateBits(a, 19); a += c;
	c -= b; c ^= RotateBits(b, 4); b += a;
}

inline static void FinalHashMix(DWORD &a, DWORD &b, DWORD &c)
{
	c ^= b; c -= RotateBits(b, 14);
	a ^= c; a -= RotateBits(c, 11);
	b ^= a; b -= RotateBits(a, 25);
	c ^= b; c -= RotateBits(b, 16);
	a ^= c; a -= RotateBits(c, 4);
	b ^= a; b -= RotateBits(a, 14);
	c ^= b; c -= RotateBits(b, 24);
}

// By Bob Jenkins, 2006. bob_jenkins@burtleburtle.net. You may use this
// code any way you wish, private, educational, or commercial. It's free.
// See http://burtleburtle.net/bob/hash/evahash.html
// See http://burtleburtle.net/bob/c/lookup3.c

ff::hash_t ff::HashBytes(const void *data, size_t length)
{
	DWORD a = MAGIC_HASH_INIT + (DWORD)length; // + (DWORD)nInitVal;
	DWORD b = a;
	DWORD c = a; // + (DWORD)(nInitVal >> 32);

	if (((size_t)data & 0x3) == 0)
	{
		const DWORD *keyData = (const DWORD *)data;

		while (length > 12)
		{
			a += keyData[0];
			b += keyData[1];
			c += keyData[2];

			HashMix(a, b, c);

			length -= 12;
			keyData += 3;
		}

		switch(length)
		{
		case 12: c += keyData[2]; b += keyData[1]; a += keyData[0]; break;
		case 11: c += keyData[2] & 0xffffff; b += keyData[1]; a += keyData[0]; break;
		case 10: c += keyData[2] & 0xffff; b += keyData[1]; a += keyData[0]; break;
		case 9: c += keyData[2] & 0xff; b += keyData[1]; a += keyData[0]; break;
		case 8: b += keyData[1]; a += keyData[0]; break;
		case 7: b += keyData[1] & 0xffffff; a += keyData[0]; break;
		case 6: b += keyData[1] & 0xffff; a += keyData[0]; break;
		case 5: b += keyData[1] & 0xff; a += keyData[0]; break;
		case 4: a += keyData[0]; break;
		case 3: a += keyData[0] & 0xffffff; break;
		case 2: a += keyData[0] & 0xffff; break;
		case 1: a += keyData[0] & 0xff; break;
		case 0: return CreateHashResult(b, c);
		}
	}
	else if (((size_t)data & 0x1) == 0)
	{
		const WORD *keyData = (const WORD *)data;

		while (length > 12)
		{
			a += keyData[0] + (((DWORD)keyData[1]) << 16);
			b += keyData[2] + (((DWORD)keyData[3]) << 16);
			c += keyData[4] + (((DWORD)keyData[5]) << 16);

			HashMix(a, b, c);

			length -= 12;
			keyData += 6;
		}

		const BYTE *byteData = (const BYTE *)keyData;

		switch(length)
		{
		case 12:
			c += keyData[4] + (((DWORD)keyData[5]) << 16);
			b += keyData[2] + (((DWORD)keyData[3]) << 16);
			a += keyData[0] + (((DWORD)keyData[1]) << 16);
			break;

		case 11:
			c += ((DWORD)byteData[10]) << 16;
			__fallthrough;

		case 10:
			c += keyData[4];
			b += keyData[2] + (((DWORD)keyData[3]) << 16);
			a += keyData[0] + (((DWORD)keyData[1]) << 16);
			break;

		case 9:
			c += byteData[8];
			__fallthrough;

		case 8:
			b += keyData[2] + (((DWORD)keyData[3]) << 16);
			a += keyData[0] + (((DWORD)keyData[1]) << 16);
			break;

		case 7:
			b += ((DWORD)byteData[6]) << 16;
			__fallthrough;

		case 6:
			b += keyData[2];
			a += keyData[0] + (((DWORD)keyData[1]) << 16);
			break;

		case 5:
			b += byteData[4];
			__fallthrough;

		case 4:
			a += keyData[0] + (((DWORD)keyData[1]) << 16);
			break;

		case 3:
			a += ((DWORD)byteData[2]) << 16;
			__fallthrough;

		case 2:
			a += keyData[0];
			break;

		case 1:
			a += byteData[0];
			break;

		case 0:
			return CreateHashResult(b, c);
		}
	}
	else
	{
		const BYTE *keyData = (const BYTE *)data;

		while (length > 12)
		{
			a += keyData[0];
			a += ((DWORD)keyData[1]) << 8;
			a += ((DWORD)keyData[2]) << 16;
			a += ((DWORD)keyData[3]) << 24;

			b += keyData[4];
			b += ((DWORD)keyData[5]) << 8;
			b += ((DWORD)keyData[6]) << 16;
			b += ((DWORD)keyData[7]) << 24;

			c += keyData[8];
			c += ((DWORD)keyData[9]) << 8;
			c += ((DWORD)keyData[10]) << 16;
			c += ((DWORD)keyData[11]) << 24;

			HashMix(a, b, c);

			length -= 12;
			keyData += 12;
		}

		switch (length)
		{
		case 12: c += ((DWORD)keyData[11]) << 24;
		case 11: c += ((DWORD)keyData[10]) << 16;
		case 10: c += ((DWORD)keyData[9]) << 8;
		case 9: c += keyData[8];
		case 8: b += ((DWORD)keyData[7]) << 24;
		case 7: b += ((DWORD)keyData[6]) << 16;
		case 6: b += ((DWORD)keyData[5]) << 8;
		case 5: b += keyData[4];
		case 4: a += ((DWORD)keyData[3]) << 24;
		case 3: a += ((DWORD)keyData[2]) << 16;
		case 2: a += ((DWORD)keyData[1]) << 8;
		case 1: a += keyData[0];
			break;

		case 0:
			return CreateHashResult(b, c);
		}
	}

	FinalHashMix(a, b, c);

	return CreateHashResult(b, c);
}
