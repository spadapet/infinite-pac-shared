#pragma once

namespace ff
{
	class BufferCache;
	class GraphStateCache;
	class IGraphDeviceChild;

	class __declspec(uuid("1b26d121-cda5-4705-ae3d-4815b4a4115b")) __declspec(novtable)
		IGraphDevice : public IUnknown
	{
	public:
		virtual bool Reset() = 0;
		virtual bool ResetIfNeeded() = 0;
		virtual bool IsSoftware() const = 0;

		virtual ID3D11DeviceX *Get3d() = 0;
		virtual ID2D1DeviceX *Get2d() = 0;
		virtual IDXGIDeviceX *GetDXGI() = 0;
		virtual ID3D11DeviceContextX *GetContext() = 0;
		virtual IDXGIAdapterX *GetAdapter() = 0;
		virtual D3D_FEATURE_LEVEL GetFeatureLevel() const = 0;

		virtual BufferCache &GetVertexBuffers() = 0;
		virtual BufferCache &GetIndexBuffers() = 0;
		virtual GraphStateCache &GetStateCache() = 0;

		virtual void AddChild(IGraphDeviceChild *child) = 0;
		virtual void RemoveChild(IGraphDeviceChild *child) = 0;
	};
}
