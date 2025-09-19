// SD Mod Manager

// Copyright (c) 2020-2024 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "main_frame.h"

#include "application.h"
#include "application_settings_dialog.h"
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
#include "system_info.hpp"
#include "type/icon.hpp"
#include "type/main_window_properties.h"
#include "utility/sdlexcept.h"
#include "utility/wx_current_dir_helper.hpp"
#include "edit_mod_dialog.hpp"
#include "enter_file_name.hpp"

#include <wx/aboutdlg.h>
#include <wx/aui/auibook.h>
#include <wx/busyinfo.h>
#include <wx/infobar.h>
#include <wx/menu.h>
#include <wx/sizer.h>
#include <wx/notifmsg.h>

using namespace mm;

namespace
{
	const wxSize minFrameSize(1240, 760);
}

MainFrame::MainFrame(Application& app)
	: wxFrame(nullptr, wxID_ANY, wxString::FromUTF8(SystemInfo::ProgramVersion),
		  app.appConfig().mainWindow().position, app.appConfig().mainWindow().size)
	, _app(app)
	, _iconStorage(std::make_unique<IconStorage>(app.appConfig().interfaceSize()))
{
	SetIcon(wxICON(MainMMIcon));
	SetSizeHints(minFrameSize, wxDefaultSize);

	reloadModel();

	createMenuBar();
	_statusBar = CreateStatusBar();

	_infoBar = new wxInfoBar(this);
	_infoBarTimer.SetOwner(this);

	auto panel = new wxPanel(this);
	auto pages = new wxNotebook(panel, wxID_ANY);

	if (_currentPlatform)
	{
		auto modListView = new ModListView(pages, *_currentPlatform, *_iconStorage, _statusBar);
		pages->AddPage(modListView, "Mods"_lng);

		if (auto presetManager = _currentPlatform->getPresetManager())
		{
			auto presetManagerView = new ManagePresetListView(pages, *_currentPlatform, *_iconStorage);
			pages->AddPage(presetManagerView, "Profiles"_lng);
		}

		modListView->SetFocus();
	}

	auto layout = new wxBoxSizer(wxVERTICAL);

	layout->Add(pages, wxSizerFlags(1).Expand());
	panel->SetSizer(layout);

	auto mainLayout = new wxBoxSizer(wxVERTICAL);
	mainLayout->Add(panel, wxSizerFlags(1).Expand());
	mainLayout->Add(_infoBar, wxSizerFlags(0).Expand());

	SetSizer(mainLayout);

	Layout();
	Maximize(app.appConfig().mainWindow().maximized);

	Bind(wxEVT_CLOSE_WINDOW, &MainFrame::OnCloseWindow, this);
	Bind(wxEVT_TIMER, [=](wxTimerEvent&) { _infoBar->Dismiss(); });

	_infoBar->Bind(wxEVT_BUTTON, [=](wxCommandEvent& event) {
		if (event.GetId() == wxID_OPEN)
			wxLaunchDefaultBrowser(L"https://github.com/SyDr/sdmm/releases/latest");
		else if (event.GetId() == wxID_DOWN)
			wxLaunchDefaultBrowser(_newVersionUrl);

		event.Skip();
	});

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
	aboutInfo.SetDescription(L"A mod manager for ERA platform");
	aboutInfo.SetCopyright(L"(C) 2020-2025 Aliaksei Karalenka");
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

		_launchMenuItem->SetBitmap(
			_iconStorage->get(launchHelper->getLaunchString(), Icon::Size::x16));

		gameMenu->AppendSeparator();

		auto launchManage = gameMenu->Append(wxID_ANY,
			wxString::Format(
				wxString("Change executable for launch"_lng), wxString::FromUTF8(launchHelper->getCaption())),
			nullptr, "Change executable for launch"_lng);

		launchManage->SetBitmap(_iconStorage->get(Icon::Stock::cog, Icon::Size::x16));

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

	auto createNewMod =
		toolsMenu->Append(wxID_ANY, "Create new mod"_lng, nullptr, "Create new mod"_lng);
	_menuItems[createNewMod->GetId()] = [&] { OnMenuToolsCreateNewMod(); };

	toolsMenu->AppendSeparator();

	auto conflictResolveMode =
		toolsMenu->Append(wxID_ANY, "conflicts/caption"_lng, nullptr, "conflicts/caption"_lng);
	_menuItems[conflictResolveMode->GetId()] = [&] { OnMenuToolsChooseConflictResolveMode(); };

	auto changeProgramSettings =
		toolsMenu->Append(wxID_ANY, "Settings"_lng, nullptr, "Change program settings"_lng);
	_menuItems[changeProgramSettings->GetId()] = [&] { OnMenuToolsChangeSettings(); };

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

	auto helpMenu = new wxMenu();

	auto updateCheck =
		helpMenu->Append(wxID_ANY, "Check for updates"_lng, nullptr, "Check for program updates"_lng);
	_menuItems[updateCheck->GetId()] = [&] { OnMenuCheckForUpdates(); };

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

void MainFrame::OnMenuToolsChangeSettings()
{
	EX_TRY;

	ApplicationSettingsDialog asd(this, _app);
	if (asd.ShowModal() == wxID_APPLY)
		wxGetApp().scheduleRestart();

	EX_UNEXPECTED;
}

void MainFrame::OnMenuToolsListModFiles()
{
	EX_TRY;

	ShowFileListDialog sfld(this, *_iconStorage, *_currentPlatform->modDataProvider(),
		_currentPlatform->modManager()->mods(), _currentPlatform->managedPath());
	sfld.ShowModal();

	EX_UNEXPECTED;
}

void MainFrame::OnMenuToolsCreateNewMod()
{
	EX_TRY;

	wxString modName;
	while (true)
	{
		modName = enterFileName(this, "Enter new mod name"_lng, "Create"_lng, modName);
		if (modName.empty())
			return;

		const auto targetPath = _currentPlatform->managedPath() / "Mods" / modName.ToUTF8();

		if (fs::exists(targetPath))
		{
			wxNotificationMessage nm(wxEmptyString, "Directory already exist"_lng, this);
			nm.Show();
			continue;
		}

		fs::create_directory(targetPath);
		fs::copy_file(_app.appConfig().programPath() / SystemInfo::DataDir / SystemInfo::ModInfoFilename,
				_currentPlatform->managedPath() / "Mods" / modName.ToUTF8() / SystemInfo::ModInfoFilename);

		_currentPlatform->reload();

		_currentPlatform->modManager()->enable(modName.ToStdString(wxConvUTF8));

		break;
	}

	EditModDialog cnmd(
		this, *_currentPlatform, modName.ToStdString(wxConvUTF8));
	cnmd.ShowModal();

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

	wxGetApp().requestUpdateCheck();

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
		CurrentDirHelper cdh(config->getDataPath().wstring());
		wxExecute(wxString::FromUTF8('\"' + helper->getLaunchString()) + '\"');
	}

	EX_UNEXPECTED;
}

