#pragma once

#include "Audio/AudioDeviceChild.h"

namespace ff
{
	class IAudioPlaying;
	class IAudioStream;

	class __declspec(uuid("a2ff3947-62ad-4ec9-94ad-7ea497a889ad")) __declspec(novtable)
		IAudioMusic : public IAudioDeviceChild
	{
	public:
		virtual bool SetStream(IAudioStream *stream, float volume = 1, float freqRatio = 1) = 0;
		virtual bool Play(IAudioPlaying **obj, bool startPlaying = true, float volume = 1, float freqRatio = 1) = 0;
		virtual bool IsPlaying() const = 0;
		virtual void StopAll() = 0;
	};

	UTIL_API bool CreateAudioMusic(IAudioDevice *device, IAudioMusic **obj);
	UTIL_API bool CreateAudioMusic(IAudioDevice *device, IAudioStream *stream, IAudioMusic **obj);
	UTIL_API bool CreateAudioMusic(IAudioDevice *device, IAudioStream *stream, float volume, float freqRatio, IAudioMusic **obj);
}
