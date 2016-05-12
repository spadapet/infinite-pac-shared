#include "pch.h"
#include "Audio/AudioBuffer.h"
#include "COM/ComAlloc.h"
#include "Data/Data.h"
#include "Data/DataFile.h"
#include "Data/DataPersist.h"
#include "Data/DataWriterReader.h"
#include "Data/SavedData.h"
#include "Dict/Dict.h"
#include "Module/ModuleFactory.h"
#include "Resource/ResourcePersist.h"

static ff::StaticString PROP_DATA(L"data");
static ff::StaticString PROP_FORMAT(L"format");

#if !METRO_APP

class WaveFileHelper
{
public:
	WaveFileHelper(ff::IData *fullData);
	~WaveFileHelper();

	const WAVEFORMATEX *GetFormat();
	size_t GetFormatSize();

	ff::IData *GetData();

private:
	bool Read(HMMIO hio);

	std::unique_ptr<WAVEFORMATEX> _format;
	ff::ComPtr<ff::IData> _fullData;
	ff::ComPtr<ff::IData> _data;
};

WaveFileHelper::WaveFileHelper(ff::IData *fullData)
	: _fullData(fullData)
{
	if (_fullData && _fullData->GetSize())
	{
		MMIOINFO info;
		ff::ZeroObject(info);
		info.fccIOProc = FOURCC_MEM;
		info.pchBuffer = (HPSTR)_fullData->GetMem();
		info.cchBuffer = (LONG)_fullData->GetSize();

		HMMIO hio = mmioOpen(nullptr, &info, MMIO_ALLOCBUF | MMIO_READ);
		assert(hio);

		if (hio)
		{
			verify(Read(hio));
			mmioClose(hio, 0);
		}
	}
}

WaveFileHelper::~WaveFileHelper()
{
}

bool WaveFileHelper::Read(HMMIO hio)
{
	assertRetVal(hio && !_format && !_data, false);

	MMCKINFO ckRiff;
	ff::ZeroObject(ckRiff);
	assertRetVal(MMSYSERR_NOERROR == mmioDescend(hio, &ckRiff, nullptr, 0), false);
	assertRetVal(ckRiff.ckid == FOURCC_RIFF && ckRiff.fccType == mmioFOURCC('W', 'A', 'V', 'E'), false);

	MMCKINFO ckIn;
	ff::ZeroObject(ckIn);
	ckIn.ckid = mmioFOURCC('f', 'm', 't', ' ');
	assertRetVal(MMSYSERR_NOERROR == mmioDescend(hio, &ckIn, &ckRiff, MMIO_FINDCHUNK), false);
	assertRetVal(ckIn.cksize >= sizeof(PCMWAVEFORMAT), false);

	PCMWAVEFORMAT pcmFormat;
	ff::ZeroObject(pcmFormat);
	assertRetVal(mmioRead(hio, (HPSTR)&pcmFormat, sizeof(pcmFormat)) == sizeof(pcmFormat), false);

	if(pcmFormat.wf.wFormatTag == WAVE_FORMAT_PCM)
	{
		_format.reset(new WAVEFORMATEX);
		*_format = *(WAVEFORMATEX*)&pcmFormat;
		_format->cbSize = 0;
	}
	else
	{
		// Read in length of extra bytes.
		WORD cbExtraBytes;
		assertRetVal(mmioRead(hio, (HPSTR)&cbExtraBytes, sizeof(WORD)) == sizeof(WORD), false);

		_format.reset((WAVEFORMATEX*)new BYTE[sizeof(WAVEFORMATEX) + cbExtraBytes]);
		*_format = *(WAVEFORMATEX*)&pcmFormat;
		_format->cbSize = cbExtraBytes;

		assertRetVal(mmioRead(hio, (HPSTR)(&_format->cbSize + 1), cbExtraBytes) == cbExtraBytes, false);
	}

	assertRetVal(MMSYSERR_NOERROR == mmioAscend(hio, &ckIn, 0), false);

	// Seek to the data
	MMCKINFO ckData;
	ff::ZeroObject(ckData);
	ckData.ckid = mmioFOURCC('d', 'a', 't', 'a');
	assertRetVal(-1 != mmioSeek(hio, ckRiff.dwDataOffset + sizeof(FOURCC), SEEK_SET), false);
	assertRetVal(MMSYSERR_NOERROR == mmioDescend(hio, &ckData, &ckRiff, MMIO_FINDCHUNK), false);

	assertRetVal(ff::CreateDataInData(_fullData, ckData.dwDataOffset, ckData.cksize, &_data), false);

	return true;
}

