#pragma once

enum CharType;
enum AudioEffect;

ff::auto_resource<ff::sprite_list> GetWallSpritePage();
ff::auto_resource<ff::sprite_list> GetOutlineSpritePage();
ff::auto_resource<ff::sprite_list> GetWallBackgroundSpritePage();
ff::auto_resource<ff::sprite_list> GetFontSpritePage();
ff::auto_resource<ff::sprite_list> GetSmallFontSpritePage();

ff::auto_resource<ff::audio_effect_base> GetGlobalEffect(CharType type, AudioEffect effect);

std::string_view GetAssetPrefix(CharType type);
size_t GetEventUp();
size_t GetEventDown();
size_t GetEventLeft();
size_t GetEventRight();
size_t GetEventAction();
size_t GetEventCancel();
size_t GetEventHome();
size_t GetEventPause();
size_t GetEventStart();
size_t GetEventPauseAdvance();
size_t GetEventClick();
std::shared_ptr<ff::input_event_provider> GetGlobalInputMapping();
