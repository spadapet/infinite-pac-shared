#include "pch.h"
#include "Core/Audio.h"
#include "Core/Difficulty.h"
#include "Core/GlobalResources.h"

static ff::auto_resource<ff::sprite_list> GetNamedSpritePage(std::string_view name)
{
    return ff::auto_resource<ff::sprite_list>(name);
}

static ff::auto_resource<ff::audio_effect_base> GetNamedSound(std::string_view name)
{
    return ff::auto_resource<ff::audio_effect_base>(name);
}

ff::auto_resource<ff::sprite_list> GetWallSpritePage()
{
    return GetNamedSpritePage("wall-fill-sprites");
}

ff::auto_resource<ff::sprite_list> GetOutlineSpritePage()
{
    return GetNamedSpritePage("wall-border-sprites");
}

ff::auto_resource<ff::sprite_list> GetWallBackgroundSpritePage()
{
    return GetNamedSpritePage("wall-bg-sprites");
}

ff::auto_resource<ff::sprite_list> GetFontSpritePage()
{
    return GetNamedSpritePage("font-sprites");
}

ff::auto_resource<ff::sprite_list> GetSmallFontSpritePage()
{
    return GetNamedSpritePage("small-font-sprites");
}

ff::auto_resource<ff::audio_effect_base> GetGlobalEffect(CharType type, AudioEffect effect)
{
    std::string name(GetAssetPrefix(type));

    switch (effect)
    {
        case EFFECT_INTRO: name += "intro"; break;
        case EFFECT_BACKGROUND1: name += "bg-1"; break;
        case EFFECT_BACKGROUND2: name += "bg-2"; break;
        case EFFECT_BACKGROUND3: name += "bg-3"; break;
        case EFFECT_BACKGROUND4: name += "bg-4"; break;
        case EFFECT_BACKGROUND_SCARED: name += "bg-scared"; break;
        case EFFECT_BACKGROUND_EYES: name += "bg-eaten"; break;
        case EFFECT_EAT_DOT1: name += "eat-dot-1"; break;
        case EFFECT_EAT_DOT2: name += "eat-dot-2"; break;
        case EFFECT_EAT_POWER1: name += "eat-dot-1"; break;
        case EFFECT_EAT_POWER2: name += "eat-dot-2"; break;
        case EFFECT_EAT_GHOST: name += "eat-ghost"; break;
        case EFFECT_EAT_FRUIT: name += "eat-fruit"; break;
        case EFFECT_FRUIT_BOUNCE: name += "fruit-bounce"; break;
        case EFFECT_FREE_LIFE: name += "free-life"; break;
        case EFFECT_DYING: name += "die"; break;
        case EFFECT_LEVEL_WIN: name += "level-win"; break;
        default: assert(false); name.clear(); break;
    }

    return GetNamedSound(name);
}

static size_t s_eventUp = ff::stable_hash_func("up"sv);
static size_t s_eventDown = ff::stable_hash_func("down"sv);
static size_t s_eventLeft = ff::stable_hash_func("left"sv);
static size_t s_eventRight = ff::stable_hash_func("right"sv);
static size_t s_eventAction = ff::stable_hash_func("action"sv);
static size_t s_eventCancel = ff::stable_hash_func("cancel"sv);
static size_t s_eventHome = ff::stable_hash_func("home"sv);
static size_t s_eventPause = ff::stable_hash_func("pause"sv);
static size_t s_eventStart = ff::stable_hash_func("start"sv);
static size_t s_eventPauseAdvance = ff::stable_hash_func("pauseAdvance"sv);
static size_t s_eventClick = ff::stable_hash_func("click"sv);

std::string_view GetAssetPrefix(CharType type)
{
    return "mr-";
}

size_t GetEventUp()
{
    return s_eventUp;
}

size_t GetEventDown()
{
    return s_eventDown;
}

size_t GetEventLeft()
{
    return s_eventLeft;
}

size_t GetEventRight()
{
    return s_eventRight;
}

size_t GetEventAction()
{
    return s_eventAction;
}

size_t GetEventCancel()
{
    return s_eventCancel;
}

size_t GetEventHome()
{
    return s_eventHome;
}

size_t GetEventPause()
{
    return s_eventPause;
}

size_t GetEventStart()
{
    return s_eventStart;
}

size_t GetEventPauseAdvance()
{
    return s_eventPauseAdvance;
}

size_t GetEventClick()
{
    return s_eventClick;
}

std::shared_ptr<ff::input_event_provider> GetGlobalInputMapping()
{
    static std::shared_ptr<ff::input_mapping> input_mapping;
    if (!input_mapping)
    {
        input_mapping = ff::auto_resource<ff::input_mapping>("controls").object();
    }

    std::vector<ff::input_vk const*> devices;
    devices.push_back(&ff::input::keyboard());
    devices.push_back(&ff::input::pointer());

    for (size_t i = 0; i < ff::input::gamepad_count(); i++)
    {
        devices.push_back(&ff::input::gamepad(i));
    }

    return std::make_shared<ff::input_event_provider>(*input_mapping, std::move(devices));
}
