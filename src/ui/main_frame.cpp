// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "main_frame.h"

#include "application.h"
#include "choose_conflict_resolve_mode_view.hpp"
#include "interface/iapp_config.hpp"
#include "interface/ii18n_service.hpp"
#include "interface/iicon_storage.h"
#include "interface/ilaunch_helper.h"
#include "interface/ilocal_config.h"
#include "interface/imod_manager.hpp"
#include "interface/imod_platform.hpp"
#include "interface/iplatform_service.h"
#include "license.hpp"
#include "manage_preset_list_view.hpp"
#include "mod_list_view.h"
#include "mod_manager_app.h"
#include "plugin_list_view.hpp"
#include "resolve_mod_conflicts_view.hpp"
#include "select_directory_view.h"
#include "select_exe.h"
#include "select_platform_view.h"
#include "show_file_list_dialog.hpp"
#include "show_file_list_helper.hpp"
#include "system_info.hpp"
#include "type/embedded_icon.h"
#include "type/main_window_properties.h"
#include "utility/sdlexcept.h"
#include "utility/shell_util.h"

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
	: wxFrame(nullptr, wxID_ANY, SystemInfo::ProgramVersion, app.appConfig().mainWindow().position,
		  app.appConfig().mainWindow().size)
	, _app(app)
{
	SetIcon(wxICON(MainMMIcon));
	SetSizeHints(minFrameSize, wxDefaultSize);

	reloadModel();

	createMenuBar();
	CreateStatusBar();

	auto panel = new wxPanel(this);
	auto pages = new wxNotebook(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize);

	if (_currentPlatform)
	{
		auto modListView = new ModListView(pages, *_currentPlatform, _app.iconStorage());
		pages->AddPage(modListView, "Mods"_lng);

		if (auto pluginManager = _currentPlatform->pluginManager())
		{
			auto pluginListView = new PluginListView(
				pages, *pluginManager, *_currentPlatform->modDataProvider(), app.iconStorage());
			pages->AddPage(pluginListView, "Plugins"_lng);
		}

		if (auto presetManager = _currentPlatform->getPresetManager())
		{
			auto presetManagerView = new ManagePresetListView(pages, *_currentPlatform, app.iconStorage());
			pages->AddPage(presetManagerView, "Profiles"_lng);
		}

		if (auto launchHelper = _currentPlatform->launchHelper())
		{
			_launchButton = new wxButton(
				panel, wxID_ANY, wxString::Format(wxString("Launch (%s)"_lng), launchHelper->getCaption()));
			_launchButton->SetBitmap(launchHelper->getIcon());

			wxSize goodSize = _launchButton->GetBestSize();
			goodSize.SetWidth(goodSize.GetHeight());
			_launchManageButton =
				new wxButton(panel, wxID_ANY, wxEmptyString, wxDefaultPosition, goodSize, wxBU_EXACTFIT);
			_launchManageButton->SetBitmap(_app.iconStorage().get(embedded_icon::cog));
			_launchManageButton->SetToolTip("Change executable for launch"_lng);
		}
	}

	auto layout = new wxBoxSizer(wxVERTICAL);

	if (_currentPlatform && _currentPlatform->launchHelper())
	{
		auto topLineSizer = new wxBoxSizer(wxHORIZONTAL);

		topLineSizer->AddStretchSpacer(1);
		topLineSizer->Add(_launchButton, wxSizerFlags(0).CenterVertical().Border(wxALL, 4));
		topLineSizer->Add(_launchManageButton, wxSizerFlags(0).CenterVertical().Border(wxALL, 4));

		layout->Add(topLineSizer, wxSizerFlags(0).Expand());
	}

	layout->Add(pages, wxSizerFlags(1).Expand());
	panel->SetSizer(layout);

	auto mainLayout = new wxBoxSizer(wxVERTICAL);
	mainLayout->Add(panel, wxSizerFlags(1).Expand());

	SetSizer(mainLayout);

	Layout();
	Maximize(app.appConfig().mainWindow().maximized);

	Bind(wxEVT_CLOSE_WINDOW, &MainFrame::OnCloseWindow, this);

	if (_currentPlatform)
	{
		if (auto launchHelper = _currentPlatform->launchHelper())
		{
			launchHelper->onDataChanged().connect([this] { updateExecutableIcon(); });
			_launchButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { onLaunchGameRequested(); });
			_launchManageButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { selectExeToLaunch(); });
		}
	}
};