void MainFrame::updateCheckCompleted(const nlohmann::json& value, bool automatic)
{
	for (const auto& id : { wxID_DOWN, wxID_OPEN, wxID_CLOSE })
		if (_infoBar->HasButtonId(id))
			_infoBar->RemoveButton(id);

	if (value.is_discarded() || value.is_null())
	{
		_infoBar->AddButton(wxID_OPEN, "Open page"_lng);
		_infoBar->AddButton(wxID_CLOSE, "Close"_lng);

		_infoBar->ShowMessage("Cannot check for program update"_lng);
		_infoBarTimer.StartOnce(10000);
		return;
	}

	if (value["tag_name"] == PROGRAM_VERSION_TAG)
	{
		if (!automatic)
			_infoBar->ShowMessage("You have latest program version"_lng);
		_infoBarTimer.StartOnce(5000);
		return;
	}

	for (const auto& item : value["assets"])
	{
		static const auto ending = _app.appConfig().portableMode() ? ".zip" : ".exe";

		const auto url = item["browser_download_url"].get<std::string>();
		if (url.ends_with(ending))
		{
			_newVersionUrl = wxString::FromUTF8(url);
			break;
		}
	}

	_infoBar->AddButton(wxID_DOWN, "Download"_lng);
	_infoBar->AddButton(wxID_OPEN, "Open page"_lng);
	_infoBar->AddButton(wxID_CLOSE, "Close"_lng);
	_infoBar->ShowMessage("New program version is available"_lng);
}

void MainFrame::updateExecutableRelatedData()
{
	if (!_currentPlatform)
		return;

	if (auto helper = _currentPlatform->launchHelper())
	{
		auto text = wxString::Format(wxString("Launch (%s)"_lng), wxString::FromUTF8(helper->getCaption()));
		auto icon = _iconStorage->get(helper->getLaunchString(), Icon::Size::x16);

		_launchMenuItem->SetItemLabel(text);
		_launchMenuItem->SetBitmap(icon);

		Layout();
	}
}
