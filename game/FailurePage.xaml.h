#pragma once

#include "FailurePage.g.h"

namespace Maze
{
	[Windows::UI::Xaml::Data::Bindable]
	public ref class FailurePage sealed
	{
	public:
		FailurePage();
		FailurePage(Platform::String ^errorText);

		property Platform::String ^ErrorText { Platform::String ^get(); }

	private:
		Platform::String ^_error;
	};
}
