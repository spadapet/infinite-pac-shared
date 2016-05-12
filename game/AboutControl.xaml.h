#pragma once

#include "AboutControl.g.h"

namespace Maze
{
	public ref class AboutControl sealed
	{
	public:
		AboutControl();

	private:
		void OnBackClicked(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^args);
	};
}
