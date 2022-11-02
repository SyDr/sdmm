// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <wx/app.h>

#include "application.h"
#include "utility/wx_widgets_ptr.hpp"

class wxSingleInstanceChecker;

namespace mm
{
	struct AppConfig;
	struct I18nService;
	class PlatformService;
	class IconStorage;

	class ModManagerApp : public wxApp, public Application
	{
	public:
		ModManagerApp();

		wxString translationString(const wxString& key);
		wxString categoryTranslationString(const wxString& key) const;
		void     scheduleRestart();

		bool OnInit() override;

		void initServices();

		int OnExit() override;

		void OnUnhandledException() override;

		IAppConfig&       appConfig() const override;
		II18nService&     i18nService() const override;
		IPlatformService& platformService() const override;
		IIconStorage&     iconStorage() const override;

	private:
		void initView();

	private:
		wxWidgetsPtr<wxFrame>      _mainFrame = nullptr;

		std::unique_ptr<wxSingleInstanceChecker> _singleInstanceChecker;

		std::unique_ptr<AppConfig> _appConfig;

		std::unique_ptr<I18nService>     _i18nService;
		std::unique_ptr<PlatformService> _platformService;

		std::unique_ptr<IconStorage> _iconStorage = std::make_unique<IconStorage>();
	};
}

wxDECLARE_APP(mm::ModManagerApp);
