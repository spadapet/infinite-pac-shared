#include "pch.h"
#include "COM/ComAlloc.h"
#include "Data/Data.h"
#include "Data/DataWriterReader.h"
#include "Graph/2D/2dEffect.h"
#include "Graph/2D/2dRenderer.h"
#include "Graph/GraphDevice.h"
#include "Graph/GraphTexture.h"
#include "Graph/Shader/GraphShader.h"
#include "Graph/State/GraphState.h"
#include "Graph/State/GraphStateCache.h"
#include "Graph/VertexFormat.h"
#include "Module/Module.h"
#include "Module/ModuleFactory.h"
#include "Resource/ResourceValue.h"

static const float MAX_DISCARD_ALPHA = 1.0f / 128.0f;

namespace ff
{
	struct FrameConstantsCore
	{
		DirectX::XMFLOAT4X4 world;
		DirectX::XMFLOAT4X4 proj;
		float zoffset;
		float maxDiscardAlpha;
		float minOutputAlpha;
		float padding;
	};

	struct FrameConstants : FrameConstantsCore
	{
		FrameConstants()
		{
			ZeroObject(*this);
			DirectX::XMStoreFloat4x4(&world, DirectX::XMMatrixIdentity());
			DirectX::XMStoreFloat4x4(&proj, DirectX::XMMatrixIdentity());
			maxDiscardAlpha = MAX_DISCARD_ALPHA;
		}
	};

	class __declspec(uuid("10005dea-f4cf-4cae-801d-5b058b6d4c3a"))
		Default2dEffect : public ComBase, public I2dEffect
	{
	public:
		DECLARE_HEADER(Default2dEffect);

		virtual HRESULT _Construct(IUnknown *unkOuter) override;
		bool Init();

		// IGraphDeviceChild
		virtual IGraphDevice *GetDevice() const override;
		virtual bool Reset() override;

		// I2dEffect
		virtual bool IsValid() override;
		virtual bool OnBeginRender(I2dRenderer *pRender) override;
		virtual void OnEndRender(I2dRenderer *pRender) override;
		virtual void OnMatrixChanging(I2dRenderer *pRender, MatrixType type) override;
		virtual void OnMatrixChanged(I2dRenderer *pRender, MatrixType type) override;
		virtual void ApplyTextures(I2dRenderer *pRender, IGraphTexture **ppTextures, size_t nTextures) override;
		virtual bool Apply(I2dRenderer *pRender, DrawType2d type, ID3D11Buffer *pVertexes, ID3D11Buffer *pIndexes, float zOffset) override;
		virtual void PushDrawType(DrawType2d typeFlags) override;
		virtual void PopDrawType() override;

	private:
		void Destroy();
		bool LoadResources(DrawType2d type);

		const GraphState *GetInfo(DrawType2d type);

		ComPtr<IGraphDevice> _device;
		ComPtr<IGraphTexture> _emptyTexture;
		Map<DrawType2d, GraphState> _typeInfo;
		Vector<DrawType2d> _typeFlags;
		I2dRenderer *_currentRenderer;

		Vector<TypedResource<IGraphShader>> _resources;
		bool _loadedResources;

		ComPtr<ID3D11VertexShader> _vsLine;
		ComPtr<ID3D11VertexShader> _vsSprite;
		ComPtr<ID3D11VertexShader> _vsMultiSprite;

		ComPtr<ID3D11PixelShader> _psLine;
		ComPtr<ID3D11PixelShader> _psSprite;
		ComPtr<ID3D11PixelShader> _psSpriteAlpha;
		ComPtr<ID3D11PixelShader> _psMultiSprite;

		ComPtr<ID3D11InputLayout> _layoutLine;
		ComPtr<ID3D11InputLayout> _layoutSprite;
		ComPtr<ID3D11InputLayout> _layoutMultiSprite;

		FrameConstants _frameConstants;
		FrameConstants _prevFrameConstants;
		ComPtr<ID3D11Buffer> _frameConstantsBuffer;
	};
}

