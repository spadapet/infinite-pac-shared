#include "pch.h"
#include "Graph/2D/2dEffect.h"
#include "Graph/2D/2dRenderer.h"
#include "Graph/2D/Sprite.h"
#include "Graph/2D/SpriteList.h"
#include "Graph/2D/SpriteOptimizer.h"
#include "Graph/GraphDevice.h"
#include "Graph/GraphTexture.h"
#include "Graph/RenderTarget/RenderTarget.h"

#include <DirectXTex.h>

// STATIC_DATA (pod)
static const int s_textureSizeMax = 1024;
static const int s_textureSizeMin = 128;
static const int s_gridSize = 8;

struct OptimizedSpriteInfo
{
	OptimizedSpriteInfo();
	OptimizedSpriteInfo(ff::ISprite *pSprite, size_t nIndex);

	ff::ComPtr<ff::ISprite> _sprite;
	ff::RectInt _srcRect;
	ff::PointFloat _srcHandle;
	ff::PointFloat _srcScale;
	size_t _destTexture;
	ff::RectInt _destRect;
	size_t _originalIndex;
};

OptimizedSpriteInfo::OptimizedSpriteInfo()
{
	ff::ZeroObject(*this);
}

OptimizedSpriteInfo::OptimizedSpriteInfo(ff::ISprite *pSprite, size_t nIndex)
	: _sprite(pSprite)
	, _destTexture(ff::INVALID_SIZE)
	, _destRect(0, 0, 0, 0)
	, _originalIndex(nIndex)
{
	const ff::SpriteData &data = pSprite->GetSpriteData();

	_srcRect = data.GetTextureRect();
	_srcScale.SetPoint(data._worldRect.Width() / _srcRect.Width(), data._worldRect.Height() / _srcRect.Height());
	_srcHandle.SetPoint(-data._worldRect.left / _srcScale.x, -data._worldRect.top / _srcScale.y);
}

struct OptimizedTextureInfo
{
	OptimizedTextureInfo();
	OptimizedTextureInfo(ff::PointInt size);

	ff::RectInt FindPlacement(ff::PointInt size);
	bool PlaceRect(ff::RectInt rect);

	ff::PointInt _size;

private:
	BYTE _rowLeft[s_textureSizeMax / s_gridSize];
	BYTE _rowRight[s_textureSizeMax / s_gridSize];
};

OptimizedTextureInfo::OptimizedTextureInfo()
{
	ff::ZeroObject(*this);
}

OptimizedTextureInfo::OptimizedTextureInfo(ff::PointInt size)
{
	// Column indexes must fit within a byte (even one beyond the last column)
	assert(s_textureSizeMax / s_gridSize < 256 && _countof(_rowLeft) == _countof(_rowRight));

	ff::ZeroObject(*this);
	_size = size;
}

ff::RectInt OptimizedTextureInfo::FindPlacement(ff::PointInt size)
{
	ff::PointInt destPos(-1, -1);

	if (size.x > 0 && size.x <= _size.x &&
		size.y > 0 && size.y <= _size.y)
	{
		ff::PointInt cellSize(
			(size.x + s_gridSize - 1) / s_gridSize,
			(size.y + s_gridSize - 1) / s_gridSize);

		for (int y = 0; y + cellSize.y <= _size.y / s_gridSize; y++)
		{
			for (int attempt = 0; attempt < 2; attempt++)
			{
				// Try to put the sprite as far left as possible in each row
				int x;
				
				if (attempt)
				{
					x = _rowRight[y];

					if (x)
					{
						// don't touch the previous sprite
						x++;
					}
				}
				else
				{
					x = _rowLeft[y];
					x -= cellSize.x + 1;
				}

				if (x >= 0 && x + cellSize.x <= _size.x / s_gridSize)
				{
					bool found = true;

					// Look for intersection with another sprite
					for (int checkY = y + cellSize.y; checkY >= y - 1; checkY--)
					{
						if (checkY >= 0 &&
							checkY < _size.y / s_gridSize &&
							checkY < _countof(_rowRight) &&
							_rowRight[checkY] &&
							_rowRight[checkY] + 1 > x &&
							x + cellSize.x + 1 > _rowLeft[checkY])
						{
							found = false;
							break;
						}
					}

					// Prefer positions further to the left
					if (found && (destPos.x == -1 || destPos.x > x * s_gridSize))
					{
						destPos.SetPoint(x * s_gridSize, y * s_gridSize);
					}
				}
			}
		}
	}

	return (destPos.x == -1)
		? ff::RectInt(0, 0, 0, 0)
		: ff::RectInt(destPos.x, destPos.y, destPos.x + size.x, destPos.y + size.y);
}

