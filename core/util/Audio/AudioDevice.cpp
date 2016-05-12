#include "pch.h"
#include "Audio/AudioDevice.h"
#include "Audio/AudioDeviceChild.h"
#include "Audio/AudioEffect.h"
#include "Audio/AudioFactory.h"
#include "Audio/AudioPlaying.h"
#include "COM/ComAlloc.h"
#include "Globals/ThreadGlobals.h"

namespace ff
{
	class __declspec(uuid("039ec1dd-9e1a-4d08-9678-7937a0671d9a"))
		AudioDevice : public ComBase, public IAudioDevice
	{
	public:
		DECLARE_HEADER(AudioDevice);

		virtual HRESULT _Construct(IUnknown *unkOuter) override;
		bool Init(StringRef name, size_t channels, size_t sampleRate);

		// IAudioDevice functions
		virtual bool IsValid() const override;
		virtual void Destroy() override;
		virtual bool Reset() override;

		virtual void Stop() override;
		virtual void Start() override;

		virtual float GetVolume(AudioVoiceType type) const override;
		virtual void SetVolume(AudioVoiceType type, float volume) override;

		virtual void AdvanceEffects() override;
		virtual void StopEffects() override;
		virtual void PauseEffects() override;
		virtual void ResumeEffects() override;

		virtual IXAudio2 *GetAudio() const override;
		virtual IXAudio2Voice *GetVoice(AudioVoiceType type) const override;

		virtual void AddChild(IAudioDeviceChild *child) override;
		virtual void RemoveChild(IAudioDeviceChild *child) override;
		virtual void AddPlaying(IAudioPlaying *child) override;
		virtual void RemovePlaying(IAudioPlaying *child) override;

	private:
		Mutex _mutex;
		ComPtr<IAudioFactory> _factory;
		IXAudio2MasteringVoice *_masterVoice;
		IXAudio2SubmixVoice *_effectVoice;
		IXAudio2SubmixVoice *_musicVoice;
		Vector<IAudioDeviceChild *> _children;
		Vector<IAudioPlaying *> _playing;
		Vector<IAudioPlaying *> _paused;
		String _name;
		size_t _channels;
		size_t _sampleRate;
		size_t _advances;
	};

	bool CreateAudioDevice(
		IAudioFactory *factory,
		ff::StringRef name,
		size_t channels,
		size_t sampleRate,
		ff::IAudioDevice **device);
}

BEGIN_INTERFACES(ff::AudioDevice)
	HAS_INTERFACE(ff::IAudioDevice)
END_INTERFACES()

bool ff::CreateAudioDevice(
	IAudioFactory *factory,
	ff::StringRef name,
	size_t channels,
	size_t sampleRate,
	ff::IAudioDevice **device)
{
	assertRetVal(device, false);
	*device = nullptr;

	ff::ComPtr<ff::AudioDevice, ff::IAudioDevice> pDevice;
	assertHrRetVal(ff::ComAllocator<ff::AudioDevice>::CreateInstance(factory, &pDevice), false);
	assertRetVal(pDevice->Init(name, channels, sampleRate), false);

	*device = pDevice.Detach();
	return true;
}

ff::AudioDevice::AudioDevice()
	: _masterVoice(nullptr)
	, _effectVoice(nullptr)
	, _musicVoice(nullptr)
	, _channels(0)
	, _sampleRate(0)
	, _advances(0)
{
}

ff::AudioDevice::~AudioDevice()
{
	assert(!_children.Size());

	Destroy();

	if (_factory)
	{
		_factory->RemoveChild(this);
	}
}

HRESULT ff::AudioDevice::_Construct(IUnknown *unkOuter)
{
	assertRetVal(_factory.QueryFrom(unkOuter), E_INVALIDARG);
	_factory->AddChild(this);

	return __super::_Construct(unkOuter);
}

bool ff::AudioDevice::Init(StringRef name, size_t channels, size_t sampleRate)
{
	assertRetVal(!_masterVoice, false);

	// There might not be an audio card, and we have to deal with that
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
	HRESULT hrMaster = _factory->GetAudio()
		? _factory->GetAudio()->CreateMasteringVoice(
			&_masterVoice,
			(UINT32)channels,
			(UINT32)sampleRate,
			0, // flags
			name.size() ? name.c_str() : nullptr,
			nullptr)
		: E_FAIL;
#else
	HRESULT hrMaster = _factory->GetAudio()
		? _factory->GetAudio()->CreateMasteringVoice(
			&_masterVoice,
			(UINT32)channels,
			(UINT32)sampleRate,
			0, // flags
			0, // index (ignoring the device name, just use the default)
			nullptr)
		: E_FAIL;
#endif

	if (SUCCEEDED(hrMaster))
	{
		XAUDIO2_SEND_DESCRIPTOR masterDescriptor;
		masterDescriptor.Flags = 0;
		masterDescriptor.pOutputVoice = _masterVoice;

		XAUDIO2_VOICE_SENDS masterSend;
		masterSend.SendCount = 1;
		masterSend.pSends = &masterDescriptor;

		XAUDIO2_VOICE_DETAILS details;
		ff::ZeroObject(details);
		_masterVoice->GetVoiceDetails(&details);

		assertHrRetVal(_factory->GetAudio()->CreateSubmixVoice(
			&_effectVoice,
			details.InputChannels,
			details.InputSampleRate,
			0, // flags
			0, // stage
			&masterSend,
			nullptr), false);

		assertHrRetVal(_factory->GetAudio()->CreateSubmixVoice(
			&_musicVoice,
			details.InputChannels,
			details.InputSampleRate,
			0, // flags
			0, // stage
			&masterSend,
			nullptr), false);
	}

	_name = name;
	_channels = channels;
	_sampleRate = sampleRate;

	return FAILED(hrMaster) || IsValid();
}

