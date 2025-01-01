#pragma once

enum CharType;

enum AudioEffect
{
    EFFECT_INTRO,
    EFFECT_BACKGROUND1,
    EFFECT_BACKGROUND2,
    EFFECT_BACKGROUND3,
    EFFECT_BACKGROUND4,
    EFFECT_BACKGROUND_SCARED,
    EFFECT_BACKGROUND_EYES,
    EFFECT_EAT_DOT1,
    EFFECT_EAT_DOT2,
    EFFECT_EAT_POWER1,
    EFFECT_EAT_POWER2,
    EFFECT_EAT_GHOST,
    EFFECT_EAT_FRUIT,
    EFFECT_FRUIT_BOUNCE,
    EFFECT_FREE_LIFE,
    EFFECT_DYING,
    EFFECT_LEVEL_WIN,

    EFFECT_COUNT,

    EFFECT_INVALID,
    EFFECT_FIRST_BACKGROUND = EFFECT_BACKGROUND1,
    EFFECT_LAST_BACKGROUND = EFFECT_BACKGROUND_EYES,
};

class ISoundEffects
{
public:
    virtual ~ISoundEffects() = default;

    static std::shared_ptr<ISoundEffects> Create(CharType type);

    virtual void Play(AudioEffect effect) = 0;
    virtual void Stop(AudioEffect effect) = 0;
    virtual void StopAll() = 0;

    virtual AudioEffect GetBG() = 0;
    virtual void StopBG() = 0;
};
