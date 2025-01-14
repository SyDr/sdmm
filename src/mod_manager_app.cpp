// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "interface/ii18n_service.hpp"
#include "mod_manager_app.h"
#include "service/app_config.hpp"
#include "service/i18n_service.h"
#include "service/icon_storage.hpp"
#include "service/platform_service.h"
#include "system_info.hpp"
#include "type/update_check_mode.hpp"
#include "ui/main_frame.h"
#include "utility/program_update_helper.hpp"
#include "utility/sdlexcept.h"

#include <boost/locale.hpp>
#include <boost/nowide/filesystem.hpp>
#include <hash-library/md5.h>
#include <wx/app.h>
#include <wx/aui/framemanager.h>
#include <wx/cmdline.h>
#include <wx/filename.h>
#include <wx/image.h>
#include <wx/snglinst.h>
#include <wx/stdpaths.h>

using namespace mm;

ModManagerApp::ModManagerApp()
{
	SetAppDisplayName(wxString::FromUTF8(SystemInfo::ProgramVersion));
	SetAppName(wxString::FromUTF8(PROGRAM_NAME));
	wxStandardPaths::Get().IgnoreAppSubDir(L"release-static");
	wxStandardPaths::Get().IgnoreAppSubDir(L"debug-asan");
	_updateHelper = std::make_unique<UpdateCheckHelper>();
}

bool ModManagerApp::OnInit()
{
	if (!wxApp::OnInit())
		return false;

	boost::nowide::nowide_filesystem();
	boost::locale::generator lg;
	std::locale::global(lg(""));

	wxInitAllImageHandlers();
	initServices();

	_singleInstanceChecker = std::make_unique<wxSingleInstanceChecker>();

	bool alreadyRunning = false;
	if (!_appConfig->portableMode())
	{
		// use default cheking (i.e. one instance of program for user)
		alreadyRunning = _singleInstanceChecker->IsAnotherRunning();
	}
	else
	{
		const auto path = _appConfig->getDataPath();

		const auto pathHash = MD5()(path.string());

		if (_singleInstanceChecker->Create(GetAppName() + L"_" + wxString::FromUTF8(pathHash)))
			alreadyRunning = _singleInstanceChecker->IsAnotherRunning();
	}

	if (alreadyRunning)
	{
		const int answer = wxMessageBox(
			"Another copy of program is running. Running more than one copy may result in incorrect data/configuration management. Do you want to run program anyway?"_lng,
			wxTheApp->GetAppName(), wxYES_NO | wxNO_DEFAULT);

		if (answer != wxYES)
			return false;
	}

	initView();

	Bind(wxEVT_ACTIVATE_APP, [=](wxActivateEvent& event) {
		if (event.GetActive())
			_mainFrame->reloadModelIfNeeded();
	});

	bool autoCheckForUpdate = false;
	if (auto ucm = _appConfig->updateCheckMode(); ucm != UpdateCheckMode::manual)
	{
		const auto lastCheck = _appConfig->lastUpdateCheck();
		const auto now       = clock::now();
		const auto diff      = now - lastCheck;

		// TODO:
		// https://docs.github.com/en/rest/using-the-rest-api/best-practices-for-using-the-rest-api?apiVersion=2022-11-28#use-conditional-requests-if-appropriate
		// for now making delay of one hour seems to be good enough

		if (ucm == UpdateCheckMode::on_every_launch && diff >= std::chrono::hours(1))
			autoCheckForUpdate = true;
		else if (ucm == UpdateCheckMode::once_per_day && diff >= std::chrono::days(1))
			autoCheckForUpdate = true;
		else if (ucm == UpdateCheckMode::once_per_week && diff >= std::chrono::days(7))
			autoCheckForUpdate = true;
		else if (ucm == UpdateCheckMode::once_per_month && diff >= std::chrono::days(30))
			autoCheckForUpdate = true;
	}

	if (autoCheckForUpdate)
		requestUpdateCheck(true);

	return true;
}

void ModManagerApp::initView()
{
	_mainFrame = new MainFrame(*this);
	_mainFrame->Show(true);
}

void ModManagerApp::OnUnhandledException()
{
	wxString what;

	try
	{
		throw;
	}
	catch (const std::exception& e)
	{
		what = wxString::FromUTF8(e.what());
	}
	catch (...)
	{
		what = L"Unknown exception";
	}

	wxMessageOutputBest().Printf(
		L"There is no good unhandled exception handling for now. \n"
		"Program will be closed now. Sorry :( \n\n"
		"Exception info: \n%s",
		what);
}

void ModManagerApp::OnInitCmdLine(wxCmdLineParser& parser)
{
	wxApp::OnInitCmdLine(parser);

	parser.AddParam(wxEmptyString, wxCMD_LINE_VAL_STRING,
		wxCMD_LINE_PARAM_OPTIONAL);  // ignore 1 param for now (should be changed in the future)
}

int ModManagerApp::OnExit()
{
	appConfig().save();

	return wxApp::OnExit();
}

std::string ModManagerApp::translationString(const std::string& key) const
{
	return _i18nService->get(key);
}

std::string ModManagerApp::categoryTranslationString(const std::string& key) const
{
	return _i18nService->category(key);
}

IAppConfig& ModManagerApp::appConfig() const
{
	return *_appConfig;
}

II18nService& ModManagerApp::i18nService() const
{
	return *_i18nService;
}

IPlatformService& ModManagerApp::platformService() const
{
	return *_platformService;
}

void ModManagerApp::scheduleRestart()
{
	CallAfter([this] {
		wxBusyCursor bc;
		if (!_mainFrame->Close())
			return;

		_appConfig->save();
		initServices();
		initView();
	});
}

void ModManagerApp::initServices()
{
	_appConfig       = std::make_unique<AppConfig>();
	_i18nService     = std::make_unique<I18nService>(*_appConfig);
	_platformService = std::make_unique<PlatformService>(*this);
}

void ModManagerApp::requestUpdateCheck(bool automatic)
{
	_updateHelper->checkForUpdate([=](nlohmann::json update) {
		CallAfter([=] {
			_appConfig->lastUpdateCheck(clock::now());

			if (_mainFrame)
				_mainFrame->updateCheckCompleted(update, automatic);
		});
	});
}

wxString operator""_lng(const char* s, std::size_t)
{
	return wxString::FromUTF8(wxGetApp().translationString(s));
}

wxIMPLEMENT_APP(ModManagerApp);
