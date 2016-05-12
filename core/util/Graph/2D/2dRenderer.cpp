#include "pch.h"
#include "COM/ComAlloc.h"
#include "Graph/Anim/AnimPos.h"
#include "Graph/2D/2dEffect.h"
#include "Graph/2D/2dRenderer.h"
#include "Graph/2D/Sprite.h"
#include "Graph/BufferCache.h"
#include "Graph/GraphDevice.h"
#include "Graph/GraphTexture.h"
#include "Graph/RenderTarget/RenderTarget.h"
#include "Graph/VertexFormat.h"
#include "Module/ModuleFactory.h"

static const size_t MAX_MULTI_SPRITE_TEXTURES = 4;
static const size_t MAX_BUCKET_TEXTURES = 8;
static const size_t MAX_DEFER_SPRITES = 16384;
static const size_t SPRITE_CHUNK_SIZE = 2048;
static const size_t MULTI_SPRITE_CHUNK_SIZE = 1024;
static const float SPRITE_DEPTH_DELTA = 1.0f / 2048.0f;
static const float MAX_SPRITE_DEPTH = MAX_DEFER_SPRITES * SPRITE_DEPTH_DELTA;

inline static bool HasAlpha(const DirectX::XMFLOAT4 &color)
{
	return color.w != 1 && color.w != 0;
}

// Keeps track of all the textures used when drawing a multi-sprite.
// There can be dupes in this list since sprites don't have to use unique textures.
typedef ff::Vector<ff::IGraphTexture *, MAX_MULTI_SPRITE_TEXTURES> MultiSpriteTextures;
typedef ff::Vector<ff::IGraphTexture *, MAX_BUCKET_TEXTURES> BucketTextures;

// Keeps track of a set of textures that can be selected into the GPU
// at the same time. The list of sprites and multi-sprites to draw using
// those textures are also stored here.
struct TextureBucket
{
	ff::Vector<ff::IGraphTexture*, MAX_BUCKET_TEXTURES> _textures;
	ff::SpriteVertexes *_opaqueSprites;
	ff::SpriteVertexes *_alphaSprites;
	ff::MultiSpriteVertexes *_opaqueMultiSprites;
	ff::MultiSpriteVertexes *_alphaMultiSprites;

	// Checks if this bucket can be used when drawing a new sprite or multi-sprite
	bool IsSupported(ff::IGraphTexture *texture, size_t &index) const;
	bool AddSupport(ff::IGraphTexture *texture, size_t &index);

	bool IsSupported(const MultiSpriteTextures &mst, size_t indexes[MAX_MULTI_SPRITE_TEXTURES]) const;
	bool AddSupport(const MultiSpriteTextures &mst, size_t indexes[MAX_MULTI_SPRITE_TEXTURES]);
};

enum class SortEntryType
{
	Sprite,
	MultiSprite,
	Line,
	Triangle,
};

struct SortEntry
{
	float _depth;
	SortEntryType _type;
	TextureBucket *_bucket;

	union
	{
		const ff::SpriteVertex *_firstSprite;
		const ff::MultiSpriteVertex *_firstMultiSprite;
		const ff::LineArtVertex *_firstLine;
		const ff::LineArtVertex *_firstTriangle;
	};

	bool operator<(const SortEntry &rhs)
	{
		return _depth > rhs._depth;
	}

	bool IsCompatible(const SortEntry &rhs)
	{
		return _type == rhs._type && _bucket == rhs._bucket;
	}
};

struct RenderState
{
	ff::ComPtr<ff::IRenderTarget> _target;
	ff::ComPtr<ff::IRenderDepth> _depth;
	ff::RectFloat _viewRect;

	// Cached old state
	size_t _matrixStackSize[ff::MATRIX_COUNT];
	size_t _effectStackSize;
	float _oldSpriteDepth;
};

class __declspec(uuid("6e511227-0dd9-4966-9ced-d244c9eb3c46"))
	Renderer2d : public ff::ComBase, public ff::I2dRenderer
{
public:
	DECLARE_HEADER(Renderer2d);

	bool Init();

	// ComBase
	virtual HRESULT _Construct(IUnknown *unkOuter) override;

	// IGraphDeviceChild
	virtual ff::IGraphDevice *GetDevice() const override;
	virtual bool Reset() override;

	// ff::I2dRenderer functions
	virtual bool BeginRender(
		ff::IRenderTarget *target,
		ff::IRenderDepth *depth,
		ff::RectFloat viewRect,
		ff::PointFloat worldTopLeft,
		ff::PointFloat worldScale,
		ff::I2dEffect *effect) override;

	virtual void Flush() override;
	virtual void EndRender(bool resetDepth) override;
	virtual ff::I2dEffect *GetEffect() override;
	virtual ff::IRenderTarget *GetTarget() override;
	virtual bool PushEffect(ff::I2dEffect *effect) override;
	virtual void PopEffect() override;
	virtual void NudgeDepth() override;
	virtual void ResetDepth() override;
	virtual const ff::RectFloat &GetRenderViewRect() const override;
	virtual const ff::RectFloat &GetRenderWorldRect() const override;
	virtual ff::PointFloat GetZBounds() const override;
	virtual bool ForceOpaqueUntilFlush(bool forceOpaque) override;

	virtual void DrawSprite(ff::ISprite *sprite, const ff::PointFloat *pos, const ff::PointFloat *scale, const float rotate, const DirectX::XMFLOAT4 *color) override;
	virtual void DrawMultiSprite(ff::ISprite **ppSprite, size_t spriteCount, const ff::PointFloat *pos, const ff::PointFloat *scale, const float rotate, const DirectX::XMFLOAT4 *color, size_t colorCount) override;
	virtual void DrawPolyLine(const ff::PointFloat *points, size_t count, const DirectX::XMFLOAT4 *color) override;
	virtual void DrawClosedPolyLine(const ff::PointFloat *points, size_t count, const DirectX::XMFLOAT4 *color) override;
	virtual void DrawLine(const ff::PointFloat *pStart, const ff::PointFloat *pEnd, const DirectX::XMFLOAT4 *color) override;
	virtual void DrawRectangle(const ff::RectFloat *rect, const DirectX::XMFLOAT4 *color) override;
	virtual void DrawRectangle(const ff::RectFloat *rect, float thickness, const DirectX::XMFLOAT4 *color) override;
	virtual void DrawFilledRectangle(const ff::RectFloat *rect, const DirectX::XMFLOAT4 *color, size_t colorCount) override;
	virtual void DrawFilledTriangle(const ff::PointFloat *points, const DirectX::XMFLOAT4 *color, size_t colorCount) override;

	virtual const DirectX::XMFLOAT4X4 &GetMatrix(ff::MatrixType type) override;
	virtual void SetMatrix(ff::MatrixType type, const DirectX::XMFLOAT4X4 *matrix) override;
	virtual void TransformMatrix(ff::MatrixType type, const DirectX::XMFLOAT4X4 *matrix) override;
	virtual void PushMatrix(ff::MatrixType type) override;
	virtual void PopMatrix(ff::MatrixType type) override;

private:
	void Destroy();

	bool BeginRender(RenderState *pState);
	void EndRender(RenderState *pState);
	void PopRenderState(bool resetDepth);

	void FlushOpaqueSprites();
	void FlushOpaqueTriangles();
	void FlushOpaqueLines();
	void FlushAllTransparent();

	// Render state
	ff::ComPtr<ff::IGraphDevice> _device;
	ff::Vector<DirectX::XMFLOAT4X4> _matrixStack[ff::MATRIX_COUNT];
	ff::Vector<ff::ComPtr<ff::I2dEffect>> _effectStack;
	std::stack<RenderState> _renderStateStack;
	RenderState *_renderState;
	ff::RectFloat _renderWorldRect;
	float _spriteDepth;
	bool _forceOpaque;

	// Line art
	ff::LineArtVertexes _opaqueLines;
	ff::LineArtVertexes _alphaLines;
	ff::LineArtVertexes _opaqueTriangles;
	ff::LineArtVertexes _alphaTriangles;

	// Sprites
	void ClearTextureBuckets();
	void DeleteSpriteArrays();
	TextureBucket *GetTextureBucket(const ff::ComPtr<ff::IGraphTexture> &texture, size_t &index);
	TextureBucket *GetTextureBucket(const MultiSpriteTextures &mst, size_t indexes[MAX_MULTI_SPRITE_TEXTURES]);
	void MapTexturesToBucket(const MultiSpriteTextures &mst, TextureBucket *pBucket);
	void CacheTextureBucket(TextureBucket *pBucket);
	void CombineTextureBuckets();

	ff::ComPtr<ID3D11Buffer> _spriteIndexes;
	ff::ComPtr<ID3D11Buffer> _triangleIndexes;
	ff::Vector<TextureBucket *> _textureBuckets;
	TextureBucket *_lastBuckets[2];
	ff::Map<ff::ComPtr<ff::IGraphTexture>, TextureBucket*> _textureToBuckets;

	// Sprite allocator stuff
	TextureBucket *NewTextureBucket();
	ff::SpriteVertexes *NewSpriteVertexes();
	ff::MultiSpriteVertexes *NewMultiSpriteVertexes();
	void DeleteTextureBucket(TextureBucket *pObj);
	void DeleteSpriteVertexes(ff::SpriteVertexes *pObj);
	void DeleteMultiSpriteVertexes(ff::MultiSpriteVertexes *pObj);

	ff::PoolAllocator<TextureBucket> _textureBucketPool;
	ff::Vector<ff::SpriteVertexes *> _freeSprites;
	ff::Vector<ff::MultiSpriteVertexes *> _freeMultiSprites;
	ff::Vector<SortEntry> _alphaSort;
};

