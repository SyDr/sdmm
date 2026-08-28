// SD Mod Manager

// Copyright (c) 2020-2025 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "main_frame.h"

#include "application.h"
#include "application_settings_dialog.h"
#include "edit_mod_dialog.hpp"
#include "enter_file_name.hpp"
#include "interface/iapp_config.hpp"
#include "interface/ii18n_service.hpp"
#include "interface/iicon_storage.hpp"
#include "interface/ilaunch_helper.hpp"
#include "interface/ilocal_config.hpp"
#include "interface/imod_data_provider.hpp"
#include "interface/imod_manager.hpp"
#include "interface/imod_package_installer.hpp"
#include "interface/imod_platform.hpp"
#include "interface/iplatform_service.hpp"
#include "license.hpp"
#include "manage_preset_list_view.hpp"
#include "mod_list_view.h"
#include "mod_package_install_dialog.hpp"
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
#include "type/program_version.hpp"

#include <wx/aboutdlg.h>
#include <wx/aui/auibook.h>
#include <wx/busyinfo.h>
#include <wx/dnd.h>
#include <wx/filedlg.h>
#include <wx/infobar.h>
#include <wx/menu.h>
#include <wx/notifmsg.h>
#include <wx/progdlg.h>
#include <wx/sizer.h>

using namespace mm;

namespace
{
	const wxSize minFrameSize(1240, 760);

	bool supportedModPackage(const fs::path& path)
	{
		auto extension = boost::nowide::narrow(path.extension().wstring());
		boost::algorithm::to_lower(extension);
		return extension == ".exe" || extension == ".rar" || extension == ".zip" || extension == ".7z";
	}

	wxString packageProgressMessage(const ModPackageProgress& progress)
	{
		const auto item = wxString::FromUTF8(progress.item);
		switch (progress.phase)
		{
		case ModPackageProgressPhase::scanning:
			return wxString::Format("dialog/package_installer/progress/scanning"_lng, item);
		case ModPackageProgressPhase::extracting:
			return wxString::Format("dialog/package_installer/progress/extracting"_lng, item);
		case ModPackageProgressPhase::validating:
			return wxString::Format("dialog/package_installer/progress/validating"_lng, item);
		case ModPackageProgressPhase::staging:
			return wxString::Format("dialog/package_installer/progress/staging"_lng, item);
		case ModPackageProgressPhase::committing:
			return "dialog/package_installer/progress/committing"_lng;
		case ModPackageProgressPhase::rolling_back:
			return "dialog/package_installer/progress/rolling_back"_lng;
		}
		return {};
	}
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
	_mainPages = new wxNotebook(panel, wxID_ANY);

	if (_currentPlatform)
	{
		_modListView = new ModListView(_mainPages, *_currentPlatform, *_iconStorage, _statusBar);
		_mainPages->AddPage(_modListView, "dialog/main_frame/page_mods"_lng);

		if (auto presetManager = _currentPlatform->getPresetManager())
		{
			auto presetManagerView = new ManagePresetListView(_mainPages, *_currentPlatform, *_iconStorage);
			_mainPages->AddPage(presetManagerView, "dialog/main_frame/page_profiles"_lng);
		}

		_modListView->SetFocus();
	}

	auto layout = new wxBoxSizer(wxVERTICAL);

	layout->Add(_mainPages, wxSizerFlags(1).Expand());
	panel->SetSizer(layout);

	auto mainLayout = new wxBoxSizer(wxVERTICAL);
	mainLayout->Add(panel, wxSizerFlags(1).Expand());
	mainLayout->Add(_infoBar, wxSizerFlags(0).Expand());

	SetSizer(mainLayout);

	Layout();
	Maximize(app.appConfig().mainWindow().maximized);

