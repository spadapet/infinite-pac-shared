#include "pch.h"
#include "Audio/AudioEffect.h"
#include "Core/Audio.h"
#include "Core/Difficulty.h"
#include "Core/GlobalResources.h"
#include "Globals/MetroGlobals.h"
#include "Graph/2D/SpriteList.h"
#include "Input/InputMapping.h"
#include "Input/Joystick/JoystickDevice.h"
#include "Input/Joystick/JoystickInput.h"
#include "Input/KeyboardDevice.h"
#include "Input/PointerDevice.h"
#include "Module/Module.h"

static ff::TypedResource<ff::ISpriteList> GetNamedSpritePage(ff::StringRef name)
{
	return ff::TypedResource<ff::ISpriteList>(name);
}

static ff::TypedResource<ff::IAudioEffect> GetNamedSound(ff::StringRef name)
{
	return ff::TypedResource<ff::IAudioEffect>(name);
}

ff::TypedResource<ff::ISpriteList> GetWallSpritePage()
{
	return GetNamedSpritePage(ff::String(L"wall-fill-sprites"));
}

ff::TypedResource<ff::ISpriteList> GetOutlineSpritePage()
{
	return GetNamedSpritePage(ff::String(L"wall-border-sprites"));
}

ff::TypedResource<ff::ISpriteList> GetWallBackgroundSpritePage()
{
	return GetNamedSpritePage(ff::String(L"wall-bg-sprites"));
}

ff::TypedResource<ff::ISpriteList> GetFontSpritePage()
{
	return GetNamedSpritePage(ff::String(L"font-sprites"));
}

ff::TypedResource<ff::ISpriteList> GetSmallFontSpritePage()
{
	return GetNamedSpritePage(ff::String(L"small-font-sprites"));
}

ff::TypedResource<ff::IAudioEffect> GetGlobalEffect(CharType type, AudioEffect effect)
{
	ff::String name = GetAssetPrefix(type);

	switch (effect)
	{
		case EFFECT_INTRO: name += L"intro"; break;
		case EFFECT_BACKGROUND1: name += L"bg-1"; break;
		case EFFECT_BACKGROUND2: name += L"bg-2"; break;
		case EFFECT_BACKGROUND3: name += L"bg-3"; break;
		case EFFECT_BACKGROUND4: name += L"bg-4"; break;
		case EFFECT_BACKGROUND_SCARED: name += L"bg-scared"; break;
		case EFFECT_BACKGROUND_EYES: name += L"bg-eaten"; break;
		case EFFECT_EAT_DOT1: name += L"eat-dot-1"; break;
		case EFFECT_EAT_DOT2: name += L"eat-dot-2"; break;
		case EFFECT_EAT_GHOST: name += L"eat-ghost"; break;
		case EFFECT_EAT_FRUIT: name += L"eat-fruit"; break;
		case EFFECT_FRUIT_BOUNCE: name += L"fruit-bounce"; break;
		case EFFECT_FREE_LIFE: name += L"free-life"; break;
		case EFFECT_DYING: name += L"die"; break;
		case EFFECT_LEVEL_WIN: name += L"level-win"; break;
		default: assert(false); name.clear(); break;
	}

	return GetNamedSound(name);
}

static ff::hash_t s_eventUp = ff::HashFunc(L"up");
static ff::hash_t s_eventDown = ff::HashFunc(L"down");
static ff::hash_t s_eventLeft = ff::HashFunc(L"left");
static ff::hash_t s_eventRight = ff::HashFunc(L"right");
static ff::hash_t s_eventAction = ff::HashFunc(L"action");
static ff::hash_t s_eventCancel = ff::HashFunc(L"cancel");
static ff::hash_t s_eventHome = ff::HashFunc(L"home");
static ff::hash_t s_eventPause = ff::HashFunc(L"pause");
static ff::hash_t s_eventStart = ff::HashFunc(L"start");
static ff::hash_t s_eventPauseAdvance = ff::HashFunc(L"pauseAdvance");
static ff::hash_t s_eventClick = ff::HashFunc(L"click");

ff::String GetAssetPrefix(CharType type)
{
	return ff::String(L"mr-");
}

ff::hash_t GetEventUp()
{
	return s_eventUp;
}

ff::hash_t GetEventDown()
{
	return s_eventDown;
}

ff::hash_t GetEventLeft()
{
	return s_eventLeft;
}

ff::hash_t GetEventRight()
{
	return s_eventRight;
}

ff::hash_t GetEventAction()
{
	return s_eventAction;
}

ff::hash_t GetEventCancel()
{
	return s_eventCancel;
}

ff::hash_t GetEventHome()
{
	return s_eventHome;
}

ff::hash_t GetEventPause()
{
	return s_eventPause;
}

ff::hash_t GetEventStart()
{
	return s_eventStart;
}

ff::hash_t GetEventPauseAdvance()
{
	return s_eventPauseAdvance;
}

ff::hash_t GetEventClick()
{
	return s_eventClick;
}

ff::TypedResource<ff::IInputMapping> GetGlobalInputMapping()
{
	ff::TypedResource<ff::IInputMapping> inputRes(L"controls");
	inputRes.SetFilter([](ff::ComPtr<ff::IInputMapping> &obj)
	{
		ff::Vector<ff::IInputDevice*> devices;
		devices.Push(ff::MetroGlobals::Get()->GetKeys());
		devices.Push(ff::MetroGlobals::Get()->GetPointer());

		for (size_t i = 0; i < ff::MetroGlobals::Get()->GetJoysticks()->GetCount(); i++)
		{
			devices.Push(ff::MetroGlobals::Get()->GetJoysticks()->GetJoystick(i));
		}

		ff::ComPtr<ff::IInputMapping> newObj;
		if (obj->Clone(devices.Data(), devices.Size(), &newObj))
		{
			obj = newObj;
		}
	});

	return inputRes;
}