BEGIN_INTERFACES(Renderer2d)
	HAS_INTERFACE(ff::I2dRenderer)
	HAS_INTERFACE(ff::IGraphDeviceChild)
END_INTERFACES()

static ff::ModuleStartup Register([](ff::Module &module)
{
	static ff::StaticString name(L"Renderer2d");
	module.RegisterClassT<Renderer2d>(name);
});

bool ff::Create2dRenderer(ff::IGraphDevice *pDevice, ff::I2dRenderer **ppRender)
{
	assertRetVal(ppRender && pDevice, false);
	*ppRender = nullptr;

	ff::ComPtr<Renderer2d> pRender;
	assertHrRetVal(ff::ComAllocator<Renderer2d>::CreateInstance(pDevice, &pRender), false);
	assertRetVal(pRender->Init(), false);

	*ppRender = pRender.Detach();
	return true;
}

bool TextureBucket::IsSupported(ff::IGraphTexture *texture, size_t &index) const
{
	index = _textures.Find(texture);
	return index != ff::INVALID_SIZE;
}

bool TextureBucket::AddSupport(ff::IGraphTexture *texture, size_t &index)
{
	index = _textures.Find(texture);

	if (index == ff::INVALID_SIZE)
	{
		if (_textures.Size() < MAX_BUCKET_TEXTURES)
		{
			index = _textures.Size();
			_textures.Push(texture);
		}
		else
		{
			return false;
		}
	}

	return true;
}

bool TextureBucket::IsSupported(const MultiSpriteTextures &mst, size_t indexes[MAX_MULTI_SPRITE_TEXTURES]) const
{
	ff::IGraphTexture *pPrev = nullptr;

	for (size_t i = 0; i < mst.Size(); i++)
	{
		ff::IGraphTexture *pFind = mst[i];

		if (pFind != pPrev)
		{
			indexes[i] = _textures.Find(pFind);

			if (indexes[i] == ff::INVALID_SIZE)
			{
				return false;
			}

			pPrev = pFind;
		}
		else
		{
			indexes[i] = indexes[i - 1];
		}
	}

	// Every texture was found in my list
	return true;
}

bool TextureBucket::AddSupport(const MultiSpriteTextures &mst, size_t indexes[MAX_MULTI_SPRITE_TEXTURES])
{
	ff::Vector<ff::IGraphTexture*, MAX_BUCKET_TEXTURES> addTextures;

	// See which textures aren't already in my list

	for (size_t i = 0; i < mst.Size(); i++)
	{
		ff::IGraphTexture *pFind = mst[i];
		indexes[i] = _textures.Find(pFind);

		if (indexes[i] == ff::INVALID_SIZE)
		{
			indexes[i] = addTextures.Find(pFind);

			if (indexes[i] == ff::INVALID_SIZE)
			{
				indexes[i] = _textures.Size() + addTextures.Size();
				addTextures.Push(pFind);
			}
			else
			{
				indexes[i] += _textures.Size();
			}
		}
	}

	if (addTextures.Size() > MAX_BUCKET_TEXTURES - _textures.Size())
	{
		// Not enough empty slots to add the new textures
		return false;
	}

	// Add each new texture

	for (size_t i = 0; i < addTextures.Size(); i++)
	{
		_textures.Push(addTextures[i]);
	}

	return true;
}

Renderer2d::Renderer2d()
	: _renderState(nullptr)
	, _spriteDepth(0)
	, _forceOpaque(false)
	, _textureBucketPool(false)
	, _renderWorldRect(0, 0, 0, 0)
{
	ff::ZeroObject(_lastBuckets);

	_textureToBuckets.SetBucketCount(256, false);

	PushMatrix(ff::MATRIX_PROJECTION);
	PushMatrix(ff::MATRIX_VIEW);
	PushMatrix(ff::MATRIX_WORLD);
}

Renderer2d::~Renderer2d()
{
	Destroy();

	if (_device)
	{
		_device->RemoveChild(this);
	}
}

HRESULT Renderer2d::_Construct(IUnknown *unkOuter)
{
	assertRetVal(_device.QueryFrom(unkOuter), E_FAIL);
	_device->AddChild(this);

	return __super::_Construct(unkOuter);
}

bool Renderer2d::Init()
{
	assertRetVal(_device, false);

	ff::Vector<WORD> indexes;
	indexes.Resize(SPRITE_CHUNK_SIZE * 6);

	WORD wCurVertex = 0;
	for (size_t nCurIndex = 0; nCurIndex < indexes.Size(); nCurIndex += 6, wCurVertex += 4)
	{
		indexes[nCurIndex + 0] = wCurVertex + 0;
		indexes[nCurIndex + 1] = wCurVertex + 1;
		indexes[nCurIndex + 2] = wCurVertex + 2;
		indexes[nCurIndex + 3] = wCurVertex + 0;
		indexes[nCurIndex + 4] = wCurVertex + 2;
		indexes[nCurIndex + 5] = wCurVertex + 3;
	}

	assertRetVal(_device->GetIndexBuffers().CreateStaticBuffer(
		indexes.Data(), indexes.ByteSize(), &_spriteIndexes), false);

	indexes.Reserve(SPRITE_CHUNK_SIZE * 3);

	wCurVertex = 0;
	for (size_t nCurIndex = 0; nCurIndex < indexes.Size(); nCurIndex++, wCurVertex++)
	{
		indexes[nCurIndex] = wCurVertex;
	}

	assertRetVal(_device->GetIndexBuffers().CreateStaticBuffer(
		indexes.Data(), indexes.ByteSize(), &_triangleIndexes), false);

	return true;
}

void Renderer2d::Destroy()
{
	DeleteSpriteArrays();

	_spriteIndexes = nullptr;
	_triangleIndexes = nullptr;
}

ff::IGraphDevice *Renderer2d::GetDevice() const
{
	return _device;
}

bool Renderer2d::Reset()
{
	Destroy();
	assertRetVal(Init(), false);
	return true;
}

bool Renderer2d::BeginRender(RenderState *pState)
{
	// Target and depth views
	{
		ID3D11RenderTargetView *pTargetView = pState->_target->GetTarget();
		ID3D11DepthStencilView *pDepthView = nullptr;

		if (!pTargetView)
		{
			return false;
		}

		if (pState->_depth)
		{
			pDepthView = pState->_depth->GetView();
			assertRetVal(pDepthView, false);
		}

		_device->GetContext()->OMSetRenderTargets(1, &pTargetView, pDepthView);
	}

	// Viewport
	{
		D3D11_VIEWPORT viewport;
		viewport.TopLeftX = pState->_viewRect.left;
		viewport.TopLeftY = pState->_viewRect.top;
		viewport.Width = pState->_viewRect.Width();
		viewport.Height = pState->_viewRect.Height();
		viewport.MinDepth = 0;
		viewport.MaxDepth = 1;

		_device->GetContext()->RSSetViewports(1, &viewport);
	}

	// Effect

	if (!GetEffect()->OnBeginRender(this))
	{
		return false;
	}

	return true;
}

