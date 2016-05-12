#pragma once

namespace ff
{
	class IGraphDevice;

	class __declspec(uuid("d29743fc-f64f-4f7c-b011-1bd6e2a5a60e")) __declspec(novtable)
		IGraphicFactory : public IUnknown
	{
	public:
		virtual IDXGIFactoryX *GetDXGI() = 0;
		virtual ID2D1FactoryX *GetFactory2d() = 0;

		virtual bool CreateDevice(IDXGIAdapterX *pCard, IGraphDevice **device) = 0;
		virtual bool CreateSoftwareDevice(IGraphDevice **device) = 0;

		virtual size_t GetDeviceCount() const = 0;
		virtual IGraphDevice *GetDevice(size_t nIndex) const = 0;

		// Helper functions for using DXGI:
		virtual Vector<ComPtr<IDXGIAdapterX>> GetAdapters() = 0;
		virtual Vector<ComPtr<IDXGIOutputX>> GetOutputs(IDXGIAdapterX *pCard) = 0;

		virtual void AddChild(IGraphDevice *child) = 0;
		virtual void RemoveChild(IGraphDevice *child) = 0;

#if !METRO_APP
		virtual bool GetAdapterForWindow(HWND hwnd, IDXGIAdapterX **ppCard, IDXGIOutputX **ppOutput) = 0;
		virtual bool GetAdapterForMonitor(HMONITOR hMonitor, IDXGIAdapterX **ppCard, IDXGIOutputX **ppOutput) = 0;
#endif
	};

	UTIL_API bool GetParentDXGI(IUnknown *pObject, REFGUID iid, void **ppParent);
	bool CreateGraphicFactory(IGraphicFactory **ppObj);
}
