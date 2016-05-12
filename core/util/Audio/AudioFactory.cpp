#include "pch.h"
#include "Audio/AudioDevice.h"
#include "Audio/AudioFactory.h"
#include "COM/ComAlloc.h"
#include "Globals/ProcessGlobals.h"

#if !METRO_APP
#include <SetupAPI.h>
#endif

namespace ff
{
	class __declspec(uuid("b1c52643-fed3-458a-b998-3036c344ced9"))
		AudioFactory : public ComBase, public IAudioFactory
	{
	public:
		DECLARE_HEADER(AudioFactory);

		bool Init();

		// IAudioFactory
		virtual IXAudio2 *GetAudio() override;

		virtual bool CreateDefaultDevice(IAudioDevice **device) override;
		virtual bool CreateDevice(StringRef name, size_t channels, size_t sampleRate, IAudioDevice **device) override;

		virtual size_t GetDeviceCount() const override;
		virtual IAudioDevice *GetDevice(size_t nIndex) const override;

		virtual void AddChild(IAudioDevice *child) override;
		virtual void RemoveChild(IAudioDevice *child) override;

	private:
		Mutex _mutex;
		ComPtr<IXAudio2> _audio;
		Vector<IAudioDevice *> _devices;
		IAudioDevice *_defaultDevice;
		bool _startedMF;
	};

	bool CreateAudioDevice(
		IAudioFactory *factory,
		StringRef name,
		size_t channels,
		size_t sampleRate,
		IAudioDevice **device);
}

BEGIN_INTERFACES(ff::AudioFactory)
	HAS_INTERFACE(ff::IAudioFactory)
END_INTERFACES()

bool ff::CreateAudioFactory(IAudioFactory **obj)
{
	assertRetVal(obj, false);

	ComPtr<AudioFactory, IAudioFactory> myObj;
	assertHrRetVal(ComAllocator<AudioFactory>::CreateInstance(&myObj), false);
	assertRetVal(myObj->Init(), false);

	*obj = myObj.Detach();
	return true;
}

ff::AudioFactory::AudioFactory()
	: _defaultDevice(nullptr)
	, _startedMF(false)
{
}

ff::AudioFactory::~AudioFactory()
{
	assert(_devices.IsEmpty());

	_audio = nullptr;

	if (_startedMF)
	{
		_startedMF = false;
		::MFShutdown();
	}
}

bool ff::AudioFactory::Init()
{
	assertHrRetVal(::MFStartup(MF_VERSION), false);
	_startedMF = true;

	return true;
}

IXAudio2 *ff::AudioFactory::GetAudio()
{
	if (!_audio)
	{
		LockMutex crit(_mutex);

		if (!_audio)
		{
			assertHrRetVal(XAudio2Create(&_audio), nullptr);

			if (GetThisModule().IsDebugBuild())
			{
				XAUDIO2_DEBUG_CONFIGURATION dc;
				ZeroObject(dc);
				dc.TraceMask = XAUDIO2_LOG_ERRORS;
				dc.BreakMask = XAUDIO2_LOG_ERRORS;
				_audio->SetDebugConfiguration(&dc);
			}
		}
	}

	return _audio;
}

bool ff::AudioFactory::CreateDefaultDevice(IAudioDevice **device)
{
	if (_defaultDevice)
	{
		*device = GetAddRef(_defaultDevice);
	}
	else
	{
		assertRetVal(CreateDevice(GetEmptyString(), 0, 0, device), false);
		_defaultDevice = *device;
	}

	return true;
}

bool ff::AudioFactory::CreateDevice(StringRef deviceId, size_t channels, size_t sampleRate, IAudioDevice **device)
{
	assertRetVal(device && GetAudio(), false);
	*device = nullptr;

	ComPtr<IAudioDevice> pDevice;
	assertRetVal(ff::CreateAudioDevice(this, deviceId, channels, sampleRate, &pDevice), false);

	_devices.Push(pDevice);
	*device = pDevice.Detach();
	return true;
}

size_t ff::AudioFactory::GetDeviceCount() const
{
	return _devices.Size();
}

ff::IAudioDevice *ff::AudioFactory::GetDevice(size_t nIndex) const
{
	assertRetVal(nIndex >= 0 && nIndex < _devices.Size(), nullptr);
	return _devices[nIndex];
}

void ff::AudioFactory::AddChild(IAudioDevice *child)
{
	ff::LockMutex crit(_mutex);

	assert(child && _devices.Find(child) == ff::INVALID_SIZE);
	_devices.Push(child);
}

void ff::AudioFactory::RemoveChild(IAudioDevice *child)
{
	ff::LockMutex crit(_mutex);

	verify(_devices.DeleteItem(child));
}