BEGIN_INTERFACES(ff::Default2dEffect)
	HAS_INTERFACE(ff::I2dEffect)
	HAS_INTERFACE(ff::IGraphDeviceChild)
END_INTERFACES()

static ff::ModuleStartup Register([](ff::Module &module)
{
	static ff::StaticString name(L"Default2dEffect");
	module.RegisterClassT<ff::Default2dEffect>(name);
});

bool ff::CreateDefault2dEffect(IGraphDevice *pDevice, I2dEffect **ppEffect)
{
	assertRetVal(pDevice && ppEffect, false);
	*ppEffect = nullptr;

	ComPtr<Default2dEffect> pEffect;
	assertHrRetVal(ComAllocator<Default2dEffect>::CreateInstance(pDevice, &pEffect), false);
	assertRetVal(pEffect->Init(), false);

	*ppEffect = pEffect.Detach();
	return true;
}

ff::Default2dEffect::Default2dEffect()
	: _loadedResources(false)
{
}

ff::Default2dEffect::~Default2dEffect()
{
	Destroy();

	if (_device)
	{
		_device->RemoveChild(this);
	}
}

HRESULT ff::Default2dEffect::_Construct(IUnknown *unkOuter)
{
	assertRetVal(_device.QueryFrom(unkOuter), E_FAIL);
	_device->AddChild(this);
	
	return __super::_Construct(unkOuter);
}

bool ff::Default2dEffect::Init()
{
	assertRetVal(_device && _device->Get3d() && _device->GetContext(), false);

	std::array<String, 7> resNames =
	{
		String(L"LineArt.vs_4_0"),
		String(L"LineArt.ps_4_0"),
		String(L"Sprite.vs_4_0"),
		String(L"Sprite.ps_4_0"),
		String(L"MultiSprite.vs_4_0"),
		String(L"MultiSprite.ps_4_0"),
		String(L"SpriteAlpha.ps_4_0"),
	};

	for (StringRef resName : resNames)
	{
		_resources.Push(TypedResource<IGraphShader>(GetThisModule().GetResources(), resName));
	}

	D3D11_SUBRESOURCE_DATA frameConstantsData;
	frameConstantsData.pSysMem = &_frameConstants;
	frameConstantsData.SysMemPitch = sizeof(FrameConstantsCore);
	frameConstantsData.SysMemSlicePitch = 0;

	CD3D11_BUFFER_DESC frameConstantsDesc(sizeof(FrameConstantsCore), D3D11_BIND_CONSTANT_BUFFER);
	assertHrRetVal(_device->Get3d()->CreateBuffer(&frameConstantsDesc, &frameConstantsData, &_frameConstantsBuffer), false);

	return true;
}

void ff::Default2dEffect::Destroy()
{
	_loadedResources = false;
	_frameConstants = FrameConstants();
	_prevFrameConstants = FrameConstants();
	_resources.Clear();
	_typeInfo.Clear();

	_emptyTexture = nullptr;
	_vsLine = nullptr;
	_vsSprite = nullptr;
	_vsMultiSprite = nullptr;
	_psLine = nullptr;
	_psSprite = nullptr;
	_psMultiSprite = nullptr;
	_psSpriteAlpha = nullptr;
	_layoutLine = nullptr;
	_layoutSprite = nullptr;
	_layoutMultiSprite = nullptr;
	_frameConstantsBuffer = nullptr;
}

static bool LoadGraphShaders(
	ff::IGraphDevice *device,
	ff::IGraphShader *vertexShaderData,
	ID3D11VertexShader **vertexShader,
	ff::IGraphShader *pixelShaderData,
	ID3D11PixelShader **pixelShader,
	ff::VertexType vertexType,
	ID3D11InputLayout **layout)
{
	if (vertexShaderData && vertexShader)
	{
		assertRetVal(vertexShaderData->GetData(), false);
		assertHrRetVal(device->Get3d()->CreateVertexShader(
			vertexShaderData->GetData()->GetMem(),
			vertexShaderData->GetData()->GetSize(),
			nullptr,
			vertexShader), false);

		if (vertexType != ff::VERTEX_TYPE_UNKNOWN && layout)
		{
			assertRetVal(CreateVertexLayout(
				device,
				vertexShaderData->GetData()->GetMem(),
				vertexShaderData->GetData()->GetSize(),
				vertexType, layout), false);
		}
	}

	if (pixelShaderData && pixelShader)
	{
		assertRetVal(pixelShaderData->GetData(), false);
		assertHrRetVal(device->Get3d()->CreatePixelShader(
			pixelShaderData->GetData()->GetMem(),
			pixelShaderData->GetData()->GetSize(),
			nullptr,
			pixelShader), false);
	}

	return true;
}

