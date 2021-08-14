// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "interface/iapp_config.h"

#include <nlohmann/json.hpp>

#include <variant>

namespace mm
{
	struct PortableMode
	{
		fspath managedPath;
	};

	struct MainMode
	{
		fspath programDataPath;
	};

	struct AppConfig : IAppConfig
	{
		AppConfig();

		bool portableMode() const override;
		fspath dataPath() const override;
		fspath programPath() const override;

		void save() override;

		auto currentLanguageCode() const -> std::string override;
		void setCurrentLanguageCode(const wxString& lngCode) override;

		wxString selectedPlatform() const override;
		void setSelectedPlatformCode(const wxString& newPlatform) override;

		fspath getDataPath() const override;
		void setDataPath(const fspath& path) override;
		void forgetDataPath(const fspath& path) override;

		std::vector<fspath> getKnownDataPathList() const override;

		void setMainWindowProperties(const MainWindowProperties& props) override;
		MainWindowProperties mainWindow() const override;

		bool dataPathHasStar(const fspath& path) const override;
		void starDataPath(const fspath& path, bool star = true) override;
		void unstarDataPath(const fspath& path) override;

	private:
		fspath configFilePath() const;
		void validate();

	private:
		std::variant<PortableMode, MainMode> _mode;

		nlohmann::json _data;
	};
}