bool OptimizedTextureInfo::PlaceRect(ff::RectInt rect)
{
	ff::RectInt rectCells(
		rect.left / s_gridSize,
		rect.top / s_gridSize,
		(rect.right + s_gridSize - 1) / s_gridSize,
		(rect.bottom + s_gridSize - 1) / s_gridSize);

	assertRetVal(
		rect.left >= 0 && rect.right <= _size.x &&
		rect.top >= 0 && rect.bottom <= _size.y &&
		rect.Width() > 0 &&
		rect.Height() > 0, false);

	// Validate that the sprite doesn't overlap anything

	for (int y = rectCells.top - 1; y <= rectCells.bottom; y++)
	{
		if (y >= 0 && y < _size.y / s_gridSize && y < _countof(_rowRight))
		{
			// Must be a one cell gap between sprites
			assertRetVal(!_rowRight[y] ||
				_rowRight[y] + 1 <= rectCells.left ||
				rectCells.right + 1 <= _rowLeft[y], false);
		}
	}

	// Invalidate the space taken up by the new sprite

	for (int y = rectCells.top; y < rectCells.bottom; y++)
	{
		if (y >= 0 && y < _size.y / s_gridSize && y < _countof(_rowRight))
		{
			if (_rowRight[y])
			{
				_rowLeft[y] = std::min<BYTE>(_rowLeft[y], rectCells.left);
				_rowRight[y] = std::max<BYTE>(_rowRight[y], rectCells.right);
			}
			else
			{
				_rowLeft[y] = rectCells.left;
				_rowRight[y] = rectCells.right;
			}
		}
	}

	return true;
}

static int CompareSpriteInfo(const OptimizedSpriteInfo &sprite1, const OptimizedSpriteInfo &sprite2)
{
	const ff::SpriteData &data1 = sprite1._sprite->GetSpriteData();
	const ff::SpriteData &data2 = sprite2._sprite->GetSpriteData();

	if (sprite1._srcRect.Height() == sprite2._srcRect.Height())
	{
		if (sprite1._srcRect.Width() == sprite2._srcRect.Width())
		{
			if (sprite1._srcRect.top == sprite2._srcRect.top)
			{
				if (sprite1._srcRect.left == sprite2._srcRect.left)
				{
					if (data1._texture == data2._texture)
					{
						return 0;
					}
					else if (data1._texture < data2._texture)
					{
						return -1;
					}
				}
				else if (sprite1._srcRect.left < sprite2._srcRect.left)
				{
					return -1;
				}
			}
			else if (sprite1._srcRect.top < sprite2._srcRect.top)
			{
				return -1;
			}
		}
		else if (sprite1._srcRect.Width() > sprite2._srcRect.Width())
		{
			// larger comes first
			return -1;
		}
	}
	else if (sprite1._srcRect.Height() > sprite2._srcRect.Height())
	{
		// larger comes first
		return -1;
	}

	return 1;
}

static int CompareSpriteInfo(const void *p1, const void *p2)
{
	const OptimizedSpriteInfo *pInfo1 = (const OptimizedSpriteInfo*)p1;
	const OptimizedSpriteInfo *pInfo2 = (const OptimizedSpriteInfo*)p2;

	return CompareSpriteInfo(*pInfo1, *pInfo2);
}

static int CompareSpriteInfoIndex(const void *p1, const void *p2)
{
	const OptimizedSpriteInfo *pInfo1 = (const OptimizedSpriteInfo*)p1;
	const OptimizedSpriteInfo *pInfo2 = (const OptimizedSpriteInfo*)p2;

	if (pInfo1->_originalIndex < pInfo2->_originalIndex)
	{
		return -1;
	}
	else if (pInfo1->_originalIndex > pInfo2->_originalIndex)
	{
		return 1;
	}

	return 0;
}

