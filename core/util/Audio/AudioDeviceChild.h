#pragma once

namespace ff
{
	class IAudioDevice;

	class __declspec(uuid("3a5f60e4-d257-4dea-b834-8e7a09324f84")) __declspec(novtable)
		IAudioDeviceChild : public IUnknown
	{
	public:
		virtual IAudioDevice *GetDevice() const = 0;
		virtual bool Reset() = 0;
	};
}
