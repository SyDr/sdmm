// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
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
	struct IPlatformService;
	struct IIconStorage;
	class MainFrame;

	class ModManagerApp : public wxApp, public Application
	{
	public:
		ModManagerApp();

		std::string translationString(const std::string& key) const;
		std::string categoryTranslationString(const std::string& key) const;

		void scheduleRestart();

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
		wxWidgetsPtr<MainFrame> _mainFrame = nullptr;

		std::unique_ptr<wxSingleInstanceChecker> _singleInstanceChecker;

		std::unique_ptr<AppConfig> _appConfig;

		std::unique_ptr<I18nService>     _i18nService;
		std::unique_ptr<IPlatformService> _platformService;

		std::unique_ptr<IIconStorage> _iconStorage;
	};
}

wxDECLARE_APP(mm::ModManagerApp);