// Returns true when all done (sprites will still be placed when false is returned)
static bool PlaceSprites(
	ff::Vector<OptimizedSpriteInfo> &sprites,
	ff::Vector<OptimizedTextureInfo> &textureInfos,
	size_t nStartTexture)
{
	size_t nSpritesDone = 0;

	for (size_t i = 0; i < sprites.Size(); i++)
	{
		OptimizedSpriteInfo &sprite = sprites[i];
		bool bReusePrevious = (i > 0) && !CompareSpriteInfo(sprite, sprites[i - 1]);

		if (bReusePrevious)
		{
			sprite._destTexture = sprites[i - 1]._destTexture;
			sprite._destRect = sprites[i - 1]._destRect;
		}
		else
		{
			if (sprite._destTexture == ff::INVALID_SIZE)
			{
				if (sprite._srcRect.Width() > s_textureSizeMax ||
					sprite._srcRect.Height() > s_textureSizeMax)
				{
					ff::PointInt size = sprite._srcRect.Size();

					// Oversized, so make a new unshared texture
					sprite._destTexture = textureInfos.Size();
					sprite._destRect.SetRect(ff::PointInt(0, 0), size);

					// texture sizes should be powers of 2 to support compression and mipmaps
					size.x = (int)ff::NearestPowerOfTwo((size_t)size.x);
					size.y = (int)ff::NearestPowerOfTwo((size_t)size.y);

					OptimizedTextureInfo newTexture(size);
					textureInfos.Push(newTexture);
				}
			}

			if (sprite._destTexture == ff::INVALID_SIZE)
			{
				// Look for empty space in an existing texture
				for (size_t h = nStartTexture; h < textureInfos.Size(); h++)
				{
					OptimizedTextureInfo &texture = textureInfos[h];

					if (texture._size.x <= s_textureSizeMax &&
						texture._size.y <= s_textureSizeMax)
					{
						sprite._destRect = texture.FindPlacement(sprite._srcRect.Size());

						if (!sprite._destRect.IsNull())
						{
							verify(texture.PlaceRect(sprite._destRect));
							sprite._destTexture = h;

							break;
						}
					}
				}
			}
		}

		if (sprite._destTexture != ff::INVALID_SIZE)
		{
			nSpritesDone++;
		}
	}

	return nSpritesDone == sprites.Size();
}

static bool ComputeOptimizedSprites(
	ff::Vector<OptimizedSpriteInfo> &sprites,
	ff::Vector<OptimizedTextureInfo> &textureInfos)
{
	for (bool bDone = false; !bDone && sprites.Size(); )
	{
		// Add a new texture, start with the smallest size and work up
		for (ff::PointInt size(s_textureSizeMin, s_textureSizeMin); !bDone && size.x <= s_textureSizeMax; size *= 2)
		{
			size_t nTextureInfo = textureInfos.Size();
			textureInfos.Push(OptimizedTextureInfo(size));

			bDone = PlaceSprites(sprites, textureInfos, textureInfos.Size() - 1);

			if (!bDone && size.x < s_textureSizeMax)
			{
				// Remove this texture and use a bigger one instead
				textureInfos.Delete(nTextureInfo);

				for (size_t i = 0; i < sprites.Size(); i++)
				{
					OptimizedSpriteInfo &sprite = sprites[i];

					if (sprite._destTexture != ff::INVALID_SIZE)
					{
						if (sprite._destTexture == nTextureInfo)
						{
							sprite._destTexture = ff::INVALID_SIZE;
							sprite._destRect.SetRect(0, 0, 0, 0);
						}
						else if (sprite._destTexture > nTextureInfo)
						{
							sprite._destTexture--;
						}
					}
				}
			}
		}
	}

	return true;
}

static bool CreateOptimizedTextures(
	DXGI_FORMAT format,
	ff::IGraphDevice *pDevice,
	ff::Vector<OptimizedTextureInfo> &textureInfos,
	ff::Vector<ff::ComPtr<ff::IGraphTexture>> &textures)
{
	for (size_t i = 0; i < textureInfos.Size(); i++)
	{
		OptimizedTextureInfo &texture = textureInfos[i];
		DXGI_FORMAT actualFormat = (format == DXGI_FORMAT_A8_UNORM) ? format : DXGI_FORMAT_R8G8B8A8_UNORM;

		ff::ComPtr<ff::IGraphTexture> pNewTexture;
		assertRetVal(CreateGraphTexture(pDevice, texture._size, actualFormat, 1, 1, 0, &pNewTexture), false);

		ff::ComPtr<ff::IRenderTarget> pRenderTarget;
		assertRetVal(ff::CreateRenderTargetTexture(pDevice, pNewTexture, 0, 1, 0, &pRenderTarget), false);
		pRenderTarget->Clear(&DirectX::XMFLOAT4(0, 0, 0, 0));

		textures.Push(pNewTexture);
	}

	return true;
}

static bool CopyOptimizedSprites(ff::Vector<OptimizedSpriteInfo> &spriteInfos, ff::Vector<ff::ComPtr<ff::IGraphTexture>> &textures)
{
	for (size_t i = 0; i < spriteInfos.Size(); i++)
	{
		OptimizedSpriteInfo &sprite = spriteInfos[i];
		const ff::SpriteData &data = sprite._sprite->GetSpriteData();
		assertRetVal(sprite._destTexture >= 0 && sprite._destTexture < textures.Size(), false);

		ff::IGraphTexture *pTexture = textures[sprite._destTexture];

		sprite._sprite->GetSpriteData().CopyTo(pTexture, sprite._destRect.TopLeft());
	}

	return true;
}

