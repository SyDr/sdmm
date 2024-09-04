// SD Mod Manager

// Copyright (c) 2020-2024 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "main_frame.h"

#include "application.h"
#include "choose_conflict_resolve_mode_view.hpp"
#include "interface/iapp_config.hpp"
#include "interface/ii18n_service.hpp"
#include "interface/iicon_storage.hpp"
#include "interface/ilaunch_helper.hpp"
#include "interface/ilocal_config.hpp"
#include "interface/imod_manager.hpp"
#include "interface/imod_platform.hpp"
#include "interface/iplatform_service.hpp"
#include "license.hpp"
#include "manage_preset_list_view.hpp"
#include "mod_list_view.h"
#include "mod_manager_app.h"
#include "select_directory_view.h"
#include "select_exe.h"
#include "service/icon_storage.hpp"
#include "show_file_list_dialog.hpp"
#include "show_file_list_helper.hpp"
#include "system_info.hpp"
#include "type/embedded_icon.h"
#include "type/main_window_properties.h"
#include "utility/sdlexcept.h"

#include <wx/aboutdlg.h>
#include <wx/aui/auibook.h>
#include <wx/busyinfo.h>
#include <wx/menu.h>
#include <wx/sizer.h>

using namespace mm;

namespace
{
	const wxSize minFrameSize(1240, 760);
}

MainFrame::MainFrame(Application& app)
	: wxFrame(nullptr, wxID_ANY, wxString::FromUTF8(SystemInfo::ProgramVersion),
		  app.appConfig().mainWindow().position, app.appConfig().mainWindow().size)
	, _app(app)
	, _iconStorage(std::make_unique<IconStorage>())
{
	SetIcon(wxICON(MainMMIcon));
	SetSizeHints(minFrameSize, wxDefaultSize);

	reloadModel();

	createMenuBar();
	CreateStatusBar();

	auto panel = new wxPanel(this);
	auto pages = new wxAuiNotebook(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxAUI_NB_TOP | wxAUI_NB_TAB_SPLIT | wxAUI_NB_TAB_MOVE | wxAUI_NB_SCROLL_BUTTONS);

	if (_currentPlatform)
	{
		auto modListView1 = new ModListView(pages, *_currentPlatform, *_iconStorage,
			ModListModelManagedMode::as_flat_list, ModListModelArchivedMode::as_flat_list);
		pages->AddPage(modListView1, "Mods"_lng + L" (flat, flat)");

		auto modListView2 = new ModListView(pages, *_currentPlatform, *_iconStorage,
			ModListModelManagedMode::as_flat_list, ModListModelArchivedMode::as_single_group);
		pages->AddPage(modListView2, "Mods"_lng + L" (flat, single)");

		auto modListView3 = new ModListView(pages, *_currentPlatform, *_iconStorage,
			ModListModelManagedMode::as_flat_list, ModListModelArchivedMode::as_individual_groups);
		pages->AddPage(modListView3, "Mods"_lng + L" (flat, individual)");

		auto modListView5 = new ModListView(pages, *_currentPlatform, *_iconStorage,
			ModListModelManagedMode::as_group, ModListModelArchivedMode::as_single_group);
		pages->AddPage(modListView5, "Mods"_lng + L" (group, single)");

		auto modListView6 = new ModListView(pages, *_currentPlatform, *_iconStorage,
			ModListModelManagedMode::as_group, ModListModelArchivedMode::as_individual_groups);
		pages->AddPage(modListView6, "Mods"_lng + L" (group, individual)");

		if (auto presetManager = _currentPlatform->getPresetManager())
		{
			auto presetManagerView = new ManagePresetListView(pages, *_currentPlatform, *_iconStorage);
			pages->AddPage(presetManagerView, "Profiles"_lng);
		}
	}

	auto layout = new wxBoxSizer(wxVERTICAL);

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
			launchHelper->onDataChanged().connect([this] { updateExecutableRelatedData(); });
		}
	}
}
void MainFrame::reloadModelIfNeeded()
{
	EX_TRY;

	if (_currentPlatform)
		_currentPlatform->reload();

	EX_UNEXPECTED;
};

void MainFrame::OnAbout()
{
	wxAboutDialogInfo aboutInfo;
	aboutInfo.SetName(wxString::FromUTF8(PROGRAM_NAME));
	aboutInfo.SetVersion(wxString::FromUTF8(PROGRAM_VERSION));
	aboutInfo.SetDescription(L"A mod manager for Era II");
	aboutInfo.SetCopyright(L"(C) 2020-2024 Aliaksei Karalenka");
	aboutInfo.SetWebSite(L"https://github.com/SyDr/sdmm/");
	aboutInfo.AddDeveloper(L"Aliaksei SyDr Karalenka");
	aboutInfo.SetLicence(wxString::FromUTF8(ProgramLicenseText));

	wxAboutBox(aboutInfo);
}