bool ff::Default2dEffect::LoadResources(DrawType2d type)
{
	assert(_resources.Size() == 7);

	if (!_loadedResources)
	{
		// Force all shaders to finish loading
		for (auto &res : _resources)
		{
			assertRetVal(res.GetObject() || res.Flush(), false);
		}

		_loadedResources = true;
	}

	switch (type & DRAW_TYPE_BITS)
	{
	case DRAW_TYPE_SPRITE:
		if (!_vsSprite)
		{
			assertRetVal(LoadGraphShaders(_device,
				_resources[2].GetObject(), &_vsSprite,
				_resources[3].GetObject(), &_psSprite,
				VERTEX_TYPE_SPRITE, &_layoutSprite), false);
		}

		if ((type & DRAW_TEXTURE_ALPHA) && !_psSpriteAlpha)
		{
			assertRetVal(LoadGraphShaders(_device,
				nullptr, nullptr,
				_resources[6].GetObject(), &_psSpriteAlpha,
				VERTEX_TYPE_UNKNOWN, nullptr), false);
		}
		break;

	case DRAW_TYPE_MULTI_SPRITE:
		if (!_vsMultiSprite)
		{
			assertRetVal(LoadGraphShaders(_device,
				_resources[4].GetObject(), &_vsMultiSprite,
				_resources[5].GetObject(), &_psMultiSprite,
				VERTEX_TYPE_MULTI_SPRITE, &_layoutMultiSprite), false);
		}
		break;

	case DRAW_TYPE_TRIANGLES:
	case DRAW_TYPE_LINES:
	case DRAW_TYPE_POINTS:
		if (!_vsLine)
		{
			assertRetVal(LoadGraphShaders(_device,
				_resources[0].GetObject(), &_vsLine,
				_resources[1].GetObject(), &_psLine,
				VERTEX_TYPE_LINE_ART, &_layoutLine), false);
		}
		break;
	}

	return true;
}

ff::IGraphDevice *ff::Default2dEffect::GetDevice() const
{
	return _device;
}

bool ff::Default2dEffect::Reset()
{
	Destroy();
	assertRetVal(Init(), false);
	return true;
}

bool ff::Default2dEffect::IsValid()
{
	return _resources.Size() == 7;
}

bool ff::Default2dEffect::OnBeginRender(I2dRenderer *pRender)
{
	assertRetVal(IsValid() && pRender, false);

	OnMatrixChanged(pRender, MATRIX_WORLD);
	OnMatrixChanged(pRender, MATRIX_PROJECTION);

	_currentRenderer = pRender;

	return true;
}

void ff::Default2dEffect::OnEndRender(I2dRenderer *pRender)
{
	assert(pRender == _currentRenderer);
	_currentRenderer = nullptr;

	// Gets rid of a harmless(?) warning when a render target was used as pixel shader input
	ID3D11ShaderResourceView *pRes = nullptr;
	pRender->GetDevice()->GetContext()->PSSetShaderResources(0, 1, &pRes);
}

void ff::Default2dEffect::OnMatrixChanging(I2dRenderer *pRender, MatrixType type)
{
}

