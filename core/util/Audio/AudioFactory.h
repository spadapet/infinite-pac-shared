#pragma once

namespace ff
{
	class IAudioDevice;

	class __declspec(uuid("90cc78de-7832-436f-8325-d3d2e2ee4330")) __declspec(novtable)
		IAudioFactory : public IUnknown
	{
	public:
		virtual IXAudio2 *GetAudio() = 0;

		virtual bool CreateDefaultDevice(IAudioDevice **device) = 0;
		virtual bool CreateDevice(StringRef deviceId, size_t channels, size_t sampleRate, IAudioDevice **device) = 0;

		virtual size_t GetDeviceCount() const = 0;
		virtual IAudioDevice *GetDevice(size_t nIndex) const = 0;

		virtual void AddChild(IAudioDevice *child) = 0;
		virtual void RemoveChild(IAudioDevice *child) = 0;
	};

	bool CreateAudioFactory(IAudioFactory **obj);
}