void MainFrame::createMenuBar()
{
	wxMenu* gameMenu = nullptr;

	if (auto launchHelper = _currentPlatform ? _currentPlatform->launchHelper() : nullptr)
	{
		gameMenu = new wxMenu();

		_launchMenuItem = gameMenu->Append(wxID_ANY,
			wxString::Format(wxString("Launch (%s)"_lng), wxString::FromUTF8(launchHelper->getCaption())),
			nullptr, "Launch game with selected executable"_lng);

		_launchMenuItem->SetBitmap(_iconStorage->get(launchHelper->getLaunchString()));

		gameMenu->AppendSeparator();

		auto launchManage = gameMenu->Append(wxID_ANY,
			wxString::Format(
				wxString("Change executable for launch"_lng), wxString::FromUTF8(launchHelper->getCaption())),
			nullptr, "Change executable for launch"_lng);

		launchManage->SetBitmap(_iconStorage->get(embedded_icon::cog));

		_menuItems[_launchMenuItem->GetId()] = [&] { onLaunchGameRequested(); };
		_menuItems[launchManage->GetId()]    = [&] { selectExeToLaunch(); };
	}

	auto toolsMenu = new wxMenu();
	if (!_app.appConfig().portableMode())
	{
		auto changeDirectory = toolsMenu->Append(wxID_ANY, "Change managed directory"_lng, nullptr,
			"Allows selecting other directory for management"_lng);

		_menuItems[changeDirectory->GetId()] = [&] { OnMenuToolsChangeDirectory(); };
	}

	auto reloadFromDisk = toolsMenu->Append(
		wxID_ANY, "Reload data from disk"_lng + L"\tF5", nullptr, "Reload data from disk"_lng);
	_menuItems[reloadFromDisk->GetId()] = [&] { OnMenuToolsReloadDataFromDisk(); };

	auto listModFiles =
		toolsMenu->Append(wxID_ANY, "List active mod files"_lng, nullptr, "List active mod files"_lng);
	_menuItems[listModFiles->GetId()] = [&] { OnMenuToolsListModFiles(); };

	auto conflictResolveMode =
		toolsMenu->Append(wxID_ANY, "conflicts/caption"_lng, nullptr, "conflicts/caption"_lng);
	_menuItems[conflictResolveMode->GetId()] = [&] { OnMenuToolsChooseConflictResolveMode(); };

	toolsMenu->AppendSeparator();

	auto languageMenu = new wxMenu();

	for (const auto& lngCode : _app.i18nService().available())
	{
		auto lngItem = languageMenu->AppendRadioItem(
			wxID_ANY, wxString::FromUTF8(_app.i18nService().languageName(lngCode)));
		if (_app.appConfig().currentLanguageCode() == lngCode)
			lngItem->Check();

		_menuItems[lngItem->GetId()] = [=] { OnMenuToolsLanguageSelected(lngCode); };
	}

	toolsMenu->AppendSubMenu(languageMenu, "Language"_lng, "Allows selecting other language for program"_lng);

	auto helpMenu              = new wxMenu();
	auto about                 = helpMenu->Append(wxID_ABOUT, "About"_lng, nullptr, "About"_lng);
	_menuItems[about->GetId()] = [&] { OnAbout(); };

	_mainMenu = new wxMenuBar();
	if (gameMenu)
		_mainMenu->Append(gameMenu, "Game"_lng);
	_mainMenu->Append(toolsMenu, "Tools"_lng);
	_mainMenu->Append(helpMenu, L"?");
	_mainMenu->Bind(wxEVT_MENU, &MainFrame::OnMenuItemSelected, this);

	SetMenuBar(_mainMenu);
}

void MainFrame::OnMenuToolsChangeDirectory()
{
	EX_TRY;

	SelectDirectoryDialog dialog(this, _app.appConfig(), *_iconStorage);

	if (dialog.ShowModal() != wxID_OK)
		return;

	wxBusyCursor bc;
	wxString     path = dialog.getSelectedPath();

	_app.appConfig().setDataPath(path.ToStdString(wxConvUTF8));
	wxGetApp().scheduleRestart();

	EX_UNEXPECTED;
}

void MainFrame::OnMenuToolsReloadDataFromDisk()
{
	EX_TRY;

	if (_currentPlatform)
		_currentPlatform->reload(true);

	EX_UNEXPECTED;
}

void MainFrame::OnMenuToolsListModFiles()
{
	EX_TRY;

	showModFileList(
		*this, *_iconStorage, *_currentPlatform->modDataProvider(), _currentPlatform->modManager()->mods());

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

void MainFrame::OnMenuToolsLanguageSelected(const std::string& value)
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

	EX_ON_EXCEPTION(empty_path_error, SINK_EXCEPTION(OnMenuToolsChangeDirectory));
	EX_ON_EXCEPTION(not_exist_path_error, [](not_exist_path_error const&) {
		wxMessageOutputMessageBox().Printf(L"Selected path doesn't exists, please choose suitable one");
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

	SelectExe dialog(this, config->getDataPath(), wxString::FromUTF8(helper->getExecutable()), *_iconStorage);

	if (dialog.ShowModal() == wxID_OK)
		helper->setExecutable(dialog.getSelectedFile().ToStdString(wxConvUTF8));

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
		wxExecute(wxString::FromUTF8('\"' + helper->getLaunchString()) + '\"');
		wxSetWorkingDirectory(currentWorkDir);
	}

	EX_UNEXPECTED;
}

void MainFrame::updateExecutableRelatedData()
{
	if (!_currentPlatform)
		return;

	if (auto helper = _currentPlatform->launchHelper())
	{
		auto text = wxString::Format(wxString("Launch (%s)"_lng), wxString::FromUTF8(helper->getCaption()));
		auto icon = _iconStorage->get(helper->getLaunchString());

		_launchMenuItem->SetItemLabel(text);
		_launchMenuItem->SetBitmap(icon);

		Layout();
	}
}