void Renderer2d::EndRender(RenderState *pState)
{
	Flush();

	GetEffect()->OnEndRender(this);
}

void Renderer2d::PopRenderState(bool resetDepth)
{
	assertRet(_renderState);

	assert(_renderState->_effectStackSize < _effectStack.Size());
	_effectStack.Resize(_renderState->_effectStackSize);

	for (size_t i = 0; i < ff::MATRIX_COUNT; i++)
	{
		assert(_renderState->_matrixStackSize[i] <= _matrixStack[i].Size());
		_matrixStack[i].Resize(_renderState->_matrixStackSize[i]);
	}

	if (resetDepth)
	{
		_spriteDepth = _renderState->_oldSpriteDepth;
	}

	_renderStateStack.pop();

	if (!_renderStateStack.empty())
	{
		_renderState = &_renderStateStack.top();
		BeginRender(_renderState);
	}
	else
	{
		_renderState = nullptr;
		_renderWorldRect.SetRect(0, 0, 0, 0);
	}
}

bool Renderer2d::BeginRender(
	ff::IRenderTarget *target,
	ff::IRenderDepth *depth,
	ff::RectFloat viewRect,
	ff::PointFloat worldTopLeft,
	ff::PointFloat worldScale,
	ff::I2dEffect *effect)
{
	assertRetVal(target && effect && effect->IsValid(), false);
	assertRetVal(worldScale.x != 0.0f && worldScale.y != 0.0f, false);
	noAssertRetVal(viewRect.Width() > 0 && viewRect.Height() > 0, false);

	if (_renderState)
	{
		EndRender(_renderState);
	}

	// Create new render state
	{
		_renderStateStack.push(RenderState());
		_renderState = &_renderStateStack.top();
		_renderState->_target = target;
		_renderState->_depth = depth;
		_renderState->_oldSpriteDepth = _spriteDepth;
		_renderState->_effectStackSize = _effectStack.Size();

		switch (target->GetRotatedDegrees())
		{
			default:
				_renderState->_viewRect = viewRect;
				break;

			case 90:
			{
				float height = (float)target->GetRotatedSize().y;
				_renderState->_viewRect.left = height - viewRect.bottom;
				_renderState->_viewRect.top = viewRect.left;
				_renderState->_viewRect.right = height - viewRect.top;
				_renderState->_viewRect.bottom = viewRect.right;
			} break;

			case 180:
			{
				ff::PointFloat targetSize = target->GetRotatedSize().ToFloat();
				_renderState->_viewRect.left = targetSize.x - viewRect.right;
				_renderState->_viewRect.top = targetSize.y - viewRect.bottom;
				_renderState->_viewRect.right = targetSize.x - viewRect.left;
				_renderState->_viewRect.bottom = targetSize.y - viewRect.top;
			} break;

			case 270:
			{
				float width = (float)target->GetRotatedSize().x;
				_renderState->_viewRect.left = viewRect.top;
				_renderState->_viewRect.top = width - viewRect.right;
				_renderState->_viewRect.right = viewRect.bottom;
				_renderState->_viewRect.bottom = width - viewRect.left;
			} break;
		}

		for (size_t i = 0; i < ff::MATRIX_COUNT; i++)
		{
			_renderState->_matrixStackSize[i] = _matrixStack[i].Size();
		}
	}

	// Push new state
	{
		_renderWorldRect.SetRect(worldTopLeft, worldTopLeft + viewRect.Size() / worldScale);
		ff::PointFloat worldCenter = _renderWorldRect.Center();

		DirectX::XMMATRIX worldMatrix = DirectX::XMMatrixOrthographicOffCenterLH(
			_renderWorldRect.left, _renderWorldRect.right,
			-_renderWorldRect.bottom, -_renderWorldRect.top,
			-MAX_SPRITE_DEPTH, SPRITE_DEPTH_DELTA);

		DirectX::XMMATRIX orientationMatrix;
		switch (target->GetRotatedDegrees())
		{
			default:
				orientationMatrix = DirectX::XMMatrixIdentity();
				break;

			case 90:
			{
				float viewHeightPerWidth = viewRect.Height() / viewRect.Width();
				float viewWidthPerHeight = viewRect.Width() / viewRect.Height();

				orientationMatrix =
					DirectX::XMMatrixTransformation2D(
						DirectX::XMVectorSet(worldCenter.x, -worldCenter.y, 0, 0), 0, // scale center
						DirectX::XMVectorSet(viewHeightPerWidth, viewWidthPerHeight, 1, 1), // scale
						DirectX::XMVectorSet(worldCenter.x, -worldCenter.y, 0, 0), // rotation center
						(float)(ff::PI_D * 3 / 2), // rotation
						DirectX::XMVectorZero()); // translation
			} break;

			case 180:
				orientationMatrix =
					DirectX::XMMatrixAffineTransformation2D(
						DirectX::XMVectorSet(1, 1, 1, 1), // scale
						DirectX::XMVectorSet(worldCenter.x, -worldCenter.y, 0, 0), // rotation center
						ff::PI_F, // rotation
						DirectX::XMVectorZero()); // translation
				break;
		
			case 270:
			{
				float viewHeightPerWidth = viewRect.Height() / viewRect.Width();
				float viewWidthPerHeight = viewRect.Width() / viewRect.Height();

				orientationMatrix =
					DirectX::XMMatrixTransformation2D(
						DirectX::XMVectorSet(worldCenter.x, -worldCenter.y, 0, 0), 0, // scale center
						DirectX::XMVectorSet(viewHeightPerWidth, viewWidthPerHeight, 1, 1), // scale
						DirectX::XMVectorSet(worldCenter.x, -worldCenter.y, 0, 0), // rotation center
						(float)(ff::PI_D / 2), // rotation
						DirectX::XMVectorZero()); // translation
			} break;
		}

		DirectX::XMFLOAT4X4 projectionMatrix;
		DirectX::XMStoreFloat4x4(&projectionMatrix, orientationMatrix * worldMatrix);

		PushMatrix(ff::MATRIX_PROJECTION);
		SetMatrix(ff::MATRIX_PROJECTION, &projectionMatrix);

		_effectStack.Push(effect);
	}

	if (!BeginRender(_renderState))
	{
		PopRenderState(true);
		return false;
	}

	return true;
}

void Renderer2d::EndRender(bool resetDepth)
{
	assertRet(_renderState);

	EndRender(_renderState);
	PopRenderState(resetDepth);
}

ff::I2dEffect *Renderer2d::GetEffect()
{
	return _effectStack.Size() ? _effectStack.GetLast() : nullptr;
}

ff::IRenderTarget *Renderer2d::GetTarget()
{
	assertRetVal(_renderState, nullptr);
	return _renderState->_target;
}

bool Renderer2d::PushEffect(ff::I2dEffect *effect)
{
	assertRetVal(effect && effect->IsValid(), false);

	bool bNewEffect = (effect != GetEffect());

	if (bNewEffect)
	{
		Flush();
		GetEffect()->OnEndRender(this);
	}

	_effectStack.Push(effect);

	if (bNewEffect && !GetEffect()->OnBeginRender(this))
	{
		_effectStack.Pop();
		return false;
	}

	return true;
}

void Renderer2d::PopEffect()
{
	assertRet(_effectStack.Size() > 1);

	bool bNewEffect = (_effectStack.GetLast() != _effectStack[_effectStack.Size() - 2]);

	if (bNewEffect)
	{
		Flush();
		GetEffect()->OnEndRender(this);
	}

	_effectStack.Pop();

	if (bNewEffect)
	{
		verify(GetEffect()->OnBeginRender(this));
	}
}

void Renderer2d::NudgeDepth()
{
	_spriteDepth -= SPRITE_DEPTH_DELTA;
}

void Renderer2d::ResetDepth()
{
	_spriteDepth = 0;
}

const ff::RectFloat &Renderer2d::GetRenderViewRect() const
{
	assert(_renderState);
	return _renderState->_viewRect;
}

const ff::RectFloat &Renderer2d::GetRenderWorldRect() const
{
	assert(_renderState);
	return _renderWorldRect;
}

