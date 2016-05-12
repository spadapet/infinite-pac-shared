#include "pch.h"
#include "AboutControl.xaml.h"
#include "App.xaml.h"
#include "FailurePage.xaml.h"
#include "Globals/MetroGlobals.h"
#include "Globals/ProcessGlobals.h"
#include "Graph/RenderTarget/RenderTarget.h"
#include "MainPage.xaml.h"
#include "States/PacApplication.h"
#include "STring/StringUtil.h"
#include "Thread/ThreadDispatch.h"
#include "Windows/FileUtil.h"

Maze::App::App()
{
	InitializeComponent();
}

Maze::App::~App()
{
	if (_globals)
	{
		_globals->Shutdown();
		_globals.reset();
	}

	if (_processGlobals)
	{
		_processGlobals->Shutdown();
		_processGlobals.reset();
	}
}

Maze::App ^Maze::App::Current::get()
{
	return safe_cast<App ^>(Application::Current);
}

Maze::MainPage ^Maze::App::Page::get()
{
	return dynamic_cast<MainPage ^>(Windows::UI::Xaml::Window::Current->Content);
}

Windows::UI::Xaml::Controls::SwapChainPanel ^Maze::App::Panel::get()
{
	MainPage ^page = App::Page;
	return (page != nullptr)
		? dynamic_cast<Windows::UI::Xaml::Controls::SwapChainPanel ^>(page->Content)
		: nullptr;
}

void Maze::App::OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs ^args)
{
	InitializeProcess();

	switch (args->PreviousExecutionState)
	{
	case Windows::ApplicationModel::Activation::ApplicationExecutionState::Running:
	case Windows::ApplicationModel::Activation::ApplicationExecutionState::Suspended:
		// great, keep on running
		break;

	default:
	case Windows::ApplicationModel::Activation::ApplicationExecutionState::NotRunning:
	case Windows::ApplicationModel::Activation::ApplicationExecutionState::ClosedByUser:
	case Windows::ApplicationModel::Activation::ApplicationExecutionState::Terminated:
		ff::GetMainThreadDispatch()->Post([this]()
		{
			InitializeGlobals();
		});
		break;
	}
}

void Maze::App::InitializeProcess()
{
	noAssertRet(!_processGlobals);
	_processGlobals = std::make_unique<ff::ProcessGlobals>();
	_processGlobals->Startup();
}

void Maze::App::InitializeGlobals()
{
	noAssertRet(!_globals);
	_globals = std::make_unique<ff::MetroGlobals>();

	auto window = Windows::UI::Xaml::Window::Current;
	auto displayInfo = Windows::Graphics::Display::DisplayInformation::GetForCurrentView();
	auto page = ref new MainPage();
	auto panel = safe_cast<Windows::UI::Xaml::Controls::SwapChainPanel ^>(page->Content);
	auto pointerSettings = Windows::UI::Input::PointerVisualizationSettings::GetForCurrentView();

	pointerSettings->IsBarrelButtonFeedbackEnabled = false;
	pointerSettings->IsContactFeedbackEnabled = false;

	auto finishInitFunc = [](ff::MetroGlobals *globals)
	{
		if (globals->GetTarget() && globals->GetTarget()->CanSetFullScreen())
		{
			ff::Dict options = globals->GetState();
			bool fullScreen = options.GetBool(PacApplication::OPTION_FULL_SCREEN, PacApplication::DEFAULT_FULL_SCREEN);
			globals->GetTarget()->SetFullScreen(fullScreen);
		}

		return true;
	};

	auto initialStateFactory = [page](ff::MetroGlobals *globals)
	{
		auto state = std::make_shared<PacApplication>();
		state->SetMainPage(page);
		return state;
	};

	window->Content = page;

	if (!_processGlobals->IsValid() ||
		!_globals->Startup(
			ff::MetroGlobalsFlags::All,
			window,
			panel,
			displayInfo,
			finishInitFunc,
			initialStateFactory))
	{
		ff::String errorText;
		if (ff::ReadWholeFile(_globals->GetLogFile(), errorText))
		{
			ff::ReplaceAll(errorText, ff::String(L"\r\n"), ff::String(L"\n"));
			ff::ReplaceAll(errorText, ff::String(L"\n"), ff::String(L"\r\n"));
			window->Content = ref new FailurePage(errorText.pstring());
		}
	}

	window->Activate();
}

void Maze::App::ShowPopup(Windows::UI::Xaml::UIElement ^elem, double width, double height)
{
	ClosePopup();
	assert(_popup == nullptr);
	noAssertRet(elem != nullptr);

	_popup = ref new Windows::UI::Xaml::Controls::Primitives::Popup();
	App::Panel->Children->Append(_popup);

	_popupClosedToken = _popup->Closed +=
		ref new Windows::Foundation::EventHandler<Platform::Object ^>(
			this, &App::OnPopupClosed);

	_popup->IsLightDismissEnabled = true;
	_popup->HorizontalAlignment = Windows::UI::Xaml::HorizontalAlignment::Center;
	_popup->VerticalAlignment = Windows::UI::Xaml::VerticalAlignment::Center;
	_popup->Width = width;
	_popup->Height = height;
	_popup->Child = elem;
	_popup->IsOpen = true;
}

void Maze::App::ClosePopup()
{
	if (_popup)
	{
		_popup->IsOpen = false;
	}
}

void Maze::App::OnPopupClosed(Platform::Object ^sender, Platform::Object ^arg)
{
	assertRet(_popup);

	_popup->Closed -= _popupClosedToken;
	_popupClosedToken.Value = 0;

	unsigned int index;
	if (App::Panel->Children->IndexOf(_popup, &index))
	{
		App::Panel->Children->RemoveAt(index);
	}
	else
	{
		assert(false);
	}

	_popup = nullptr;
}

void Maze::App::ShowAboutPage()
{
	ff::GetMainThreadDispatch()->Post([this]()
	{
		auto window = Windows::UI::Xaml::Window::Current;

		auto page = ref new AboutControl();
		page->Height = std::min(window->Bounds.Height - 20.0f, 600.0f);

		ShowPopup(page, page->Width, page->Height);
	}, true);
}

bool Maze::App::IsShowingPopup()
{
	return _popup != nullptr;
}
