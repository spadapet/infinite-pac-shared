#pragma once
#include "Resource/ResourceValue.h"

namespace ff
{
	class IAudioEffect;
	class IInputMapping;
	class ISpriteList;
}

enum CharType;
enum AudioEffect;

ff::TypedResource<ff::ISpriteList> GetWallSpritePage();
ff::TypedResource<ff::ISpriteList> GetOutlineSpritePage();
ff::TypedResource<ff::ISpriteList> GetWallBackgroundSpritePage();
ff::TypedResource<ff::ISpriteList> GetFontSpritePage();
ff::TypedResource<ff::ISpriteList> GetSmallFontSpritePage();

ff::TypedResource<ff::IAudioEffect> GetGlobalEffect(CharType type, AudioEffect effect);

ff::String GetAssetPrefix(CharType type);
ff::hash_t GetEventUp();
ff::hash_t GetEventDown();
ff::hash_t GetEventLeft();
ff::hash_t GetEventRight();
ff::hash_t GetEventAction();
ff::hash_t GetEventCancel();
ff::hash_t GetEventHome();
ff::hash_t GetEventPause();
ff::hash_t GetEventStart();
ff::hash_t GetEventPauseAdvance();
ff::hash_t GetEventClick();
ff::TypedResource<ff::IInputMapping> GetGlobalInputMapping();