void MainFrame::OnAbout()
{
	wxAboutDialogInfo aboutInfo;
	aboutInfo.SetName(PROGRAM_NAME);
	aboutInfo.SetVersion(PROGRAM_VERSION);
	aboutInfo.SetDescription("A mod manager for Era II");
	aboutInfo.SetCopyright(L"(C) 2020-2023 Aliaksei Karalenka");
	aboutInfo.SetWebSite("http://wforum.heroes35.net");
	aboutInfo.AddDeveloper("Aliaksei SyDr Karalenka");
	aboutInfo.SetLicence(ProgramLicenseText);

	wxAboutBox(aboutInfo);
}

void MainFrame::createMenuBar()
{
	auto toolsMenu = new wxMenu();
	if (!_app.appConfig().portableMode())
	{
		auto changeDirectory = toolsMenu->Append(wxID_ANY, "Change managed directory"_lng, nullptr,
			"Allows selecting other directory for management"_lng);

		_menuItems[changeDirectory->GetId()] = [&] { OnMenuToolsChangeDirectory(); };
	}

	auto reloadFromDisk = toolsMenu->Append(
		wxID_ANY, "Reload data from disk"_lng + "\tF5", nullptr, "Reload data from disk"_lng);
	_menuItems[reloadFromDisk->GetId()] = [&] { OnMenuToolsReloadDataFromDisk(); };

	auto listModFiles =
		toolsMenu->Append(wxID_ANY, "List active mod files"_lng, nullptr, "List active mod files"_lng);
	_menuItems[listModFiles->GetId()] = [&] { OnMenuToolsListModFiles(); };

	auto sortMods =
		toolsMenu->Append(wxID_ANY, "Resolve mods conflicts"_lng, nullptr, "Resolve mods conflicts"_lng);
	_menuItems[sortMods->GetId()] = [&] { OnMenuToolsSortMods(); };

	auto conflictResolveMode =
		toolsMenu->Append(wxID_ANY, "conflicts/caption"_lng, nullptr, "conflicts/caption"_lng);
	_menuItems[conflictResolveMode->GetId()] = [&] { OnMenuToolsChooseConflictResolveMode(); };

	toolsMenu->AppendSeparator();

	auto languageMenu = new wxMenu();

	for (const auto& lngCode : { "en_US", "ru_RU" })  // let's wait until someone complains
	{
		auto lngItem = languageMenu->AppendRadioItem(wxID_ANY, _app.i18nService().languageName(lngCode));
		if (_app.appConfig().currentLanguageCode() == lngCode)
			lngItem->Check();

		_menuItems[lngItem->GetId()] = [=] { OnMenuToolsLanguageSelected(lngCode); };
	}

	toolsMenu->AppendSubMenu(languageMenu, "Language"_lng, "Allows selecting other language for program"_lng);

	auto helpMenu              = new wxMenu();
	auto about                 = helpMenu->Append(wxID_ABOUT, "About"_lng, nullptr, "About"_lng);
	_menuItems[about->GetId()] = [&] { OnAbout(); };

	_mainMenu = new wxMenuBar();
	_mainMenu->Append(toolsMenu, "Tools"_lng);
	_mainMenu->Append(helpMenu, "?");
	_mainMenu->Bind(wxEVT_MENU, &MainFrame::OnMenuItemSelected, this);

	SetMenuBar(_mainMenu);
}

void MainFrame::OnMenuToolsChangeDirectory()
{
	EX_TRY;

	SelectDirectoryDialog dialog(this, _app.appConfig(), _app.iconStorage());

	if (dialog.ShowModal() != wxID_OK)
		return;

	wxBusyCursor bc;
	wxString     path = dialog.getSelectedPath();

	_app.appConfig().setDataPath(path.ToStdWstring());
	wxGetApp().scheduleRestart();

	EX_UNEXPECTED;
}