void ff::Default2dEffect::OnMatrixChanged(I2dRenderer *pRender, MatrixType type)
{
	switch (type)
	{
	case MATRIX_PROJECTION:
		DirectX::XMStoreFloat4x4(&_frameConstants.proj,
			DirectX::XMMatrixTranspose(
				DirectX::XMLoadFloat4x4(&pRender->GetMatrix(MATRIX_PROJECTION))));
		break;

	case MATRIX_WORLD:
		DirectX::XMStoreFloat4x4(&_frameConstants.world,
			DirectX::XMMatrixTranspose(
				DirectX::XMLoadFloat4x4(&pRender->GetMatrix(MATRIX_WORLD))));
		break;
	}
}

void ff::Default2dEffect::ApplyTextures(I2dRenderer *pRender, IGraphTexture **ppTextures, size_t nTextures)
{
	if (!_emptyTexture)
	{
		// Use this texture in place of textures that aren't loaded yet
		CreateGraphTexture(_device, PointInt(1, 1), DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 0, &_emptyTexture);
	}

	ID3D11ShaderResourceView *resources[8] = { 0 };

	for (size_t i = 0; i < nTextures; i++)
	{
		resources[i] = ppTextures[i] ? ppTextures[i]->GetShaderResource() : nullptr;

		if (!resources[i] && _emptyTexture)
		{
			resources[i] = _emptyTexture->GetShaderResource();
		}
	}

#ifdef _DEBUG
	for (size_t i = nTextures; i < _countof(resources); i++)
	{
		// Avoid harmless debug warnings from Direct3D by selecting an empty
		// texture into the unused slots.

		if (!resources[i] && _emptyTexture)
		{
			resources[i] = _emptyTexture->GetShaderResource();
		}
	}
#endif

	UINT nResources = _countof(resources);
	pRender->GetDevice()->GetContext()->PSSetShaderResources(0, nResources, resources);
}

bool ff::Default2dEffect::Apply(
	I2dRenderer *pRender,
	DrawType2d type,
	ID3D11Buffer *pVertexes,
	ID3D11Buffer *pIndexes,
	float zOffset)
{
	assertRetVal(LoadResources(type), false);

	if (_typeFlags.Size())
	{
		type = (DrawType2d)((type & DRAW_TYPE_BITS) | _typeFlags.GetLast());
	}

	_frameConstants.zoffset = zOffset;

	switch (type & DRAW_TEXTURE_BITS)
	{
	case DRAW_TEXTURE_RGB:
		_frameConstants.maxDiscardAlpha = 0.5;
		_frameConstants.minOutputAlpha = 1;
		break;

	default:
		_frameConstants.maxDiscardAlpha = MAX_DISCARD_ALPHA;
		_frameConstants.minOutputAlpha = 0;
		break;
	}

	if (memcmp(&_frameConstants, &_prevFrameConstants, sizeof(FrameConstantsCore)))
	{
		_prevFrameConstants = _frameConstants;
		_device->GetContext()->UpdateSubresource(_frameConstantsBuffer, 0, nullptr, &_frameConstants, 0, 0);
	}

	const GraphState *state = GetInfo(type);
	assertRetVal(state && state->Apply(_device, pVertexes, pIndexes), false);

	return true;
}

