// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "main_frame.h"

#include "application.h"
#include "choose_conflict_resolve_mode_view.hpp"
#include "domain/imod_manager.hpp"
#include "domain/imod_platform.hpp"
#include "interface/domain/ilocal_config.h"
#include "interface/service/iapp_config.h"
#include "interface/service/iplatform_service.h"
#include "manage_preset_list_view.hpp"
#include "mod_list_view.h"
#include "mod_manager_app.h"
#include "resolve_mod_conflicts_view.hpp"
#include "select_directory_view.h"
#include "select_exe.h"
#include "select_platform_view.h"
#include "service/ii18n_service.hpp"
#include "show_file_list_dialog.hpp"
#include "show_file_list_helper.hpp"
#include "system_info.hpp"
#include "types/main_window_properties.h"
#include "utility/sdlexcept.h"

#include <wx/aboutdlg.h>
#include <wx/aui/auibook.h>
#include <wx/busyinfo.h>
#include <wx/menu.h>
#include <wx/sizer.h>

using namespace mm;

namespace
{
	const wxSize minFrameSize(930, 575);
}

MainFrame::MainFrame(Application& app)
	: wxFrame(nullptr, wxID_ANY, constant::program_full_version, app.appConfig().mainWindow().position,
			  app.appConfig().mainWindow().size)
	, _app(app)
{
	SetIcon(wxICON(MainMMIcon));
	SetSizeHints(minFrameSize, wxDefaultSize);

	reloadModel();

	createMenuBar();
	CreateStatusBar();

	auto layout = new wxBoxSizer(wxHORIZONTAL);
	if (_currentPlatform)
	{
		_modListView = new ModListView(this, *_currentPlatform, _app.iconStorage());
		layout->Add(_modListView, wxSizerFlags(1).Expand());
	}

	SetSizer(layout);
	Layout();
	Maximize(app.appConfig().mainWindow().maximized);

	Bind(wxEVT_CLOSE_WINDOW, &MainFrame::OnCloseWindow, this);
};

void MainFrame::OnAbout()
{
	wxAboutDialogInfo aboutInfo;
	aboutInfo.SetName(PROGRAM_NAME);
	aboutInfo.SetVersion(PROGRAM_VERSION);
	aboutInfo.SetDescription("A mod manager for Era II");
	aboutInfo.SetCopyright(L"(C) 2020 Aliaksei Karalenka");
	aboutInfo.SetWebSite("http://wforum.heroes35.net");
	aboutInfo.AddDeveloper("Aliaksei SyDr Karalenka");
	aboutInfo.SetLicence(constant::program_license_text);

	wxAboutBox(aboutInfo);
}

void MainFrame::createMenuBar()
{
	auto profileMenu = new wxMenu();
	auto manageProfiles =
		profileMenu->Append(wxID_ANY, "Manage"_lng, nullptr, "Manage profiles"_lng);
	auto saveProfile =
		profileMenu->Append(wxID_ANY, "Save as"_lng, nullptr, "Save current mod list as profile"_lng);
	_menuItems[manageProfiles->GetId()] = [&] { OnMenuManageProfilesRequested(false); };
	_menuItems[saveProfile->GetId()]    = [&] { OnMenuManageProfilesRequested(true); };

	auto toolsMenu = new wxMenu();
	if (!_app.appConfig().portableMode())
	{
		auto changeDirectory = toolsMenu->Append(wxID_ANY, "Change managed directory"_lng, nullptr,
												 "Allows selecting other directory for management"_lng);

		_menuItems[changeDirectory->GetId()] = [&] { OnMenuToolsChangeDirectory(); };
	}

	auto listModFiles =
		toolsMenu->Append(wxID_ANY, "List active mod files"_lng, nullptr, "List active mod files"_lng);
	_menuItems[listModFiles->GetId()] = [&] { OnMenuToolsListModFiles(); };

	auto sortMods =
		toolsMenu->Append(wxID_ANY, "Resolve mods conflicts"_lng, nullptr, "Resolve mods conflicts"_lng);
	_menuItems[sortMods->GetId()] = [&] { OnMenuToolsSortMods(); };

	toolsMenu->AppendSeparator();

	auto languageMenu = new wxMenu();

	for (const auto& lngCode : { "en_US", "ru_RU" })  // let's wait until someone complains
	{
		auto lngItem = languageMenu->AppendRadioItem(wxID_ANY, _app.i18nService().languageName(lngCode));
		if (_app.appConfig().currentLanguageCode() == lngCode)
			lngItem->Check();

		_menuItems[lngItem->GetId()] = [&] { OnMenuToolsLanguageSelected(lngCode); };
	}

	toolsMenu->AppendSubMenu(languageMenu, "Language"_lng, "Allows selecting other language for program"_lng);

	auto helpMenu              = new wxMenu();
	auto about                 = helpMenu->Append(wxID_ABOUT, "About"_lng, nullptr, "About"_lng);
	_menuItems[about->GetId()] = [&] { OnAbout(); };

	_mainMenu = new wxMenuBar();
	_mainMenu->Append(profileMenu, "Profiles"_lng);
	_mainMenu->Append(toolsMenu, "Tools"_lng);
	_mainMenu->Append(helpMenu, "?");
	_mainMenu->Bind(wxEVT_MENU, &MainFrame::OnMenuItemSelected, this);

	SetMenuBar(_mainMenu);
}

