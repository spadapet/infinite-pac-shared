#pragma once

#include "App.g.h"

namespace ff
{
	class MetroGlobals;
	class ProcessGlobals;
}

namespace Maze
{
	ref class MainPage;

	ref class App sealed
	{
	public:
		App();
		virtual ~App();

		virtual void OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs ^args) override;
		void ShowAboutPage();
		bool IsShowingPopup();
		void ClosePopup();

	internal:
		static property App ^Current { App ^get(); }
		static property MainPage ^Page { MainPage ^get(); }
		static property Windows::UI::Xaml::Controls::SwapChainPanel ^Panel { Windows::UI::Xaml::Controls::SwapChainPanel ^get(); }

	private:
		void InitializeProcess();
		void InitializeGlobals();

		void ShowPopup(Windows::UI::Xaml::UIElement ^elem, double width, double height);
		void OnPopupClosed(Platform::Object ^sender, Platform::Object ^arg);

		std::unique_ptr<ff::ProcessGlobals> _processGlobals;
		std::unique_ptr<ff::MetroGlobals> _globals;
		Windows::UI::Xaml::Controls::Primitives::Popup ^_popup;
		Windows::Foundation::EventRegistrationToken _popupClosedToken;
	};
}
