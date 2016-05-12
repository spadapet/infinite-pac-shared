#include "pch.h"
#include "FailurePage.xaml.h"

Maze::FailurePage::FailurePage()
{
	InitializeComponent();
}

Maze::FailurePage::FailurePage(Platform::String ^errorText)
	: _error(errorText)
{
	InitializeComponent();
}

Platform::String ^Maze::FailurePage::ErrorText::get()
{
	return _error ? _error : "";
}
