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
#include "ui/main_frame.h"
#include "utility/program_update_helper.hpp"
#include "utility/sdlexcept.h"

#include <boost/locale.hpp>
#include <boost/nowide/filesystem.hpp>
#include <wx/app.h>
#include <wx/aui/framemanager.h>
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
	if (_singleInstanceChecker->IsAnotherRunning())
	{
		wxLogError(L"Application is already running...");
		return false;
	}

	initView();

	Bind(wxEVT_ACTIVATE_APP, [=](wxActivateEvent& event) {
		if (event.GetActive())
			_mainFrame->reloadModelIfNeeded();
	});

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

void ModManagerApp::requestUpdateCheck()
{
	_updateHelper->checkForUpdate([=](nlohmann::json update) {
		CallAfter([=] {
			if (update["tag_name"] == PROGRAM_VERSION_TAG)
				return;

			const int answer = wxMessageBox("New program version is available. Open release page?"_lng,
				"New version available"_lng, wxYES_NO);
			if (answer == wxYES)
				wxLaunchDefaultBrowser(wxString::FromUTF8(update["html_url"].get<std::string>()));
		});
	});
}

wxString operator""_lng(const char* s, std::size_t)
{
	return wxString::FromUTF8(wxGetApp().translationString(s));
}

wxIMPLEMENT_APP(ModManagerApp);
