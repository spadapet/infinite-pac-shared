#pragma once

namespace ff
{
	class __declspec(uuid("34549a41-ce57-4b8f-85b0-53cd5820c2dd")) __declspec(novtable)
		IAudioBuffer : public IUnknown
	{
	public:
		virtual bool SetFile(ff::StringRef path) = 0;

		virtual const WAVEFORMATEX *GetFormat() const = 0;
		virtual size_t GetFormatSize() const = 0;
		virtual const BYTE *GetData() const = 0;
		virtual size_t GetDataSize() const = 0;
	};

	UTIL_API bool CreateAudioBuffer(IAudioBuffer **buffer);
}