void MainFrame::OnMenuManageProfilesRequested(bool saveAs)
{
	try_handle_exceptions(this, [&] {
		ManagePresetListView mplv(this, *_currentPlatform, _app.iconStorage());
		if (saveAs)
			mplv.CallAfter(&ManagePresetListView::onSavePresetRequested, wxEmptyString);

		mplv.ShowWindowModal();
	});
}

void MainFrame::OnMenuToolsChangeDirectory()
{
	EX_TRY;

	SelectDirectoryDialog dialog(this, _app.appConfig(), _app.iconStorage());

	if (dialog.ShowModal() == wxID_OK)
	{
		wxBusyCursor bc;
		wxString     path = dialog.getSelectedPath();

		_app.appConfig().setDataPath(path.ToStdWstring());
		wxGetApp().scheduleRestart();
	}

	EX_UNEXPECTED;
}

void MainFrame::OnMenuToolsListModFiles()
{
	try_handle_exceptions(this, [&] {
		showModFileList(*this, _app, *_currentPlatform->modDataProvider(),
						_currentPlatform->getModManager()->mods());
	});
}

void MainFrame::OnMenuToolsSortMods()
{
	try_handle_exceptions(this, [&] {
		ResolveModConflictsView view(this, _app.appConfig(), _app.iconStorage(), *_currentPlatform);
		view.ShowModal();
	});
}

void MainFrame::OnMenuChangePlatform()
{
	EX_TRY;

	SelectPlatformView dialog(this);

	if (dialog.ShowModal() == wxID_OK)
	{
		wxBusyCursor bc;
		std::string  platform = dialog.selectedPlatform();

		_app.appConfig().setSelectedPlatformCode(platform);
		wxGetApp().scheduleRestart();
	}

	EX_UNEXPECTED;
}

void MainFrame::OnMenuToolsLanguageSelected(const wxString& value)
{
	_app.appConfig().setCurrentLanguageCode(value);
	wxGetApp().scheduleRestart();
}

void MainFrame::OnMenuItemSelected(wxCommandEvent& event)
{
	const auto item = _menuItems.find(event.GetId());

	wxASSERT(item != _menuItems.end());

	if (item == _menuItems.end())
		return;

	EX_TRY;

	item->second();

	EX_UNEXPECTED;
}

void MainFrame::reloadModel()
{
	EX_TRY;

	_currentPlatform = _app.platformService().create(_app.appConfig().selectedPlatform());

	/*if (_currentPlatform->localConfig()->conflictResolveMode() ==
	ConflictResolveMode::undefined)
	{
		ChooseConflictResolveModeView dialog(this);

		if (dialog.ShowModal() == wxID_OK)
			_currentPlatform->localConfig()->conflictResolveMode(dialog.conflictResolveMode());
	}*/

	EX_ON_EXCEPTION(empty_path_error, SINK_EXCEPTION(OnMenuToolsChangeDirectory));
	EX_ON_EXCEPTION(not_exist_path_error, [](not_exist_path_error const&) {
		wxMessageOutputBest().Printf("Selected path doesn't exists, please choose suitable one");
	});
	EX_UNEXPECTED;
}

void MainFrame::OnMenuCheckForUpdates()
{
	EX_TRY;

	throw std::runtime_error("Hey, you found a bug here... No... In fact this is not a bug");

	EX_UNEXPECTED;
}

void MainFrame::OnCloseWindow(wxCloseEvent& event)
{
	saveWindowProperties();

	event.Skip();
}

void MainFrame::saveWindowProperties()
{
	MainWindowProperties props;

	props.maximized = IsMaximized();
	props.position  = GetPosition();
	props.size      = GetSize();

	_app.appConfig().setMainWindowProperties(props);
}