ff::PointFloat Renderer2d::GetZBounds() const
{
	return ff::PointFloat(-MAX_SPRITE_DEPTH, SPRITE_DEPTH_DELTA);
}

bool Renderer2d::ForceOpaqueUntilFlush(bool forceOpaque)
{
	bool old = _forceOpaque;
	_forceOpaque = forceOpaque;
	return old;
}

void Renderer2d::Flush()
{
	_forceOpaque = false;
	assertRet(_renderState);

	CombineTextureBuckets();

	FlushOpaqueTriangles();
	FlushOpaqueLines();
	FlushOpaqueSprites();
	FlushAllTransparent();

	ClearTextureBuckets();
}

static void AdjustSpriteRect(
	const ff::RectFloat &rect,
	const ff::PointFloat *pos,
	const ff::PointFloat *scale,
	const float rotate,
	ff::PointFloat outRectPoints[4])
{
	DirectX::XMMATRIX matrix = DirectX::XMMatrixAffineTransformation2D(
		scale ? DirectX::XMLoadFloat2((DirectX::XMFLOAT2*)scale) : DirectX::XMVectorSplatOne(),
		DirectX::XMVectorZero(),
		rotate,
		pos ? DirectX::XMLoadFloat2((DirectX::XMFLOAT2*)pos) : DirectX::XMVectorZero());

	outRectPoints[0].SetPoint(rect.left, rect.top);
	outRectPoints[1].SetPoint(rect.right, rect.top);
	outRectPoints[2].SetPoint(rect.right, rect.bottom);
	outRectPoints[3].SetPoint(rect.left, rect.bottom);

	DirectX::XMFLOAT4 output[4];
	DirectX::XMVector2TransformStream(output, sizeof(DirectX::XMFLOAT4), (DirectX::XMFLOAT2*)outRectPoints, sizeof(DirectX::XMFLOAT2), 4, matrix);

	outRectPoints[0].SetPoint(output[0].x, output[0].y);
	outRectPoints[1].SetPoint(output[1].x, output[1].y);
	outRectPoints[2].SetPoint(output[2].x, output[2].y);
	outRectPoints[3].SetPoint(output[3].x, output[3].y);
}

static void AdjustSpriteRect(const ff::RectFloat &rect, const ff::PointFloat *pos, const ff::PointFloat *scale, ff::PointFloat outRectPoints[4])
{
	ff::RectFloat rectCopy;
	const ff::RectFloat *pRectCopy;

	if (pos || scale)
	{
		DirectX::XMStoreFloat4((DirectX::XMFLOAT4*)&rectCopy.arr[0],
			DirectX::XMVectorMultiplyAdd(
				DirectX::XMLoadFloat4((const DirectX::XMFLOAT4*)&rect.arr[0]),
				scale ? DirectX::XMVectorSet(scale->x, scale->y, scale->x, scale->y) : DirectX::XMVectorSplatOne(),
				pos ? DirectX::XMVectorSet(pos->x, pos->y, pos->x, pos->y) : DirectX::XMVectorZero()));

		pRectCopy = &rectCopy;
	}
	else
	{
		pRectCopy = &rect;
	}

	outRectPoints[0].SetPoint(pRectCopy->left, pRectCopy->top);
	outRectPoints[1].SetPoint(pRectCopy->right, pRectCopy->top);
	outRectPoints[2].SetPoint(pRectCopy->right, pRectCopy->bottom);
	outRectPoints[3].SetPoint(pRectCopy->left, pRectCopy->bottom);
}

void Renderer2d::DrawSprite(
		ff::ISprite *sprite,
		const ff::PointFloat *pos,
		const ff::PointFloat *scale,
		const float rotate,
		const DirectX::XMFLOAT4 *color)
{
	assertRet(sprite);

	const ff::SpriteData &data = sprite->GetSpriteData();
	size_t textureIndex = ff::INVALID_SIZE;

	ff::PointFloat rectPoints[4];
	if (rotate != 0)
	{
		AdjustSpriteRect(data._worldRect, pos, scale, -rotate, rectPoints);
	}
	else
	{
		AdjustSpriteRect(data._worldRect, pos, scale, rectPoints);
	}

	ff::SpriteVertex sv;
	sv.color = color ? *color : ff::GetColorWhite();

	bool hasAlpha = (data._type == ff::SpriteType::Transparent) || ::HasAlpha(sv.color);
	TextureBucket *bucket = GetTextureBucket(data._texture, textureIndex);
	ff::SpriteVertexes *spriteArray = (hasAlpha && !_forceOpaque) ? bucket->_alphaSprites : bucket->_opaqueSprites;
	size_t firstSprite = spriteArray->Size();

	spriteArray->InsertDefault(firstSprite, 4);
	ff::SpriteVertex *writeSprite = spriteArray->Data(firstSprite, 4);

	// upper left
	sv.pos.x = rectPoints[0].x;
	sv.pos.y = -rectPoints[0].y;
	sv.pos.z = _spriteDepth;
	sv.tex.x = data._textureUV.left;
	sv.tex.y = data._textureUV.top;
	sv.ntex = (UINT)textureIndex;
	writeSprite[0] = sv;

	// upper right
	sv.pos.x = rectPoints[1].x;
	sv.pos.y = -rectPoints[1].y;
	sv.tex.x = data._textureUV.right;
	writeSprite[1] = sv;

	// bottom right
	sv.pos.x = rectPoints[2].x;
	sv.pos.y = -rectPoints[2].y;
	sv.tex.y = data._textureUV.bottom;
	writeSprite[2] = sv;

	// bottom left
	sv.pos.x = rectPoints[3].x;
	sv.pos.y = -rectPoints[3].y;
	sv.tex.x = data._textureUV.left;
	writeSprite[3] = sv;

	NudgeDepth();
}