void MainFrame::OnMenuToolsReloadDataFromDisk()
{
	EX_TRY;

	if (_currentPlatform)
		_currentPlatform->reload();

	EX_UNEXPECTED;
}

void MainFrame::OnMenuToolsListModFiles()
{
	EX_TRY;

	showModFileList(
		*this, _app, *_currentPlatform->modDataProvider(), _currentPlatform->modManager()->mods());

	EX_UNEXPECTED;
}

void MainFrame::OnMenuToolsSortMods()
{
	EX_TRY;

	ResolveModConflictsView view(this, _app.appConfig(), _app.iconStorage(), *_currentPlatform);
	view.ShowModal();

	EX_UNEXPECTED;
}

void MainFrame::OnMenuToolsChooseConflictResolveMode()
{
	EX_TRY;

	ChooseConflictResolveModeView dialog(this);

	if (dialog.ShowModal() != wxID_OK)
		return;

	_currentPlatform->localConfig()->conflictResolveMode(dialog.conflictResolveMode());

	EX_UNEXPECTED;
}

void MainFrame::OnMenuChangePlatform()
{
	EX_TRY;

	SelectPlatformView dialog(this);

	if (dialog.ShowModal() != wxID_OK)
		return;

	wxBusyCursor bc;

	_app.appConfig().setSelectedPlatformCode(dialog.selectedPlatform());
	wxGetApp().scheduleRestart();

	EX_UNEXPECTED;
}

void MainFrame::OnMenuToolsLanguageSelected(const wxString& value)
{
	_app.appConfig().setCurrentLanguageCode(value.ToStdString(wxConvUTF8));
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

	if (_currentPlatform->localConfig()->conflictResolveMode() == ConflictResolveMode::undefined)
		CallAfter(&MainFrame::OnMenuToolsChooseConflictResolveMode);

	EX_ON_EXCEPTION(empty_path_error, SINK_EXCEPTION(OnMenuToolsChangeDirectory));
	EX_ON_EXCEPTION(not_exist_path_error, [](not_exist_path_error const&) {
		wxMessageOutputMessageBox().Printf("Selected path doesn't exists, please choose suitable one");
	});
	EX_UNEXPECTED;
}

void MainFrame::OnMenuCheckForUpdates()
{
	EX_TRY;

	throw std::runtime_error("Hey, you found a bug here... No... Not a bug.");

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

void MainFrame::selectExeToLaunch()
{
	EX_TRY;

	if (!_currentPlatform)
		return;

	auto config = _currentPlatform->localConfig();
	auto helper = _currentPlatform->launchHelper();

	SelectExe dialog(this, config->getDataPath(), helper->getExecutable(), _app.iconStorage());

	if (dialog.ShowModal() == wxID_OK)
		helper->setExecutable(dialog.getSelectedFile().ToStdString());

	EX_UNEXPECTED;
}

void MainFrame::onLaunchGameRequested()
{
	EX_TRY;

	if (!_currentPlatform)
		return;

	auto config = _currentPlatform->localConfig();
	auto helper = _currentPlatform->launchHelper();

	if (helper->getExecutable().empty())
		selectExeToLaunch();

	if (!helper->getExecutable().empty())
	{
		const auto currentWorkDir = wxGetCwd();

		wxSetWorkingDirectory(config->getDataPath().wstring());
		shellLaunch(helper->getLaunchString());
		wxSetWorkingDirectory(currentWorkDir);
	}

	EX_UNEXPECTED;
}

void MainFrame::updateExecutableIcon()
{
	if (!_currentPlatform)
		return;

	if (auto helper = _currentPlatform->launchHelper())
	{
		_launchButton->SetLabelText(wxString::Format(wxString("Launch (%s)"_lng), helper->getCaption()));
		_launchButton->SetBitmap(wxNullBitmap);
		_launchButton->SetBitmap(helper->getIcon());
		Layout();
	}
}
