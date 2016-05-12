#pragma once

namespace ff
{
	class IGraphDevice;

	class __declspec(uuid("7cc18924-5269-476f-b863-ef7c0f2f784a")) __declspec(novtable)
		IGraphDeviceChild : public IUnknown
	{
	public:
		virtual IGraphDevice *GetDevice() const = 0;
		virtual bool Reset() = 0;
	};
}