static bool ConvertOptimizedTextures(
	DXGI_FORMAT format,
	size_t mipMapLevels,
	ff::Vector<ff::ComPtr<ff::IGraphTexture>> &textures,
	ff::Vector<ff::ComPtr<ff::IGraphTexture>> &finalTextures,
	ff::Vector<std::shared_ptr<DirectX::ScratchImage>> &alphaTextures)
{
	for (size_t i = 0; i < textures.Size(); i++)
	{
		ff::IGraphTexture *texture = textures[i];
		ff::IGraphDevice *device = texture->GetDevice();

		ff::ComPtr<ff::IGraphTexture> newTexture;
		assertRetVal(texture->Convert(format, mipMapLevels, &newTexture), false);
		finalTextures.Push(newTexture);

		DirectX::ScratchImage scratch;
		assertHrRetVal(DirectX::CaptureTexture(device->Get3d(), device->GetContext(), texture->GetTexture(), scratch), false);

		std::shared_ptr<DirectX::ScratchImage> alphaScratch = std::make_shared<DirectX::ScratchImage>();
		assertHrRetVal(DirectX::Convert(scratch.GetImages(), 1, scratch.GetMetadata(), DXGI_FORMAT_A8_UNORM, DirectX::TEX_FILTER_DEFAULT, 0, *alphaScratch), false);
		alphaTextures.Push(alphaScratch);
	}

	return true;
}

static ff::SpriteType UpdateSpriteType(const OptimizedSpriteInfo &sprite, const DirectX::ScratchImage &alphaTexture)
{
	const ff::SpriteData &data = sprite._sprite->GetSpriteData();
	if (data._type != ff::SpriteType::Unknown || alphaTexture.GetImageCount() == 0)
	{
		return data._type;
	}

	ff::RectInt rect = sprite._destRect;
	ff::SpriteType newType = ff::SpriteType::Opaque;
	const DirectX::Image &image = *alphaTexture.GetImages();

	for (int y = rect.top; y < rect.bottom && newType == ff::SpriteType::Opaque; y++)
	{
		const uint8_t *alpha = image.pixels + y * image.rowPitch + rect.left;
		for (int x = rect.left; x < rect.right; x++, alpha++)
		{
			if (*alpha && *alpha != 0xFF)
			{
				newType = ff::SpriteType::Transparent;
				break;
			}
		}
	}

	return newType;
}

bool ff::OptimizeSprites(ISpriteList *pSprites, DXGI_FORMAT format, size_t nMipMapLevels, ISpriteList **ppOptimizedSprites)
{
	assertRetVal(pSprites && ppOptimizedSprites, false);

	// Create the return value
	ComPtr<ISpriteList> pNewSprites;
	assertRetVal(CreateSpriteList(pSprites->GetDevice(), &pNewSprites), false);

	// Cache all the sprites
	Vector<OptimizedSpriteInfo> sprites;
	sprites.Reserve(pSprites->GetCount());

	for (size_t i = 0; i < pSprites->GetCount(); i++)
	{
		OptimizedSpriteInfo sprite(pSprites->Get(i), i);
		sprites.Push(sprite);
	}

	// Sort the sprites by size
	if (!sprites.IsEmpty())
	{
		qsort(sprites.Data(), sprites.Size(), sizeof(OptimizedSpriteInfo), CompareSpriteInfo);
	}

	// Figure out how many textures to create
	Vector<OptimizedTextureInfo> textureInfos;
	Vector<ComPtr<IGraphTexture>> textures;
	Vector<ComPtr<IGraphTexture>> finalTextures;
	Vector<std::shared_ptr<DirectX::ScratchImage>> alphaTextures;

	assertRetVal(ComputeOptimizedSprites(sprites, textureInfos), false);
	assertRetVal(CreateOptimizedTextures(format, pSprites->GetDevice(), textureInfos, textures), false);

	if (!sprites.IsEmpty())
	{
		qsort(sprites.Data(), sprites.Size(), sizeof(OptimizedSpriteInfo), CompareSpriteInfoIndex);
	}

	assertRetVal(CopyOptimizedSprites(sprites, textures), false);
	assertRetVal(ConvertOptimizedTextures(format, nMipMapLevels, textures, finalTextures, alphaTextures), false);

	// Create final sprites
	for (size_t i = 0; i < sprites.Size(); i++)
	{
		OptimizedSpriteInfo &sprite = sprites[i];
		const SpriteData &data = sprite._sprite->GetSpriteData();
		SpriteType type = UpdateSpriteType(sprite, *alphaTextures[sprite._destTexture]);

		assertRetVal(pNewSprites->Add(
			finalTextures[sprite._destTexture],
			data._name,
			sprite._destRect.ToFloat(),
			sprite._srcHandle,
			sprite._srcScale,
			type), false);
	}

	*ppOptimizedSprites = pNewSprites.Detach();
	return true;
}