void Renderer2d::DrawMultiSprite(
	ff::ISprite **ppSprite,
	size_t spriteCount,
	const ff::PointFloat *pos,
	const ff::PointFloat *scale,
	const float rotate,
	const DirectX::XMFLOAT4 *pColors,
	size_t colorCount)
{
	assertRet(ppSprite && spriteCount);

	DirectX::XMFLOAT4 colors[4] =
	{
		ff::GetColorWhite(),
		ff::GetColorWhite(),
		ff::GetColorWhite(),
		ff::GetColorWhite()
	};

	switch (colorCount)
	{
		case 4: colors[3] = pColors[3]; __fallthrough;
		case 3: colors[2] = pColors[2]; __fallthrough;
		case 2: colors[1] = pColors[1]; __fallthrough;
		case 1: colors[0] = pColors[0]; __fallthrough;
	}

	// Get the sprite data (up to four)

	const ff::SpriteData *data[4];
	data[0] = &ppSprite[0]->GetSpriteData();
	data[1] = data[0];
	data[2] = data[0];
	data[3] = data[0];

	switch (spriteCount)
	{
		case 4: data[3] = &ppSprite[3]->GetSpriteData(); __fallthrough;
		case 3: data[2] = &ppSprite[2]->GetSpriteData(); __fallthrough;
		case 2: data[1] = &ppSprite[1]->GetSpriteData(); __fallthrough;
	}

	bool hasAlpha =
		data[0]->_type == ff::SpriteType::Transparent ||
		data[1]->_type == ff::SpriteType::Transparent ||
		data[2]->_type == ff::SpriteType::Transparent ||
		data[3]->_type == ff::SpriteType::Transparent;

	if (!hasAlpha)
	{
		hasAlpha =
			::HasAlpha(colors[0]) ||
			::HasAlpha(colors[1]) ||
			::HasAlpha(colors[2]) ||
			::HasAlpha(colors[3]);
	}

	// Get the texture for each sprite data

	MultiSpriteTextures mst;
	mst.Push(data[0]->_texture);

	if (spriteCount > 1) mst.Push(data[1]->_texture);
	if (spriteCount > 2) mst.Push(data[2]->_texture);
	if (spriteCount > 3) mst.Push(data[3]->_texture);

	// Get the colors

	ff::MultiSpriteVertex msv;
	CopyMemory(&msv.color0, colors, sizeof(colors));

	// Find out where this sprite is drawn in the world (from the first sprite)

	ff::PointFloat rectPoints[4];

	if (rotate != 0)
	{
		AdjustSpriteRect(data[0]->_worldRect, pos, scale, -rotate, rectPoints);
	}
	else
	{
		AdjustSpriteRect(data[0]->_worldRect, pos, scale, rectPoints);
	}

	// Create the vertexes

	size_t textureIndexes[4] = { ff::INVALID_SIZE, ff::INVALID_SIZE, ff::INVALID_SIZE, ff::INVALID_SIZE };
	TextureBucket *bucket = GetTextureBucket(mst, textureIndexes);
	ff::MultiSpriteVertexes *spriteArray = (hasAlpha && !_forceOpaque) ? bucket->_alphaMultiSprites : bucket->_opaqueMultiSprites;
	size_t firstSprite = spriteArray->Size();
	spriteArray->InsertDefault(firstSprite, 4);
	ff::MultiSpriteVertex *writeSprite = spriteArray->Data(firstSprite, 4);

	// upper left
	msv.pos.x = rectPoints[0].x;
	msv.pos.y = -rectPoints[0].y;
	msv.pos.z = _spriteDepth;
	msv.tex0.x = data[0]->_textureUV.left;
	msv.tex0.y = data[0]->_textureUV.top;
	msv.tex1.x = data[1]->_textureUV.left;
	msv.tex1.y = data[1]->_textureUV.top;
	msv.tex2.x = data[2]->_textureUV.left;
	msv.tex2.y = data[2]->_textureUV.top;
	msv.tex3.x = data[3]->_textureUV.left;
	msv.tex3.y = data[3]->_textureUV.top;
	msv.ntex[0] = (textureIndexes[0] == ff::INVALID_SIZE) ? ff::INVALID_DWORD : (DWORD)textureIndexes[0];
	msv.ntex[1] = (textureIndexes[1] == ff::INVALID_SIZE) ? ff::INVALID_DWORD : (DWORD)textureIndexes[1];
	msv.ntex[2] = (textureIndexes[2] == ff::INVALID_SIZE) ? ff::INVALID_DWORD : (DWORD)textureIndexes[2];
	msv.ntex[3] = (textureIndexes[3] == ff::INVALID_SIZE) ? ff::INVALID_DWORD : (DWORD)textureIndexes[3];
	writeSprite[0] = msv;

	// upper right
	msv.pos.x = rectPoints[1].x;
	msv.pos.y = -rectPoints[1].y;
	msv.tex0.x = data[0]->_textureUV.right;
	msv.tex1.x = data[1]->_textureUV.right;
	msv.tex2.x = data[2]->_textureUV.right;
	msv.tex3.x = data[3]->_textureUV.right;
	writeSprite[1] = msv;

	// bottom right
	msv.pos.x = rectPoints[2].x;
	msv.pos.y = -rectPoints[2].y;
	msv.tex0.y = data[0]->_textureUV.bottom;
	msv.tex1.y = data[1]->_textureUV.bottom;
	msv.tex2.y = data[2]->_textureUV.bottom;
	msv.tex3.y = data[3]->_textureUV.bottom;
	writeSprite[2] = msv;

	// bottom left
	msv.pos.x = rectPoints[3].x;
	msv.pos.y = -rectPoints[3].y;
	msv.tex0.x = data[0]->_textureUV.left;
	msv.tex1.x = data[1]->_textureUV.left;
	msv.tex2.x = data[2]->_textureUV.left;
	msv.tex3.x = data[3]->_textureUV.left;
	writeSprite[3] = msv;

	NudgeDepth();
}

void Renderer2d::DrawPolyLine(const ff::PointFloat *points, size_t count, const DirectX::XMFLOAT4 *color)
{
	assertRet(points && count > 1);

	for (size_t i = 1; i < count; i++, points++)
	{
		DrawLine(points, points + 1, color);
	}
}

void Renderer2d::DrawClosedPolyLine(const ff::PointFloat *points, size_t count, const DirectX::XMFLOAT4 *color)
{
	DrawPolyLine(points, count, color);

	if (count >= 3)
	{
		DrawLine(&points[count - 1], points, color);
	}
}

void Renderer2d::DrawLine(const ff::PointFloat *pStart, const ff::PointFloat *pEnd, const DirectX::XMFLOAT4 *color)
{
	assertRet(pStart && pEnd);

	ff::LineArtVertex lv;
	lv.color = color ? *color : ff::GetColorWhite();
	lv.pos.z = _spriteDepth;

	ff::LineArtVertexes &vertexes = (::HasAlpha(lv.color) && !_forceOpaque) ? _alphaLines : _opaqueLines;

	lv.pos.x = pStart->x;
	lv.pos.y = -pStart->y;
	vertexes.Push(&lv, 1);

	lv.pos.x = pEnd->x;
	lv.pos.y = -pEnd->y;
	vertexes.Push(&lv, 1);

	NudgeDepth();
}

void Renderer2d::DrawRectangle(const ff::RectFloat *rect, const DirectX::XMFLOAT4 *color)
{
	assertRet(rect);

	const ff::PointFloat points[5] =
	{
		rect->TopLeft(),
		rect->TopRight(),
		rect->BottomRight(),
		rect->BottomLeft(),
		rect->TopLeft(),
	};

	DrawPolyLine(points, _countof(points), color);
}

void Renderer2d::DrawRectangle(const ff::RectFloat *rectIn, float thickness, const DirectX::XMFLOAT4 *color)
{
	assertRet(rectIn);

	ff::RectFloat rect(*rectIn);
	rect.Normalize();

	if (rect.Width() <= thickness || rect.Height() <= thickness)
	{
		rect.Deflate(-thickness, -thickness);
		DrawFilledRectangle(&rect, color, color ? 1 : 0);
	}
	else
	{
		size_t nColors = color ? 1 : 0;
		float oldDepth = _spriteDepth;
		float ht = thickness / 2;

		DirectX::XMVECTOR thickVector = DirectX::XMVectorSet(-ht, -ht, ht, ht);
		DirectX::XMVECTOR top = DirectX::XMVectorAdd(DirectX::XMVectorSet(rect.left, rect.top, rect.right, rect.top), thickVector);
		DirectX::XMVECTOR bottom = DirectX::XMVectorAdd(DirectX::XMVectorSet(rect.left, rect.bottom, rect.right, rect.bottom), thickVector);

		thickVector = DirectX::XMVectorSet(-ht, ht, ht, -ht);
		DirectX::XMVECTOR left = DirectX::XMVectorAdd(DirectX::XMVectorSet(rect.left, rect.top, rect.left, rect.bottom), thickVector);
		DirectX::XMVECTOR right = DirectX::XMVectorAdd(DirectX::XMVectorSet(rect.right, rect.top, rect.right, rect.bottom), thickVector);

		DirectX::XMStoreFloat4((DirectX::XMFLOAT4*)&rect, top);
		DrawFilledRectangle(&rect, color, nColors);

		_spriteDepth = oldDepth;
		DirectX::XMStoreFloat4((DirectX::XMFLOAT4*)&rect, bottom);
		DrawFilledRectangle(&rect, color, nColors);

		_spriteDepth = oldDepth;
		DirectX::XMStoreFloat4((DirectX::XMFLOAT4*)&rect, left);
		DrawFilledRectangle(&rect, color, nColors);

		_spriteDepth = oldDepth;
		DirectX::XMStoreFloat4((DirectX::XMFLOAT4*)&rect, right);
		DrawFilledRectangle(&rect, color, nColors);
	}
}

