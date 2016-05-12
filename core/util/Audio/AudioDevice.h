#pragma once

namespace ff
{
	class IAudioDeviceChild;
	class IAudioPlaying;
	class IThreadDispatch;

	enum class AudioVoiceType
	{
		MASTER,
		EFFECTS,
		MUSIC,
	};

	class __declspec(uuid("bd29d94d-5c4d-4f97-8856-9789d5c4e02b")) __declspec(novtable)
		IAudioDevice : public IUnknown
	{
	public:
		virtual bool IsValid() const = 0;
		virtual void Destroy() = 0;
		virtual bool Reset() = 0;

		virtual void Stop() = 0;
		virtual void Start() = 0;

		virtual float GetVolume(AudioVoiceType type) const = 0;
		virtual void SetVolume(AudioVoiceType type, float volume) = 0;

		virtual void AdvanceEffects() = 0;
		virtual void StopEffects() = 0;
		virtual void PauseEffects() = 0;
		virtual void ResumeEffects() = 0;

		virtual IXAudio2 *GetAudio() const = 0;
		virtual IXAudio2Voice *GetVoice(AudioVoiceType type) const = 0;

		virtual void AddChild(IAudioDeviceChild *child) = 0;
		virtual void RemoveChild(IAudioDeviceChild *child) = 0;
		virtual void AddPlaying(IAudioPlaying *child) = 0;
		virtual void RemovePlaying(IAudioPlaying *child) = 0;
	};
}