const WAVEFORMATEX *WaveFileHelper::GetFormat()
{
	return _format.get();
}

size_t WaveFileHelper::GetFormatSize()
{
	return _format ? _format->cbSize + sizeof(WAVEFORMATEX) : 0;
}

ff::IData *WaveFileHelper::GetData()
{
	return _data;
}

#endif // !METRO_APP

class __declspec(uuid("383db502-512c-4892-94d2-2cdaeccb754d"))
	AudioBuffer
		: public ff::ComBase
		, public ff::IAudioBuffer
		, public ff::IResourceSave
{
public:
	DECLARE_HEADER(AudioBuffer);

	// IAudioBuffer functions
	virtual bool SetFile(ff::StringRef path) override;

	virtual const WAVEFORMATEX *GetFormat() const override;
	virtual size_t GetFormatSize() const override;
	virtual const BYTE *GetData() const override;
	virtual size_t GetDataSize() const override;

	// IResourceSave
	virtual bool LoadResource(const ff::Dict &dict) override;
	virtual bool SaveResource(ff::Dict &dict) override;

private:
	ff::ComPtr<ff::IData> _format;
	ff::ComPtr<ff::IData> _data;
};

BEGIN_INTERFACES(AudioBuffer)
	HAS_INTERFACE(ff::IAudioBuffer)
	HAS_INTERFACE(ff::IResourceLoad)
	HAS_INTERFACE(ff::IResourceSave)
END_INTERFACES()

static ff::ModuleStartup RegisterAudioBuffer([](ff::Module &module)
{
	static ff::StaticString name(L"wave");
	module.RegisterClassT<AudioBuffer>(name, __uuidof(ff::IAudioBuffer));
});

bool ff::CreateAudioBuffer(ff::IAudioBuffer **buffer)
{
	return SUCCEEDED(ff::ComAllocator<AudioBuffer>::CreateInstance(
		nullptr, GUID_NULL, __uuidof(ff::IAudioBuffer), (void**)buffer));
}

AudioBuffer::AudioBuffer()
{
}

AudioBuffer::~AudioBuffer()
{
}

#if METRO_APP
bool AudioBuffer::SetFile(ff::StringRef path)
{
	assertRetVal(false, false);
}
#else
bool AudioBuffer::SetFile(ff::StringRef path)
{
	assertRetVal(!_format && !_data, false);

	ff::ComPtr<ff::IDataFile> pFile;
	assertRetVal(ff::CreateDataFile(path, false, &pFile), false);

	ff::ComPtr<ff::ISavedData> pFileData;
	assertRetVal(ff::CreateSavedDataFromFile(pFile, 0, pFile->GetSize(), pFile->GetSize(), false, &pFileData), false);

	WaveFileHelper waveHelper(pFileData->Load());
	assertRetVal(waveHelper.GetData() && waveHelper.GetFormat() && waveHelper.GetFormatSize(), false);

	ff::ComPtr<ff::IDataVector> pVector;
	ff::ComPtr<ff::IDataWriter> pWriter;
	assertRetVal(ff::CreateDataWriter(&pVector, &pWriter), false);
	assertRetVal(pWriter->Write(waveHelper.GetFormat(), waveHelper.GetFormatSize()), false);

	_data = waveHelper.GetData();
	_format = pVector;

	return true;
}
#endif

const WAVEFORMATEX *AudioBuffer::GetFormat() const
{
	return _format ? (const WAVEFORMATEX*)_format->GetMem() : nullptr;
}

size_t AudioBuffer::GetFormatSize() const
{
	return GetFormat() ? GetFormat()->cbSize + sizeof(WAVEFORMATEX) : 0;
}

const BYTE *AudioBuffer::GetData() const
{
	return _data ? _data->GetMem() : nullptr;
}

size_t AudioBuffer::GetDataSize() const
{
	return _data ? _data->GetSize() : 0;
}

bool AudioBuffer::LoadResource(const ff::Dict &dict)
{
	ff::Dict dataDict = dict.GetDict(PROP_DATA);

	_data = dataDict.GetData(PROP_DATA);
	_format = dataDict.GetData(PROP_FORMAT);
	assertRetVal(_data && _format, false);

	return true;
}

bool AudioBuffer::SaveResource(ff::Dict &dict)
{
	ff::Dict dataDict;
	dataDict.SetData(PROP_DATA, _data);
	dataDict.SetData(PROP_FORMAT, _format);
	dict.SetDict(PROP_DATA, dataDict);

	return true;
}