const ff::GraphState *ff::Default2dEffect::GetInfo(DrawType2d type)
{
	BucketIter iter = _typeInfo.Get(type);

	if (iter == INVALID_ITER)
	{
		GraphState info;
		info._vertexConstants.Push(_frameConstantsBuffer);
		info._pixelConstants.Push(_frameConstantsBuffer);

		switch (type & DRAW_TYPE_BITS)
		{
		case DRAW_TYPE_SPRITE:
			info._vertexType = VERTEX_TYPE_SPRITE;
			info._stride = (UINT)GetVertexStride(info._vertexType);
			info._topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			info._layout = _layoutSprite;
			info._vertex = _vsSprite;
			info._pixel = (type & DRAW_TEXTURE_ALPHA) ? _psSpriteAlpha : _psSprite;
			break;

		case DRAW_TYPE_MULTI_SPRITE:
			info._vertexType = VERTEX_TYPE_MULTI_SPRITE;
			info._stride = (UINT)GetVertexStride(info._vertexType);
			info._topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			info._layout = _layoutMultiSprite;
			info._vertex = _vsMultiSprite;
			info._pixel = _psMultiSprite;
			break;

		case DRAW_TYPE_TRIANGLES:
			info._vertexType = VERTEX_TYPE_LINE_ART;
			info._stride = (UINT)GetVertexStride(info._vertexType);
			info._topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			info._layout = _layoutLine;
			info._vertex = _vsLine;
			info._pixel = _psLine;
			break;

		case DRAW_TYPE_LINES:
			info._vertexType = VERTEX_TYPE_LINE_ART;
			info._stride = (UINT)GetVertexStride(info._vertexType);
			info._topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
			info._layout = _layoutLine;
			info._vertex = _vsLine;
			info._pixel = _psLine;
			break;

		case DRAW_TYPE_POINTS:
			info._vertexType = VERTEX_TYPE_LINE_ART;
			info._stride = (UINT)GetVertexStride(info._vertexType);
			info._topology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
			info._layout = _layoutLine;
			info._vertex = _vsLine;
			info._pixel = _psLine;
			break;

		default:
			assertRetVal(false, nullptr);
		}

		assertRetVal(info._layout && info._vertex && info._pixel, nullptr);

		D3D11_BLEND_DESC blendDesc;
		D3D11_DEPTH_STENCIL_DESC depthDesc;
		D3D11_RASTERIZER_DESC rasterDesc;
		D3D11_SAMPLER_DESC samplerDesc;

		_device->GetStateCache().GetDefault(blendDesc);
		_device->GetStateCache().GetDefault(depthDesc);
		_device->GetStateCache().GetDefault(rasterDesc);
		_device->GetStateCache().GetDefault(samplerDesc);

		// Blend
		{
			info._sampleMask = 0xFFFFFFFF;

			// newColor = (srcColor * SrcBlend) BlendOp (destColor * DestBlend)
			// newAlpha = (srcAlpha * SrcBlendAlpha) BlendOpAlpha (destAlpha * DestBlendAlpha)

			switch (type & DRAW_BLEND_BITS)
			{
			case 0:
			case DRAW_BLEND_ALPHA:
				blendDesc.RenderTarget[0].BlendEnable = TRUE;
				blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
				blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
				blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
				blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
				blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
				break;

			case DRAW_BLEND_ALPHA_MAX:
				blendDesc.RenderTarget[0].BlendEnable = TRUE;
				blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
				blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
				blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
				blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
				blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_MAX;
				break;

			case DRAW_BLEND_ADD:
				blendDesc.RenderTarget[0].BlendEnable = TRUE;
				blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
				blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
				blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
				blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
				blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
				break;

			case DRAW_BLEND_SUB:
				blendDesc.RenderTarget[0].BlendEnable = TRUE;
				blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
				blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
				blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_REV_SUBTRACT;
				blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
				blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
				blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_REV_SUBTRACT;
				break;

			case DRAW_BLEND_MULTIPLY:
				blendDesc.RenderTarget[0].BlendEnable = TRUE;
				blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ZERO;
				blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_COLOR;
				blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
				blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_SRC_ALPHA;
				blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
				break;

			case DRAW_BLEND_INV_MUL:
				blendDesc.RenderTarget[0].BlendEnable = TRUE;
				blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ZERO;
				blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_COLOR;
				blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
				blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
				blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
				break;

			case DRAW_BLEND_COPY_ALPHA:
				blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALPHA;
				break;

			case DRAW_BLEND_COPY_ALL:
				// default is good
				break;

			case DRAW_BLEND_COPY_PMA:
				blendDesc.RenderTarget[0].BlendEnable = TRUE;
				blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
				blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
				blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
				blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
				blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
				break;

			default:
				assertRetVal(false, false);
			}
		}

		// Depth/Stencil
		{
			info._stencil = 1;

			switch (type & DRAW_DEPTH_BITS)
			{
			case 0:
			case DRAW_DEPTH_ENABLE:
				depthDesc.DepthEnable = TRUE;
				depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
				break;

			case DRAW_DEPTH_DISABLE:
				depthDesc.DepthEnable = FALSE;
				depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
				break;

			default:
				assertRetVal(false, false);
			}

			switch (type & DRAW_STENCIL_BITS)
			{
			case 0:
			case DRAW_STENCIL_NONE:
				break;

			case DRAW_STENCIL_WRITE:
				depthDesc.StencilEnable = TRUE;
				depthDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
				depthDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
				depthDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
				depthDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
				depthDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
				depthDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
				depthDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
				depthDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
				break;

			case DRAW_STENCIL_IS_SET:
				depthDesc.StencilEnable = TRUE;
				depthDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
				depthDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
				depthDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
				depthDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
				depthDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
				depthDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
				depthDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
				depthDesc.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;
				break;

			case DRAW_STENCIL_NOT_SET:
				depthDesc.StencilEnable = TRUE;
				depthDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
				depthDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
				depthDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
				depthDesc.FrontFace.StencilFunc = D3D11_COMPARISON_NOT_EQUAL;
				depthDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
				depthDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
				depthDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
				depthDesc.BackFace.StencilFunc = D3D11_COMPARISON_NOT_EQUAL;
				break;

			default:
				assertRetVal(false, nullptr);
			}
		}

		// Raster
		{
			rasterDesc.CullMode = D3D11_CULL_NONE;
			rasterDesc.MultisampleEnable = FALSE;
			rasterDesc.AntialiasedLineEnable = FALSE;
		}

		// Sampler
		{
			if ((type & DRAW_SAMPLE_MIN_LINEAR) && (type & DRAW_SAMPLE_MAG_LINEAR))
			{
				samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			}
			else if (type & DRAW_SAMPLE_MAG_LINEAR)
			{
				samplerDesc.Filter = D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
			}
			else if (type & DRAW_SAMPLE_MIN_LINEAR)
			{
				samplerDesc.Filter = D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
			}
			else
			{
				samplerDesc.Filter = D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
			}

			if (type & DRAW_SAMPLE_WRAP)
			{
				samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
				samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
				samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
			}
		}

		info._blend = _device->GetStateCache().GetBlendState(blendDesc);
		info._depth = _device->GetStateCache().GetDepthStencilState(depthDesc);
		info._raster = _device->GetStateCache().GetRasterizerState(rasterDesc);
		info._samplers.Push(_device->GetStateCache().GetSamplerState(samplerDesc));

		assertRetVal(
			info._blend &&
			info._depth &&
			info._raster &&
			info._samplers[0], nullptr);

		iter = _typeInfo.Insert(type, info);
	}

	return &_typeInfo.ValueAt(iter);
}

