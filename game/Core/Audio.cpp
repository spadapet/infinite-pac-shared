#include "pch.h"
#include "Audio/AudioEffect.h"
#include "COM/ComObject.h"
#include "Core/Audio.h"
#include "Core/GlobalResources.h"
#include "Module/Module.h"
#include "Resource/ResourceValue.h"
#include "States/PacApplication.h"

class __declspec(uuid("da01c079-2679-4db7-8db7-a5231262eca1"))
	SoundEffects : public ff::ComBase, public ISoundEffects
{
public:
	DECLARE_HEADER(SoundEffects);

	bool Init(CharType type);

	// ISoundEffects

	virtual void Play(AudioEffect effect) override;
	virtual void Stop(AudioEffect effect) override;
	virtual void StopAll() override;

	virtual AudioEffect GetBG() override;
	virtual void StopBG() override;

private:
	void PlayBG(AudioEffect effect);
	bool IsBG(AudioEffect effect) const;
	bool IsEnabled() const;

	ff::TypedResource<ff::IAudioEffect> _effects[EFFECT_COUNT];
	AudioEffect _curBG;
};

BEGIN_INTERFACES(SoundEffects)
	HAS_INTERFACE(ISoundEffects)
END_INTERFACES()

// static
bool ISoundEffects::Create(CharType type, ISoundEffects **ppAudio)
{
	assertRetVal(ppAudio, false);
	*ppAudio = nullptr;

	ff::ComPtr<SoundEffects> pAudio = new ff::ComObject<SoundEffects>;
	assertRetVal(pAudio->Init(type), false);

	*ppAudio = ff::GetAddRef<ISoundEffects>(pAudio);

	return true;
}

SoundEffects::SoundEffects()
	: _curBG(EFFECT_INVALID)
{
}

SoundEffects::~SoundEffects()
{
	StopAll();
}

bool SoundEffects::Init(CharType type)
{
	for (size_t i = 0; i < _countof(_effects); i++)
	{
		_effects[i] = GetGlobalEffect(type, (AudioEffect)i);
	}

	return true;
}

void SoundEffects::Play(AudioEffect effect)
{
	noAssertRet(IsEnabled());

	if (effect >= 0 && effect < _countof(_effects))
	{
		if (IsBG(effect))
		{
			PlayBG(effect);
		}
		else
		{
			ff::IAudioEffect *effectObj = _effects[effect].GetObject();
			if (!effectObj)
			{
				effectObj = _effects[effect].Flush();
				assert(effectObj);
			}

			if (effectObj)
			{
				effectObj->StopAll();
				effectObj->Play();
			}
		}
	}
}

void SoundEffects::Stop(AudioEffect effect)
{
	if (effect >= 0 && effect < _countof(_effects))
	{
		if (IsBG(effect))
		{
			StopBG();
		}
		else
		{
			ff::IAudioEffect *effectObj = _effects[effect].GetObject();
			if (effectObj)
			{
				effectObj->StopAll();
			}
		}
	}
}

void SoundEffects::StopAll()
{
	StopBG();

	for (size_t i = 0; i < _countof(_effects); i++)
	{
		ff::IAudioEffect *effectObj = _effects[i].GetObject();
		if (effectObj)
		{
			effectObj->StopAll();
		}
	}
}

void SoundEffects::PlayBG(AudioEffect effect)
{
	noAssertRet(IsEnabled());

	if (effect != _curBG)
	{
		StopBG();

		_curBG = effect;

		ff::IAudioEffect *effectObj = _effects[effect].GetObject();
		if (!effectObj)
		{
			effectObj = _effects[effect].Flush();
		}
		
		if (effectObj && !effectObj->IsPlaying())
		{
			effectObj->Play();
		}
	}
}

void SoundEffects::StopBG()
{
	if (IsBG(_curBG))
	{
		ff::IAudioEffect *effectObj = _effects[_curBG].GetObject();
		if (effectObj)
		{
			effectObj->StopAll();
		}
	}

	_curBG = EFFECT_INVALID;
}

AudioEffect SoundEffects::GetBG()
{
	return _curBG;
}

bool SoundEffects::IsBG(AudioEffect effect) const
{
	return (effect >= EFFECT_FIRST_BACKGROUND && effect <= EFFECT_LAST_BACKGROUND);
}

bool SoundEffects::IsEnabled() const
{
	const ff::Dict &options = PacApplication::Get()->GetOptions();
	bool soundOn = options.GetBool(PacApplication::OPTION_SOUND_ON, PacApplication::DEFAULT_SOUND_ON);
	return soundOn;
}
