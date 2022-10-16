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
		fs::path managedPath;
	};

	struct MainMode
	{
		fs::path programDataPath;
	};

	struct AppConfig : IAppConfig
	{
		AppConfig();

		bool portableMode() const override;
		fs::path dataPath() const override;
		fs::path programPath() const override;

		void save() override;

		auto currentLanguageCode() const -> std::string override;
		void setCurrentLanguageCode(const wxString& lngCode) override;

		wxString selectedPlatform() const override;
		void setSelectedPlatformCode(const wxString& newPlatform) override;

		fs::path getDataPath() const override;
		void setDataPath(const fs::path& path) override;
		void forgetDataPath(const fs::path& path) override;

		std::vector<fs::path> getKnownDataPathList() const override;

		void setMainWindowProperties(const MainWindowProperties& props) override;
		MainWindowProperties mainWindow() const override;

		bool dataPathHasStar(const fs::path& path) const override;
		void starDataPath(const fs::path& path, bool star = true) override;
		void unstarDataPath(const fs::path& path) override;

	private:
		fs::path configFilePath() const;
		void validate();

	private:
		std::variant<PortableMode, MainMode> _mode;

		nlohmann::json _data;
	};
}
