#include "pch.h"
#include "Audio/AudioStream.h"
#include "COM/ComAlloc.h"
#include "Data/DataFile.h"
#include "Data/DataPersist.h"
#include "Data/DataWriterReader.h"
#include "Data/SavedData.h"
#include "Dict/Dict.h"
#include "Module/ModuleFactory.h"
#include "Resource/ResourcePersist.h"
#include "Windows/FileUtil.h"

static ff::StaticString PROP_DATA(L"data");

namespace ff
{
	class __declspec(uuid("3e93b56b-a835-4fc7-834e-83e0717ce24c"))
		AudioStream
			: public ComBase
			, public IAudioStream
			, public IResourceSave
	{
	public:
		DECLARE_HEADER(AudioStream);

#if METRO_APP
		bool SetStream(Windows::Storage::Streams::IRandomAccessStream ^stream);
#endif

		// IAudioStream functions
		virtual bool SetFile(StringRef path) override;
		virtual bool SetData(ISavedData *data) override;
		virtual bool CreateReader(IDataReader **obj) override;

		// IResourceSave
		virtual bool LoadResource(const ff::Dict &dict) override;
		virtual bool SaveResource(ff::Dict &dict) override;

	private:
		ComPtr<ISavedData> _data;
#if METRO_APP
		Windows::Storage::Streams::IRandomAccessStream ^_stream;
#endif
	};
}

BEGIN_INTERFACES(ff::AudioStream)
	HAS_INTERFACE(ff::IAudioStream)
	HAS_INTERFACE(ff::IResourceLoad)
	HAS_INTERFACE(ff::IResourceSave)
END_INTERFACES()

static ff::ModuleStartup RegisterAudioStream([](ff::Module &module)
{
	static ff::StaticString name(L"mp3");
	module.RegisterClassT<ff::AudioStream>(name, __uuidof(ff::IAudioStream));
});

bool ff::CreateAudioStream(IAudioStream **obj)
{
	return SUCCEEDED(ComAllocator<ff::AudioStream>::CreateInstance(nullptr, GUID_NULL, __uuidof(IAudioStream), (void**)obj));
}

bool ff::CreateAudioStream(ISavedData *data, IAudioStream **obj)
{
	assertRetVal(CreateAudioStream(obj), false);
	assertRetVal((*obj)->SetData(data), false);
	return true;
}

bool ff::CreateAudioStream(StringRef path, IAudioStream **obj)
{
	assertRetVal(CreateAudioStream(obj), false);
	assertRetVal((*obj)->SetFile(path), false);
	return true;
}

#if METRO_APP
bool ff::CreateAudioStream(Windows::Storage::Streams::IRandomAccessStream ^stream, IAudioStream **obj)
{
	ff::ComPtr<AudioStream, IAudioStream> myObj;
	assertHrRetVal(ComAllocator<AudioStream>::CreateInstance(&myObj), false);
	assertRetVal(myObj->SetStream(stream), false);

	*obj = myObj.Detach();
	return true;
}
#endif

ff::AudioStream::AudioStream()
{
}

ff::AudioStream::~AudioStream()
{
}

#if METRO_APP
bool ff::AudioStream::SetStream(Windows::Storage::Streams::IRandomAccessStream ^stream)
{
	assertRetVal(!_data && _stream == nullptr && stream != nullptr, false);
	_stream = stream;
	return true;
}
#endif

bool ff::AudioStream::SetFile(StringRef path)
{
	assertRetVal(!_data, false);
#if METRO_APP
	assertRetVal(_stream == nullptr, false);
#endif

	ComPtr<IDataFile> file;
	assertRetVal(FileExists(path), false);
	assertRetVal(CreateDataFile(path, false, &file), false);
	assertRetVal(ff::CreateSavedDataFromFile(file, 0, file->GetSize(), file->GetSize(), false, &_data), false);

	return true;
}

bool ff::AudioStream::SetData(ISavedData *data)
{
	assertRetVal(!_data && data, false);
	_data = data;
	return true;
}

bool ff::AudioStream::CreateReader(IDataReader **obj)
{
	assertRetVal(obj, false);

#if METRO_APP
	if (_stream != nullptr)
	{
		Windows::Storage::Streams::IRandomAccessStream ^stream = _stream->CloneStream();
		return ff::CreateDataReader(stream, obj);
	}
#endif

	assertRetVal(_data, false);
	return _data->CreateSavedDataReader(obj);
}

bool ff::AudioStream::LoadResource(const ff::Dict &dict)
{
	ff::Dict dataDict = dict.GetDict(PROP_DATA);

	_data = dataDict.GetSavedData(PROP_DATA);
	assertRetVal(_data, false);

	return true;
}

bool ff::AudioStream::SaveResource(ff::Dict &dict)
{
	ff::Dict dataDict;
	dataDict.SetSavedData(PROP_DATA, _data);
	dict.SetDict(PROP_DATA, dataDict);

	return true;
}
