#include "pch.h"
#include "Core/Audio.h"
#include "Core/GlobalResources.h"
#include "States/PacApplication.h"

class SoundEffects : public ISoundEffects
{
public:
    SoundEffects(CharType type);
    virtual ~SoundEffects() override;

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
    bool IsVibrateEnabled() const;

    ff::auto_resource<ff::audio_effect_base> _effects[EFFECT_COUNT];
    AudioEffect _curBG;
};

// static
std::shared_ptr<ISoundEffects> ISoundEffects::Create(CharType type)
{
    return std::make_shared<SoundEffects>(type);
}

SoundEffects::SoundEffects(CharType type)
    : _curBG(EFFECT_INVALID)
{
    for (size_t i = 0; i < _countof(_effects); i++)
    {
        _effects[i] = GetGlobalEffect(type, (AudioEffect)i);
    }
}

SoundEffects::~SoundEffects()
{
    StopAll();
}

void SoundEffects::Play(AudioEffect effect)
{
    check_ret(IsEnabled());

    if (IsEnabled() && effect >= 0 && effect < _countof(_effects))
    {
        if (IsBG(effect))
        {
            PlayBG(effect);
        }
        else if (_effects[effect].object())
        {
            _effects[effect]->stop();
            _effects[effect]->play();
        }
    }

    if (IsVibrateEnabled())
    {
        switch (effect)
        {
            case EFFECT_DYING:
                ff::input::gamepad(0).vibrate(0.375, 0, 1.25);
                break;

            case EFFECT_EAT_GHOST:
                ff::input::gamepad(0).vibrate(0.75, 0, 0.75);
                break;

            case EFFECT_EAT_FRUIT:
            case EFFECT_EAT_POWER1:
            case EFFECT_EAT_POWER2:
                ff::input::gamepad(0).vibrate(0, 1, 0.5);
                break;

            case EFFECT_FRUIT_BOUNCE:
                ff::input::gamepad(0).vibrate(0.5, 0, 0.1);
                break;

            case EFFECT_EAT_DOT1:
                ff::input::gamepad(0).vibrate(0, 0.5, 0.1);
                break;

            case EFFECT_EAT_DOT2:
                // No vibrate, every other dot is enough shaking
                break;
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
        else if (_effects[effect].object())
        {
            _effects[effect]->stop();
        }
    }
}

void SoundEffects::StopAll()
{
    StopBG();

    for (size_t i = 0; i < _countof(_effects); i++)
    {
        _effects[i].object()->stop();
    }
}

void SoundEffects::PlayBG(AudioEffect effect)
{
    check_ret(IsEnabled());

    if (effect != _curBG)
    {
        StopBG();

        _curBG = effect;

        ff::audio_effect_base* effectObj = _effects[effect].object().get();
        if (!effectObj->playing())
        {
            effectObj->play();
        }
    }
}

void SoundEffects::StopBG()
{
    if (IsBG(_curBG))
    {
        ff::audio_effect_base* effectObj = _effects[_curBG].object().get();
        effectObj->stop();
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
    const ff::dict& options = PacApplication::Get()->GetOptions();
    bool soundOn = options.get<bool>(PacApplication::OPTION_SOUND_ON, PacApplication::DEFAULT_SOUND_ON);
    return soundOn;
}

bool SoundEffects::IsVibrateEnabled() const
{
    const ff::dict& options = PacApplication::Get()->GetOptions();
    bool vibrateOn = options.get<bool>(PacApplication::OPTION_VIBRATE_ON, PacApplication::DEFAULT_VIBRATE_ON);
    return vibrateOn;
}
