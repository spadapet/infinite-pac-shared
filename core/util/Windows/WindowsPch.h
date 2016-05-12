#pragma once

// Windows includes
// Precompiled header only

#define NOMINMAX

// Plain normal windows stuff
#include <Windows.h>
#include <WindowsX.h>
#include <winerror.h>
#include <TChar.h>
#include <CommCtrl.h>
#include <OleCtl.h>
#include <Process.h>

// Media foundation
#include <MFApi.h>
#include <MFError.h>
#include <MFIdl.h>
#include <MFReadWrite.h>

#if METRO_APP
#include <agile.h>
#include <collection.h>
#include <ppltasks.h>
#include <wrl/client.h>
#else
#include <DwmApi.h>
#include <MetaHost.h>
#include <ShlObj.h>
#include <ShObjIdl.h>
#include <Uxtheme.h>
#endif

#undef NOMINMAX
