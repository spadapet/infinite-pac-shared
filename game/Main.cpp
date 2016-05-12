#include "pch.h"
#include "App.xaml.h"
#include "MainUtilInclude.h"
#include "Module/Module.h"
#include "String/StringUtil.h"

// {D81663B1-9722-4808-90E9-6C656A7F990F}
static const GUID MODULE_ID = { 0xd81663b1, 0x9722, 0x4808, { 0x90, 0xe9, 0x6c, 0x65, 0x6a, 0x7f, 0x99, 0xf } };
static ff::StaticString MODULE_NAME(L"Maze");
EXTERN_C IMAGE_DOS_HEADER __ImageBase;

static HINSTANCE GetMainInstance()
{
	return (HINSTANCE)&__ImageBase;
}

static std::unique_ptr<ff::Module> CreateThisModule()
{
	return std::make_unique<ff::Module>(MODULE_NAME, MODULE_ID, GetMainInstance());
}

static ff::ModuleFactory RegisterThisModule(MODULE_NAME, MODULE_ID, GetMainInstance, ff::GetModuleStartup, CreateThisModule);

[Platform::MTAThread]
int main(Platform::Array<Platform::String ^> ^args)
{
	ff::SetMainModule(MODULE_NAME, MODULE_ID, GetMainInstance());

	auto callbackFunc = [](Windows::UI::Xaml::ApplicationInitializationCallbackParams ^args)
	{
		auto app = ref new ::Maze::App();
	};

	auto callback = ref new Windows::UI::Xaml::ApplicationInitializationCallback(callbackFunc);
	Windows::UI::Xaml::Application::Start(callback);

	return 0;
}