void Renderer2d::DrawFilledRectangle(const ff::RectFloat *rect, const DirectX::XMFLOAT4 *color, size_t colorCount)
{
	assertRet(rect);

	DirectX::XMFLOAT4 colors[4];

	switch (colorCount)
	{
	default:
		assert(false);
		__fallthrough;

	case 0:
		colors[0] = ff::GetColorWhite();
		colors[1] = ff::GetColorWhite();
		colors[2] = ff::GetColorWhite();
		colors[3] = ff::GetColorWhite();
		break;

	case 1:
		colors[0] = *color;
		colors[1] = *color;
		colors[2] = *color;
		colors[3] = *color;
		break;

	case 2:
		colors[0] = color[0];
		colors[1] = color[0];
		colors[2] = color[1];
		colors[3] = color[1];
		break;

	case 4:
		CopyMemory(colors, color, sizeof(colors));
		break;
	}

	bool hasAlpha =
		::HasAlpha(colors[0]) ||
		::HasAlpha(colors[1]) ||
		::HasAlpha(colors[2]) ||
		::HasAlpha(colors[3]);

	ff::LineArtVertexes &vertexes = (hasAlpha && !_forceOpaque) ? _alphaTriangles : _opaqueTriangles;
	ff::LineArtVertex lv;
	lv.pos.z = _spriteDepth;

	lv.pos.x = rect->left;
	lv.pos.y = -rect->top;
	lv.color = colors[0];
	vertexes.Push(&lv, 1);

	lv.pos.x = rect->right;
	lv.color = colors[1];
	vertexes.Push(&lv, 1);

	lv.pos.y = -rect->bottom;
	lv.color = colors[2];
	vertexes.Push(&lv, 1);
	vertexes.Push(&lv, 1);

	lv.pos.x = rect->left;
	lv.color = colors[3];
	vertexes.Push(&lv, 1);

	lv.pos.y = -rect->top;
	lv.color = colors[0];
	vertexes.Push(&lv, 1);

	NudgeDepth();
}

void Renderer2d::DrawFilledTriangle(const ff::PointFloat *points, const DirectX::XMFLOAT4 *color, size_t colorCount)
{
	assertRet(points);

	DirectX::XMFLOAT4 colors[3];

	switch (colorCount)
	{
	default:
		assert(false);
		__fallthrough;

	case 0:
		colors[0] = ff::GetColorWhite();
		colors[1] = ff::GetColorWhite();
		colors[2] = ff::GetColorWhite();
		break;

	case 1:
		colors[0] = *color;
		colors[1] = *color;
		colors[2] = *color;
		break;

	case 3:
		CopyMemory(colors, color, sizeof(colors));
		break;
	}

	bool hasAlpha =
		::HasAlpha(colors[0]) ||
		::HasAlpha(colors[1]) ||
		::HasAlpha(colors[2]);

	ff::LineArtVertexes &vertexes = (hasAlpha && !_forceOpaque) ? _alphaTriangles : _opaqueTriangles;
	ff::LineArtVertex lv;
	lv.pos.z = _spriteDepth;

	lv.pos.x = points[0].x;
	lv.pos.y = -points[0].y;
	lv.color = colors[0];
	vertexes.Push(&lv, 1);

	lv.pos.x = points[1].x;
	lv.pos.y = -points[1].y;
	lv.color = colors[1];
	vertexes.Push(&lv, 1);

	lv.pos.x = points[2].x;
	lv.pos.y = -points[2].y;
	lv.color = colors[2];
	vertexes.Push(&lv, 1);

	NudgeDepth();
}

const DirectX::XMFLOAT4X4 &Renderer2d::GetMatrix(ff::MatrixType type)
{
	return _matrixStack[type].GetLast();
}

void Renderer2d::SetMatrix(ff::MatrixType type, const DirectX::XMFLOAT4X4 *matrixIn)
{
	ff::I2dEffect *effect = GetEffect();

	if (effect)
	{
		Flush();
		effect->OnMatrixChanging(this, type);
	}

	DirectX::XMFLOAT4X4 &matrix = _matrixStack[type].GetLast();

	if (matrixIn)
	{
		matrix = *matrixIn;
	}
	else
	{
		matrix = ff::GetIdentityMatrix();
	}

	if (effect)
	{
		effect->OnMatrixChanged(this, type);
	}
}

void Renderer2d::TransformMatrix(ff::MatrixType type, const DirectX::XMFLOAT4X4 *matrixIn)
{
	if (matrixIn && _matrixStack[type].Size())
	{
		ff::I2dEffect *effect = GetEffect();

		if (effect)
		{
			Flush();
			effect->OnMatrixChanging(this, type);
		}

		DirectX::XMFLOAT4X4 &matrix = _matrixStack[type].GetLast();
		DirectX::XMStoreFloat4x4(&matrix,
			DirectX::XMLoadFloat4x4(matrixIn) * DirectX::XMLoadFloat4x4(&matrix));

		if (effect)
		{
			effect->OnMatrixChanged(this, type);
		}
	}
}

void Renderer2d::PushMatrix(ff::MatrixType type)
{
	DirectX::XMFLOAT4X4 matrix;

	if (_matrixStack[type].Size())
	{
		matrix = _matrixStack[type].GetLast();
	}
	else
	{
		matrix = ff::GetIdentityMatrix();
	}

	_matrixStack[type].Push(matrix);
}

void Renderer2d::PopMatrix(ff::MatrixType type)
{
	assertRet(_matrixStack[type].Size() > 1);

	ff::I2dEffect *effect = GetEffect();

	if (effect)
	{
		Flush();
		effect->OnMatrixChanging(this, type);
	}

	_matrixStack[type].Pop();

	if (effect)
	{
		effect->OnMatrixChanged(this, type);
	}
}

void Renderer2d::FlushOpaqueSprites()
{
	noAssertRet(_textureBuckets.Size());

	ff::I2dEffect *effect = GetEffect();

	for (TextureBucket *bucket : _textureBuckets)
	{
		ff::SpriteVertexes *sprites = bucket->_opaqueSprites;
		ff::MultiSpriteVertexes *multiSprites = bucket->_opaqueMultiSprites;

		effect->ApplyTextures(this, bucket->_textures.Data(), bucket->_textures.Size());

		if (sprites && sprites->Size())
		{
			for (size_t i = 0, count = 0; i < sprites->Size(); i += count)
			{
				count = std::min(
					sprites->Size() - i, // the number of vertexes left for this bucket
					SPRITE_CHUNK_SIZE * 4); // the number of vertex slots left for drawing

				ff::AutoBufferMap<ff::SpriteVertex> vertexBuffer(_device->GetVertexBuffers(), count);
				if (vertexBuffer.GetMem())
				{
					CopyMemory(vertexBuffer.GetMem(), sprites->Data(i), vertexBuffer.GetSize());

					if (effect->Apply(this, ff::DRAW_TYPE_SPRITE, vertexBuffer.Unmap(), _spriteIndexes, 0))
					{
						_device->GetContext()->DrawIndexed((UINT)count * 6 / 4, 0, 0);
					}
				}
			}
		}

		if (multiSprites && multiSprites->Size())
		{
			for (size_t i = 0, count = 0; i < multiSprites->Size(); i += count)
			{
				count = std::min(
					multiSprites->Size() - i, // the number of vertexes left for this bucket
					MULTI_SPRITE_CHUNK_SIZE * 4); // the number of vertex slots left for drawing

				ff::AutoBufferMap<ff::MultiSpriteVertex> vertexBuffer(_device->GetVertexBuffers(), count);
				if (vertexBuffer.GetMem())
				{
					CopyMemory(vertexBuffer.GetMem(), multiSprites->Data(i), vertexBuffer.GetSize());

					if (effect->Apply(this, ff::DRAW_TYPE_MULTI_SPRITE, vertexBuffer.Unmap(), _spriteIndexes, 0))
					{
						_device->GetContext()->DrawIndexed((UINT)count * 6 / 4, 0, 0);
					}
				}
			}
		}
	}
}

void Renderer2d::FlushOpaqueTriangles()
{
	noAssertRet(_opaqueTriangles.Size());

	for (size_t i = 0, count = 0; i < _opaqueTriangles.Size(); i += count)
	{
		count = std::min(_opaqueTriangles.Size() - i, SPRITE_CHUNK_SIZE * 3);
		assert(count % 3 == 0);

		ff::AutoBufferMap<ff::LineArtVertex> vertexBuffer(_device->GetVertexBuffers(), count);
		if (vertexBuffer.GetMem())
		{
			CopyMemory(vertexBuffer.GetMem(), &_opaqueTriangles[i], sizeof(ff::LineArtVertex) * count);

			if (GetEffect()->Apply(this, ff::DRAW_TYPE_TRIANGLES, vertexBuffer.Unmap(), _triangleIndexes, 0))
			{
				_device->GetContext()->DrawIndexed((UINT)count, 0, 0);
			}
		}
	}

	_opaqueTriangles.Clear();
}

