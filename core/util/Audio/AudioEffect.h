#pragma once

#include "Audio/AudioDeviceChild.h"

namespace ff
{
	class IAudioBuffer;
	class IAudioPlaying;

	class __declspec(uuid("31361e4d-00ed-4493-a498-2b0627d6de91")) __declspec(novtable)
		IAudioEffect : public IAudioDeviceChild
	{
	public:
		virtual bool SetBuffer(
			IAudioBuffer *buffer,
			size_t start = 0,
			size_t length = 0,
			size_t loopStart = 0,
			size_t loopLength = 0,
			size_t loopCount = 0,
			float volume = 1,
			float freqRatio = 1) = 0;

		virtual bool Play(
			bool startPlaying = true,
			float volume = 1,
			float freqRatio = 1,
			IAudioPlaying **playing = nullptr) = 0;

		virtual bool IsPlaying() const = 0;
		virtual void StopAll() = 0;
	};

	UTIL_API bool CreateAudioEffect(IAudioDevice *device, IAudioEffect **ppEffect);
}