bool ff::AudioDevice::IsValid() const
{
	return _masterVoice != nullptr;
}

void ff::AudioDevice::Destroy()
{
	for (size_t i = 0; i < _children.Size(); i++)
	{
		_children[i]->Reset();
	}

	Stop();

	if (_effectVoice)
	{
		_effectVoice->DestroyVoice();
		_effectVoice = nullptr;
	}

	if (_musicVoice)
	{
		_musicVoice->DestroyVoice();
		_musicVoice = nullptr;
	}

	if (_masterVoice)
	{
		_masterVoice->DestroyVoice();
		_masterVoice = nullptr;
	}
}

bool ff::AudioDevice::Reset()
{
	Destroy();
	assertRetVal(Init(_name, _channels, _sampleRate), false);

	return true;
}

void ff::AudioDevice::Stop()
{
	if (_factory->GetAudio())
	{
		StopEffects();
		_factory->GetAudio()->StopEngine();
	}
}

void ff::AudioDevice::Start()
{
	if (_factory->GetAudio())
	{
		_factory->GetAudio()->StartEngine();
	}
}

float ff::AudioDevice::GetVolume(AudioVoiceType type) const
{
	float volume = 1;

	if (GetVoice(type))
	{
		GetVoice(type)->GetVolume(&volume);
	}

	return volume;
}

void ff::AudioDevice::SetVolume(AudioVoiceType type, float volume)
{
	if (GetVoice(type))
	{
		volume = std::max<float>(0, volume);
		volume = std::min<float>(1, volume);

		GetVoice(type)->SetVolume(volume);
	}
}

void ff::AudioDevice::AdvanceEffects()
{
	// Check if speakers were plugged in every two seconds
	if (++_advances % 120 == 0 && !IsValid())
	{
		Reset();
	}

	for (size_t i = PreviousSize(_playing.Size()); i != INVALID_SIZE; i = PreviousSize(i))
	{
		// OK to advance paused effects, I guess (change if needed)
		_playing[i]->Advance();
	}
}

void ff::AudioDevice::StopEffects()
{
	for (size_t i = 0; i < _playing.Size(); i++)
	{
		_playing[i]->Stop();
	}
}

void ff::AudioDevice::PauseEffects()
{
	for (size_t i = 0; i < _playing.Size(); i++)
	{
		IAudioPlaying *playing = _playing[i];
		if (_paused.Find(playing) == INVALID_SIZE && !playing->IsPaused())
		{
			_paused.Push(playing);
			playing->Pause();
		}
	}
}

void ff::AudioDevice::ResumeEffects()
{
	for (size_t i = 0; i < _paused.Size(); i++)
	{
		_paused[i]->Resume();
	}

	_paused.Clear();
}

IXAudio2 *ff::AudioDevice::GetAudio() const
{
	return _factory->GetAudio();
}

IXAudio2Voice *ff::AudioDevice::GetVoice(AudioVoiceType type) const
{
	switch (type)
	{
	case AudioVoiceType::MASTER:
		return _masterVoice;

	case AudioVoiceType::EFFECTS:
		return _effectVoice;

	case AudioVoiceType::MUSIC:
		return _musicVoice;

	default:
		assertRetVal(false, _masterVoice);
	}
}

void ff::AudioDevice::AddChild(IAudioDeviceChild *child)
{
	LockMutex crit(_mutex);

	assert(child && _children.Find(child) == ff::INVALID_SIZE);
	_children.Push(child);
}

void ff::AudioDevice::RemoveChild(IAudioDeviceChild *child)
{
	LockMutex crit(_mutex);

	verify(_children.DeleteItem(child));
}

void ff::AudioDevice::AddPlaying(IAudioPlaying *child)
{
	LockMutex crit(_mutex);

	assert(child && _playing.Find(child) == ff::INVALID_SIZE);
	_playing.Push(child);
}

void ff::AudioDevice::RemovePlaying(IAudioPlaying *child)
{
	LockMutex crit(_mutex);

	verify(_playing.DeleteItem(child));
}