void Renderer2d::FlushOpaqueLines()
{
	noAssertRet(_opaqueLines.Size());

	for (size_t i = 0; i < _opaqueLines.Size(); i += SPRITE_CHUNK_SIZE)
	{
		size_t nVertexCount = std::min(_opaqueLines.Size() - i, SPRITE_CHUNK_SIZE);
		assert(nVertexCount % 2 == 0);
		
		ff::AutoBufferMap<ff::LineArtVertex> vertexBuffer(_device->GetVertexBuffers(), nVertexCount);
		if (vertexBuffer.GetMem())
		{
			CopyMemory(vertexBuffer.GetMem(), &_opaqueLines[i], sizeof(ff::LineArtVertex) * nVertexCount);

			if (GetEffect()->Apply(this, ff::DRAW_TYPE_LINES, vertexBuffer.Unmap(), nullptr, 0))
			{
				_device->GetContext()->Draw((UINT)nVertexCount, 0);
			}
		}
	}

	_opaqueLines.Clear();
}

void Renderer2d::FlushAllTransparent()
{
	assert(_alphaSort.IsEmpty());

	// Populate _alphaSort
	SortEntry newEntry;

	// Sprites and multi-sprites from texture buckets
	for (TextureBucket *bucket : _textureBuckets)
	{
		newEntry._bucket = bucket;

		newEntry._type = SortEntryType::Sprite;
		for (size_t i = 0; i < bucket->_alphaSprites->Size(); i += 4)
		{
			const ff::SpriteVertex &sv = bucket->_alphaSprites->GetAt(i);
			newEntry._depth = sv.pos.z;
			newEntry._firstSprite = &sv;
			_alphaSort.Push(&newEntry, 1);
		}

		newEntry._type = SortEntryType::MultiSprite;
		for (size_t i = 0; i < bucket->_alphaMultiSprites->Size(); i += 4)
		{
			const ff::MultiSpriteVertex &msv = bucket->_alphaMultiSprites->GetAt(i);
			newEntry._depth = msv.pos.z;
			newEntry._firstMultiSprite = &msv;
			_alphaSort.Push(&newEntry, 1);
		}
	}

	// Lines
	newEntry._bucket = nullptr;
	newEntry._type = SortEntryType::Line;
	for (size_t i = 0; i < _alphaLines.Size(); i += 2)
	{
		const ff::LineArtVertex &lv = _alphaLines[i];
		newEntry._depth = lv.pos.z;
		newEntry._firstLine = &lv;
		_alphaSort.Push(&newEntry, 1);
	}

	// Triangles
	newEntry._type = SortEntryType::Triangle;
	for (size_t i = 0; i < _alphaTriangles.Size(); i += 3)
	{
		const ff::LineArtVertex &lv = _alphaTriangles[i];
		newEntry._depth = lv.pos.z;
		newEntry._firstLine = &lv;
		_alphaSort.Push(&newEntry, 1);
	}

	if (_alphaSort.IsEmpty())
	{
		return;
	}

	std::sort(_alphaSort.begin(), _alphaSort.end());

	ff::I2dEffect *effect = GetEffect();
	size_t count = 0;

	for (size_t i = 0; i < _alphaSort.Size(); i += count)
	{
		SortEntry *firstEntry = &_alphaSort[i];
		TextureBucket *bucket = firstEntry->_bucket;
		size_t chunkSize = (firstEntry->_type == SortEntryType::MultiSprite)
			? MULTI_SPRITE_CHUNK_SIZE
			: SPRITE_CHUNK_SIZE;

		// Gather as many entries as possible that have the same draw properties
		for (count = 1; i + count < _alphaSort.Size() && count < chunkSize; count++)
		{
			if (!firstEntry->IsCompatible(firstEntry[count]))
			{
				break;
			}
		}

		switch (firstEntry->_type)
		{
		default:
			assert(false);
			break;

		case SortEntryType::Sprite:
			{
				effect->ApplyTextures(this, bucket->_textures.Data(), bucket->_textures.Size());

				ff::AutoBufferMap<ff::SpriteVertex> vertexBuffer(_device->GetVertexBuffers(), count * 4);
				if (vertexBuffer.GetMem())
				{
					for (size_t h = 0; h < count; h++)
					{
						CopyMemory(vertexBuffer.GetMem() + h * 4, firstEntry[h]._firstSprite, sizeof(ff::SpriteVertex) * 4);
					}

					if (effect->Apply(this, ff::DRAW_TYPE_SPRITE, vertexBuffer.Unmap(), _spriteIndexes, 0))
					{
						for (size_t h = 0; h < count; h++)
						{
							_device->GetContext()->DrawIndexed(6, 0, (int)h * 4);
						}
					}
				}
			}
			break;

		case SortEntryType::MultiSprite:
			{
				effect->ApplyTextures(this, bucket->_textures.Data(), bucket->_textures.Size());

				ff::AutoBufferMap<ff::MultiSpriteVertex> vertexBuffer(_device->GetVertexBuffers(), count * 4);
				if (vertexBuffer.GetMem())
				{
					for (size_t h = 0; h < count; h++)
					{
						CopyMemory(vertexBuffer.GetMem() + h * 4, firstEntry[h]._firstMultiSprite, sizeof(ff::MultiSpriteVertex) * 4);
					}

					if (effect->Apply(this, ff::DRAW_TYPE_MULTI_SPRITE, vertexBuffer.Unmap(), _spriteIndexes, 0))
					{
						for (size_t h = 0; h < count; h++)
						{
							_device->GetContext()->DrawIndexed(6, 0, (int)h * 4);
						}
					}
				}
			}
			break;

		case SortEntryType::Line:
			{
				ff::AutoBufferMap<ff::LineArtVertex> vertexBuffer(_device->GetVertexBuffers(), count * 2);
				if (vertexBuffer.GetMem())
				{
					for (size_t h = 0; h < count; h++)
					{
						CopyMemory(vertexBuffer.GetMem() + h * 2, firstEntry[h]._firstLine, sizeof(ff::LineArtVertex) * 2);
					}

					if (effect->Apply(this, ff::DRAW_TYPE_LINES, vertexBuffer.Unmap(), nullptr, 0))
					{
						size_t drawn;
						for (size_t h = 0, left = count; h < count; h += drawn, left -= drawn)
						{
							// Items at the same depth can be drawn at the same time
							float depth = firstEntry[h]._depth;
							for (drawn = 1; drawn < left; drawn++)
							{
								if (firstEntry[h + drawn]._depth != depth)
								{
									break;
								}
							}

							_device->GetContext()->Draw((UINT)drawn * 2, (UINT)h * 2);
						}
					}
				}
			}
			break;

		case SortEntryType::Triangle:
			{
				ff::AutoBufferMap<ff::LineArtVertex> vertexBuffer(_device->GetVertexBuffers(), count * 3);
				if (vertexBuffer.GetMem())
				{
					for (size_t h = 0; h < count; h++)
					{
						CopyMemory(vertexBuffer.GetMem() + h * 3, firstEntry[h]._firstTriangle, sizeof(ff::LineArtVertex) * 3);
					}

					if (effect->Apply(this, ff::DRAW_TYPE_TRIANGLES, vertexBuffer.Unmap(), _triangleIndexes, 0))
					{
						size_t drawn;
						for (size_t h = 0, left = count; h < count; h += drawn, left -= drawn)
						{
							// Items at the same depth can be drawn at the same time
							float depth = firstEntry[h]._depth;
							for (drawn = 1; drawn < left; drawn++)
							{
								if (firstEntry[h + drawn]._depth != depth)
								{
									break;
								}
							}

							_device->GetContext()->DrawIndexed((UINT)drawn * 3, 0, (int)h * 3);
						}
					}
				}
			}
			break;
		}
	}

	_alphaSort.Clear();
	_alphaLines.Clear();
	_alphaTriangles.Clear();
}

void Renderer2d::ClearTextureBuckets()
{
	for (TextureBucket *bucket : _textureBuckets)
	{
		DeleteTextureBucket(bucket);
	}

	_textureBuckets.Clear();
	_textureToBuckets.Clear();

	ff::ZeroObject(_lastBuckets);
}

void Renderer2d::DeleteSpriteArrays()
{
	ClearTextureBuckets();

	for (size_t i = 0; i < _freeSprites.Size(); i++)
	{
		delete _freeSprites[i];
	}

	for (size_t i = 0; i < _freeMultiSprites.Size(); i++)
	{
		delete _freeMultiSprites[i];
	}

	_freeSprites.Clear();
	_freeMultiSprites.Clear();
}