void ff::Default2dEffect::PushDrawType(DrawType2d typeFlags)
{
	assert(!(typeFlags & DRAW_TYPE_BITS));

	DWORD newType = typeFlags & ~DRAW_TYPE_BITS;
	DWORD prevType = _typeFlags.Size() ? _typeFlags.GetLast() : 0;

	if (typeFlags & DRAW_STENCIL_BITS)
	{
		prevType &= ~DRAW_STENCIL_BITS;
	}

	if (typeFlags & DRAW_SAMPLE_BITS)
	{
		prevType &= ~DRAW_SAMPLE_BITS;
	}

	if (typeFlags & DRAW_DEPTH_BITS)
	{
		prevType &= ~DRAW_DEPTH_BITS;
	}

	if (typeFlags & DRAW_BLEND_BITS)
	{
		prevType &= ~DRAW_BLEND_BITS;
	}

	if (typeFlags & DRAW_TEXTURE_BITS)
	{
		prevType &= ~DRAW_TEXTURE_BITS;
	}

	newType |= prevType;

	_typeFlags.Push((DrawType2d)newType);

	if (_currentRenderer)
	{
		_currentRenderer->Flush();
	}
}

void ff::Default2dEffect::PopDrawType()
{
	if (_currentRenderer)
	{
		_currentRenderer->Flush();
	}

	assertRet(_typeFlags.Size());
	_typeFlags.Pop();
}
