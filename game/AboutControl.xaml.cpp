#include "pch.h"
#include "AboutControl.xaml.h"
#include "App.xaml.h"

Maze::AboutControl::AboutControl()
{
	InitializeComponent();

	auto package = Windows::ApplicationModel::Package::Current;
	auto id = package->Id;

	GameNameText->Text = package->DisplayName;
	CompanyText->Text = "By " + package->PublisherDisplayName;
	VersionText->Text = "Version " + id->Version.Major.ToString() + "." + id->Version.Minor.ToString();
}

void Maze::AboutControl::OnBackClicked(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^args)
{
	App::Current->ClosePopup();
}
