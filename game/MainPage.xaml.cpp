#include "pch.h"
#include "App.xaml.h"
#include "Core/GlobalResources.h"
#include "MainPage.xaml.h"
#include "States/PacApplication.h"
#include "Thread/ThreadDispatch.h"

Maze::MainPage::MainPage()
{
	InitializeComponent();
}

Maze::MainPage::~MainPage()
{
}

Windows::UI::Xaml::Visibility Maze::MainPage::HomeVisibility::get()
{
	return (_state == State::Paused)
		? Windows::UI::Xaml::Visibility::Visible
		: Windows::UI::Xaml::Visibility::Collapsed;
}

Windows::UI::Xaml::Visibility Maze::MainPage::PauseVisibility::get()
{
	return (_state == State::Playing)
		? Windows::UI::Xaml::Visibility::Visible
		: Windows::UI::Xaml::Visibility::Collapsed;
}

void Maze::MainPage::SetState(State state)
{
	if (state != _state)
	{
		ff::GetMainThreadDispatch()->Post([this, state]()
		{
			if (state != _state)
			{
				_state = state;

				OnPropertyChanged("HomeVisibility");
				OnPropertyChanged("PauseVisibility");
			}
		}, true);
	}
}

void Maze::MainPage::OnPropertyChanged(Platform::String ^name)
{
	PropertyChanged(this, ref new Windows::UI::Xaml::Data::PropertyChangedEventArgs(name));
}

void Maze::MainPage::OnClickHome(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^args)
{
	PacApplication::Get()->SetInputEvent(GetEventHome());
}

void Maze::MainPage::OnClickPause(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^args)
{
	PacApplication::Get()->SetInputEvent(GetEventPause());
}