	Bind(wxEVT_CLOSE_WINDOW, &MainFrame::OnCloseWindow, this);
	Bind(wxEVT_DROP_FILES, &MainFrame::OnFilesDropped, this);
	DragAcceptFiles(true);
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

MainFrame::~MainFrame()
{
	// The pages reference the platform and icon services owned by this frame. Destroy them
	// while those services are still alive so native control teardown cannot call into stale data.
	if (_mainPages)
	{
		_mainPages->DeleteAllPages();
		_modListView = nullptr;
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
	aboutInfo.SetDescription("dialog/main_frame/about_description"_lng);
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
			wxString::Format(
				wxString("dialog/main_frame/menu/game/launch"_lng), wxString::FromUTF8(launchHelper->getCaption())),
			nullptr, "dialog/main_frame/menu/game/launch_tip"_lng);

		_launchMenuItem->SetBitmap(_iconStorage->get(launchHelper->getLaunchString(), Icon::Size::x16));

		gameMenu->AppendSeparator();

		auto launchManage = gameMenu->Append(wxID_ANY,
			wxString::Format(wxString("dialog/main_frame/menu/game/change_selected"_lng),
				wxString::FromUTF8(launchHelper->getCaption())),
			nullptr, "dialog/main_frame/menu/game/change_selected"_lng);

		launchManage->SetBitmap(_iconStorage->get(Icon::Stock::cog, Icon::Size::x16));

		_menuItems[_launchMenuItem->GetId()] = [&] { onLaunchGameRequested(); };
		_menuItems[launchManage->GetId()]    = [&] { selectExeToLaunch(); };
	}

	wxMenu* modMenu = nullptr;

	if (_currentPlatform)
	{
		modMenu = new wxMenu();

		auto addNewMods = modMenu->Append(wxID_ANY,
			"dialog/main_frame/menu/mod/add_new_mods"_lng, nullptr,
			"dialog/main_frame/menu/mod/add_new_mods_tip"_lng);
		_menuItems[addNewMods->GetId()] = [&] { OnMenuModAddNewMods(); };

		auto createNewMod = modMenu->Append(wxID_ANY, "dialog/main_frame/menu/mod/create_new"_lng, nullptr,
			"dialog/main_frame/menu/mod/create_new"_lng);
		_menuItems[createNewMod->GetId()] = [&] { OnMenuModCreateNewMod(); };

		modMenu->AppendSeparator();

		auto listModFiles = modMenu->Append(wxID_ANY, "dialog/main_frame/menu/mod/list_files"_lng, nullptr,
			"dialog/main_frame/menu/mod/list_files"_lng);
		_menuItems[listModFiles->GetId()] = [&] { OnMenuModListModFiles(); };

		auto openModsFolder = modMenu->Append(wxID_ANY, "dialog/main_frame/menu/mod/open_directory"_lng,
			nullptr, "dialog/main_frame/menu/mod/open_directory"_lng);
		_menuItems[openModsFolder->GetId()] = [&] { OnMenuModOpenModFolder(); };
	}

	auto toolsMenu = new wxMenu();
	if (!_app.appConfig().portableMode())
	{
		auto changeDirectory = toolsMenu->Append(wxID_ANY, "dialog/main_frame/menu/tools/change_managed_directory"_lng,
			nullptr, "dialog/main_frame/menu/tools/change_managed_directory_tip"_lng);

		_menuItems[changeDirectory->GetId()] = [&] { OnMenuToolsChangeDirectory(); };
	}

	if (_currentPlatform)
	{
		auto reloadFromDisk =
			toolsMenu->Append(wxID_ANY, "dialog/main_frame/menu/tools/reload_data"_lng + L"\tF5", nullptr,
				"dialog/main_frame/menu/tools/reload_data"_lng);
		_menuItems[reloadFromDisk->GetId()] = [&] { OnMenuToolsReloadDataFromDisk(); };
	}

	toolsMenu->AppendSeparator();

	auto changeProgramSettings = toolsMenu->Append(
		wxID_ANY, "dialog/main_frame/menu/tools/settings"_lng, nullptr, "dialog/main_frame/menu/tools/settings_tip"_lng);
	_menuItems[changeProgramSettings->GetId()] = [&] { OnMenuToolsChangeSettings(); };

	auto languageMenu = new wxMenu();

	for (const auto& lngCode : _app.i18nService().available())
	{
		auto lngItem = languageMenu->AppendRadioItem(
			wxID_ANY, wxString::FromUTF8(_app.i18nService().languageName(lngCode)));
		if (_app.appConfig().currentLanguageCode() == lngCode)
			lngItem->Check();

		_menuItems[lngItem->GetId()] = [=] { OnMenuToolsLanguageSelected(lngCode); };
	}

	toolsMenu->AppendSubMenu(
		languageMenu, "dialog/main_frame/menu/tools/language"_lng, "dialog/main_frame/menu/tools/language_tip"_lng);

