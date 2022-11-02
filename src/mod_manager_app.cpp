// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "impl/service/app_config.h"
#include "impl/service/i18n_service.h"
#include "impl/service/icon_storage.h"
#include "impl/service/platform_service.h"
#include "interface/iapp_config.h"
#include "mod_manager_app.h"
#include "service/ii18n_service.hpp"
#include "system_info.hpp"
#include "ui/main_frame.h"
#include "utility/sdlexcept.h"

#include <wx/app.h>
#include <wx/aui/framemanager.h>
#include <wx/filename.h>
#include <wx/image.h>
#include <wx/snglinst.h>
#include <wx/stdpaths.h>

using namespace mm;

ModManagerApp::ModManagerApp()
{
	SetAppDisplayName(constant::program_full_version);
	SetAppName(PROGRAM_NAME);
}

bool ModManagerApp::OnInit()
{
	if (!wxApp::OnInit())
		return false;

	wxInitAllImageHandlers();

	initServices();

	_singleInstanceChecker = std::make_unique<wxSingleInstanceChecker>();
	if (_singleInstanceChecker->IsAnotherRunning())
	{
		wxLogError("Application is already running...");
		return false;
	}

	initView();

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
	catch (std::exception const& e)
	{
		what = wxString::FromUTF8(e.what());
	}
	catch (...)
	{
		what = "Unknown exception";
	}

	wxMessageOutputBest().Printf(
		"There is no good unhandled exception handling for now. \n"
		"Program will be closed now. Sorry :( \n\n"
		"Exception info: \n%s",
		what);
}

int ModManagerApp::OnExit()
{
	appConfig().save();

	return wxApp::OnExit();
}

wxString ModManagerApp::translationString(const wxString& key)
{
	return _i18nService->get(key);
}

wxString ModManagerApp::categoryTranslationString(const wxString& key) const
{
	return _i18nService->category(key.ToStdString());
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

IIconStorage& ModManagerApp::iconStorage() const
{
	return *_iconStorage;
}

void ModManagerApp::scheduleRestart()
{
	CallAfter(
		[this]
		{
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

wxString operator""_lng(const char* s, std::size_t)
{
	return wxGetApp().translationString(s);
}

wxIMPLEMENT_APP(ModManagerApp);
