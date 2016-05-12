#pragma once

namespace ff
{
	class IDataReader;
	class ISavedData;

	class __declspec(uuid("9cd8283a-7c8e-4c15-866d-1e8f3f7eccd2")) __declspec(novtable)
		IAudioStream : public IUnknown
	{
	public:
		virtual bool SetFile(StringRef path) = 0;
		virtual bool SetData(ISavedData *data) = 0;
		virtual bool CreateReader(IDataReader **obj) = 0;
	};

	UTIL_API bool CreateAudioStream(IAudioStream **obj);
	UTIL_API bool CreateAudioStream(ISavedData *data, IAudioStream **obj);
	UTIL_API bool CreateAudioStream(StringRef path, IAudioStream **obj);
#if METRO_APP
	UTIL_API bool CreateAudioStream(Windows::Storage::Streams::IRandomAccessStream ^stream, IAudioStream **obj);
#endif
}