void Renderer2d::CacheTextureBucket(TextureBucket *pBucket)
{
	for (size_t i = 1; i < _countof(_lastBuckets); i++)
	{
		_lastBuckets[i] = _lastBuckets[i - 1];
	}

	_lastBuckets[0] = pBucket;
}

TextureBucket *Renderer2d::GetTextureBucket(const ff::ComPtr<ff::IGraphTexture> &texture, size_t &index)
{
	// Check the MRU cache first

	for (size_t i = 0; i < _countof(_lastBuckets); i++)
	{
		if (_lastBuckets[i] && _lastBuckets[i]->IsSupported(texture, index))
		{
			return _lastBuckets[i];
		}
	}

	// Check the texture-to-bucket map

	ff::BucketIter iter = _textureToBuckets.Get(texture);

	if (iter == ff::INVALID_ITER)
	{
		// Can't reuse a bucket, make a new one

		TextureBucket *pBucket = NewTextureBucket();
		pBucket->AddSupport(texture, index);

		iter = _textureToBuckets.Insert(texture, pBucket);
	}

	TextureBucket *pBucket = _textureToBuckets.ValueAt(iter);
	CacheTextureBucket(pBucket);
	verify(pBucket->IsSupported(texture, index));

	return pBucket;
}

TextureBucket *Renderer2d::GetTextureBucket(const MultiSpriteTextures &mst, size_t indexes[MAX_MULTI_SPRITE_TEXTURES])
{
	// Check the MRU cache first

	for (size_t i = 0; i < _countof(_lastBuckets); i++)
	{
		if (_lastBuckets[i] && _lastBuckets[i]->IsSupported(mst, indexes))
		{
			return _lastBuckets[i];
		}
	}

	// Find a supported bucket for the list of textures

	for (size_t i = 0; i < mst.Size(); i++)
	{
		ff::ComPtr<ff::IGraphTexture> texture = mst[i];

		for (ff::BucketIter iter = _textureToBuckets.Get(texture); iter != ff::INVALID_ITER; iter = _textureToBuckets.GetNext(iter))
		{
			TextureBucket *pBucket = _textureToBuckets.ValueAt(iter);

			if (i == 0 && pBucket->IsSupported(mst, indexes))
			{
				CacheTextureBucket(pBucket);
				return pBucket;
			}

			if (pBucket->AddSupport(mst, indexes))
			{
				MapTexturesToBucket(mst, pBucket);
				CacheTextureBucket(pBucket);
				return pBucket;
			}
		}
	}

	// No buckets can support the textures, make a new one
	{
		TextureBucket *pBucket = NewTextureBucket();
		pBucket->AddSupport(mst, indexes);

		MapTexturesToBucket(mst, pBucket);
		CacheTextureBucket(pBucket);

		return pBucket;
	}
}

void Renderer2d::MapTexturesToBucket(const MultiSpriteTextures &mst, TextureBucket *pBucket)
{
	// Make sure that each texture in "mst" maps to "pBucket"

	ff::ComPtr<ff::IGraphTexture> pPrevTexture;

	for (size_t i = 0; i < mst.Size(); i++)
	{
		ff::ComPtr<ff::IGraphTexture> texture = mst[i];

		if (pPrevTexture != texture)
		{
			pPrevTexture = texture;

			bool bFound = false;

			for (ff::BucketIter iter = _textureToBuckets.Get(texture); iter != ff::INVALID_ITER; iter = _textureToBuckets.GetNext(iter))
			{
				if (_textureToBuckets.ValueAt(iter) == pBucket)
				{
					bFound = true;
					break;
				}
			}

			if (!bFound)
			{
				_textureToBuckets.Insert(texture, pBucket);
			}
		}
	}
}

void Renderer2d::CombineTextureBuckets()
{
	_textureToBuckets.Clear();

	for (size_t i = 0; i + 1 < _textureBuckets.Size(); i++)
	{
		TextureBucket *bucket = _textureBuckets[i];
		size_t count = bucket->_textures.Size();
		size_t unused = MAX_BUCKET_TEXTURES - count;

		// Look for another bucket to fill the unused textures slots

		for (size_t h = i + 1; unused && h < _textureBuckets.Size();)
		{
			TextureBucket *otherBucket = _textureBuckets[h];

			if (otherBucket->_textures.Size() > unused)
			{
				h++;
				continue;
			}

			// Fix up the texture index for the vertexes before copying them over
			DWORD offset = (DWORD)count;

			std::array<ff::SpriteVertexes *, 2> otherSprites =
			{
				otherBucket->_opaqueSprites,
				otherBucket->_alphaSprites
			};

			std::array<ff::MultiSpriteVertexes *, 2> otherMultiSprites =
			{
				otherBucket->_opaqueMultiSprites,
				otherBucket->_alphaMultiSprites
			};

			for (ff::SpriteVertexes *curSprites : otherSprites)
			{
				if (curSprites->Size())
				{
					for (ff::SpriteVertex &sv : *curSprites)
					{
						if (sv.ntex != ff::INVALID_DWORD)
						{
							sv.ntex += offset;
						}
					}

					ff::SpriteVertexes *newSprites = (curSprites == otherSprites[0])
						? bucket->_opaqueSprites
						: bucket->_alphaSprites;
						
					newSprites->Push(curSprites->Data(), curSprites->Size());
				}
			}

			for (ff::MultiSpriteVertexes *curSprites : otherMultiSprites)
			{
				if (curSprites->Size())
				{
					for (ff::MultiSpriteVertex &msv : *curSprites)
					{
						if (msv.ntex[0] != ff::INVALID_DWORD) msv.ntex[0] += offset;
						if (msv.ntex[1] != ff::INVALID_DWORD) msv.ntex[1] += offset;
						if (msv.ntex[2] != ff::INVALID_DWORD) msv.ntex[2] += offset;
						if (msv.ntex[3] != ff::INVALID_DWORD) msv.ntex[3] += offset;
					}

					ff::MultiSpriteVertexes *newSprites = (curSprites == otherMultiSprites[0])
						? bucket->_opaqueMultiSprites
						: bucket->_alphaMultiSprites;

					newSprites->Push(curSprites->Data(), curSprites->Size());
				}
			}

			bucket->_textures.Push(otherBucket->_textures.Data(), otherBucket->_textures.Size());
			count = bucket->_textures.Size();
			unused = MAX_BUCKET_TEXTURES - count;

			DeleteTextureBucket(otherBucket);
			_textureBuckets.Delete(h);
		}
	}
}

TextureBucket *Renderer2d::NewTextureBucket()
{
	TextureBucket *pObj = _textureBucketPool.New();
	_textureBuckets.Push(pObj);

	pObj->_opaqueSprites = NewSpriteVertexes();
	pObj->_alphaSprites = NewSpriteVertexes();
	pObj->_opaqueMultiSprites = NewMultiSpriteVertexes();
	pObj->_alphaMultiSprites = NewMultiSpriteVertexes();

	return pObj;
}

ff::SpriteVertexes *Renderer2d::NewSpriteVertexes()
{
	return _freeSprites.Size()
		? _freeSprites.Pop()
		: new ff::SpriteVertexes();
}

ff::MultiSpriteVertexes *Renderer2d::NewMultiSpriteVertexes()
{
	return _freeMultiSprites.Size()
		? _freeMultiSprites.Pop()
		: new ff::MultiSpriteVertexes();
}

void Renderer2d::DeleteTextureBucket(TextureBucket *pObj)
{
	if (pObj)
	{
		DeleteSpriteVertexes(pObj->_opaqueSprites);
		DeleteSpriteVertexes(pObj->_alphaSprites);
		DeleteMultiSpriteVertexes(pObj->_opaqueMultiSprites);
		DeleteMultiSpriteVertexes(pObj->_alphaMultiSprites);

		_textureBucketPool.Delete(pObj);
	}
}

void Renderer2d::DeleteSpriteVertexes(ff::SpriteVertexes *pObj)
{
	if (pObj)
	{
		pObj->Clear();
		_freeSprites.Push(pObj);
	}
}

void Renderer2d::DeleteMultiSpriteVertexes(ff::MultiSpriteVertexes *pObj)
{
	if (pObj)
	{
		pObj->Clear();
		_freeMultiSprites.Push(pObj);
	}
}
