#pragma once

#include "Audio/AudioDeviceChild.h"

namespace ff
{
	class __declspec(uuid("cfbf0fa1-e942-469f-b317-3310d89d1caf")) __declspec(novtable)
		IAudioPlaying : public IAudioDeviceChild
	{
	public:
		virtual bool IsPlaying() const = 0;
		virtual bool IsPaused() const = 0;
		virtual bool IsStopped() const = 0;

		virtual void Advance() = 0;
		virtual void Stop() = 0;
		virtual void Pause() = 0;
		virtual void Resume() = 0;

		virtual double GetDuration() const = 0; // in seconds
		virtual double GetPosition() const = 0; // in seconds
		virtual bool SetPosition(double value) = 0; // in seconds
		virtual double GetVolume() const = 0;
		virtual bool SetVolume(double value) = 0;
		virtual bool FadeIn(double value) = 0;
		virtual bool FadeOut(double value) = 0;
	};
}