	auto helpMenu = new wxMenu();

	auto updateCheck = helpMenu->Append(
		wxID_ANY, "dialog/main_frame/menu/about/check_update"_lng, nullptr, "dialog/main_frame/menu/about/check_update_tip"_lng);
	_menuItems[updateCheck->GetId()] = [&] { OnMenuCheckForUpdates(); };

	auto about =
		helpMenu->Append(wxID_ABOUT, "dialog/main_frame/menu/about/about"_lng, nullptr, "dialog/main_frame/menu/about/about"_lng);
	_menuItems[about->GetId()] = [&] { OnAbout(); };

	_mainMenu = new wxMenuBar();
	if (gameMenu)
		_mainMenu->Append(gameMenu, "dialog/main_frame/menu/game/label"_lng);
	if (modMenu)
		_mainMenu->Append(modMenu, "dialog/main_frame/menu/mod/label"_lng);
	_mainMenu->Append(toolsMenu, "dialog/main_frame/menu/tools/label"_lng);
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

	ApplicationSettingsDialog asd(this, _app, _currentPlatform ? _currentPlatform->localConfig() : nullptr);
	if (asd.ShowModal() == wxID_APPLY)
		wxGetApp().scheduleRestart();

	EX_UNEXPECTED;
}

void MainFrame::OnMenuModAddNewMods()
{
	if (!_currentPlatform || !_currentPlatform->modPackageInstaller())
		return;

	wxFileDialog dialog(this, "dialog/package_installer/select_caption"_lng, wxEmptyString,
		wxEmptyString, "dialog/package_installer/file_filter"_lng,
		wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
	if (dialog.ShowModal() != wxID_OK)
		return;

	wxArrayString selected;
	dialog.GetPaths(selected);
	std::vector<fs::path> packages;
	packages.reserve(selected.size());
	for (const auto& path : selected)
		packages.emplace_back(path.ToStdWstring());

	importModPackages(packages);
}

void MainFrame::OnFilesDropped(wxDropFilesEvent& event)
{
	if (!_currentPlatform || !_currentPlatform->modPackageInstaller())
		return;

	std::vector<fs::path> packages;
	packages.reserve(event.GetNumberOfFiles());
	for (int index = 0; index < event.GetNumberOfFiles(); ++index)
	{
		fs::path path(event.GetFiles()[index].ToStdWstring());
		if (supportedModPackage(path))
			packages.emplace_back(std::move(path));
	}

	if (packages.empty())
	{
		wxMessageBox("message/error/no_supported_mod_packages"_lng, wxTheApp->GetAppName(),
			wxOK | wxICON_ERROR, this);
		return;
	}

	importModPackages(packages);
}

void MainFrame::importModPackages(const std::vector<fs::path>& packages)
{
	auto installer = _currentPlatform ? _currentPlatform->modPackageInstaller() : nullptr;
	if (!installer || packages.empty())
		return;

	try
	{
		ModPackageInspection inspection;
		{
			wxProgressDialog progress("dialog/package_installer/progress/caption"_lng,
				"dialog/package_installer/progress/preparing"_lng, 100, this,
				wxPD_APP_MODAL | wxPD_CAN_ABORT | wxPD_ELAPSED_TIME | wxPD_AUTO_HIDE);
			auto callback = [&](const ModPackageProgress& state) {
				if (auto cancel = progress.FindWindow(wxID_CANCEL))
					cancel->Enable(state.cancellable);
				bool skip = false;
				return progress.Pulse(packageProgressMessage(state), &skip);
			};
			inspection = installer->inspect(packages, callback);
		}

		try
		{
			ModPackageInstallDialog review(this, inspection);
			if (review.ShowModal() != wxID_OK)
			{
				installer->discard(inspection);
				return;
			}

			const auto selected = review.selectedMods();
			if (selected.empty())
			{
				installer->discard(inspection);
				return;
			}

			ModPackageInstallResult result;
			{
				wxProgressDialog progress("dialog/package_installer/progress/caption"_lng,
					"dialog/package_installer/progress/preparing"_lng, 100, this,
					wxPD_APP_MODAL | wxPD_CAN_ABORT | wxPD_ELAPSED_TIME | wxPD_AUTO_HIDE);
				auto callback = [&](const ModPackageProgress& state) {
					if (auto cancel = progress.FindWindow(wxID_CANCEL))
						cancel->Enable(state.cancellable);
					bool skip = false;
					return progress.Pulse(packageProgressMessage(state), &skip);
				};
				result = installer->install(inspection, selected, callback);
			}

			if (!result.installed_mod_ids.empty() && _modListView)
			{
				_mainPages->SetSelection(0);
				_modListView->selectMod(result.installed_mod_ids.front());
			}

			std::vector<std::string> names;
			for (const auto& id : result.installed_mod_ids)
				names.emplace_back(_currentPlatform->modDataProvider()->modData(id).name);
			wxMessageBox(wxString::Format("message/info/mod_packages_installed"_lng,
				static_cast<int>(names.size()), wxString::FromUTF8(boost::join(names, ", "))),
				wxTheApp->GetAppName(), wxOK | wxICON_INFORMATION, this);
		}
		catch (...)
		{
			installer->discard(inspection);
			throw;
		}
	}
	catch (const ModPackageOperationCancelled&)
	{}
	catch (const std::exception& error)
	{
		wxMessageBox(wxString::Format("message/error/package_install_failed"_lng,
			wxStringFromUnspecified(error.what())), wxTheApp->GetAppName(), wxOK | wxICON_ERROR, this);
	}
}

void MainFrame::OnMenuModListModFiles()
{
	EX_TRY;

	ShowFileListDialog sfld(this, *_iconStorage, *_currentPlatform->modDataProvider(),
		_currentPlatform->modManager()->mods(), _currentPlatform->managedPath());
	sfld.ShowModal();

	EX_UNEXPECTED;
}

void MainFrame::OnMenuModOpenModFolder()
{
	EX_TRY;

	wxLaunchDefaultApplication(wxString::FromUTF8(_currentPlatform->modsDirPath().string()));

	EX_UNEXPECTED;
}

void MainFrame::OnMenuModCreateNewMod()
{
	EX_TRY;

	wxString modName;
	while (true)
	{
		modName = enterFileName(this, "dialog/main_frame/new_mod_prompt"_lng, "dialog/main_frame/new_mod_prompt_label"_lng, modName);
		if (modName.empty())
			return;

		const auto targetPath = _currentPlatform->managedPath() / "Mods" / modName.ToUTF8();

		if (fs::exists(targetPath))
		{
			wxNotificationMessage nm(wxEmptyString, "message/error/directory_already_exist"_lng, this);
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

	EditModDialog cnmd(this, *_currentPlatform, modName.ToStdString(wxConvUTF8));
	cnmd.ShowModal();

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
	EX_ON_EXCEPTION(not_exist_path_error, [this](const not_exist_path_error&) {
		wxMessageOutputMessageBox().Printf(
			"message/error/path_not_exists"_lng, wxString::FromUTF8(_app.appConfig().getDataPath().string()));
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
		_infoBar->AddButton(wxID_OPEN, "dialog/button/open_page"_lng);
		_infoBar->AddButton(wxID_CLOSE, "dialog/button/close"_lng);

		_infoBar->ShowMessage("message/notification/cannot_check_for_update"_lng);
		_infoBarTimer.StartOnce(10000);
		return;
	}

	ProgramVersion pv(value["tag_name"].get<std::string>());

	if (pv <= ProgramVersion::current())
	{
		if (!automatic)
		{
			_infoBar->ShowMessage("message/notification/you_have_latest_version"_lng);
			_infoBarTimer.StartOnce(5000);
		}
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

	_infoBar->AddButton(wxID_DOWN, "dialog/button/download"_lng);
	_infoBar->AddButton(wxID_OPEN, "dialog/button/open_page"_lng);
	_infoBar->AddButton(wxID_CLOSE, "dialog/button/close"_lng);

	_infoBar->ShowMessage("message/notification/new_version_available"_lng);
}

void MainFrame::updateExecutableRelatedData()
{
	if (!_currentPlatform)
		return;

	if (auto helper = _currentPlatform->launchHelper())
	{
		auto text =
			wxString::Format(wxString("dialog/main_frame/menu/game/launch"_lng), wxString::FromUTF8(helper->getCaption()));
		auto icon = _iconStorage->get(helper->getLaunchString(), Icon::Size::x16);

		_launchMenuItem->SetItemLabel(text);
		_launchMenuItem->SetBitmap(icon);

		Layout();
	}
}
