#pragma once

#include "MainPage.g.h"

namespace Maze
{
	[Windows::UI::Xaml::Data::Bindable]
	public ref class MainPage sealed : Windows::UI::Xaml::Data::INotifyPropertyChanged
	{
	public:
		MainPage();
		virtual ~MainPage();

		virtual event Windows::UI::Xaml::Data::PropertyChangedEventHandler ^PropertyChanged;
		property Windows::UI::Xaml::Visibility HomeVisibility { Windows::UI::Xaml::Visibility get(); }
		property Windows::UI::Xaml::Visibility PauseVisibility { Windows::UI::Xaml::Visibility get(); }

	internal:
		enum class State
		{
			None,
			Playing,
			Paused,
		};

		void SetState(State state);

	private:
		void OnPropertyChanged(Platform::String ^name);
		void OnClickHome(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^args);
		void OnClickPause(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^args);

		State _state;
	};
}
