// SD Mod Manager

// Copyright (c) 2020-2025 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "interface/iapp_config.hpp"

#include <nlohmann/json.hpp>

#include <variant>

namespace mm
{
	struct PortableMode
	{
		fs::path managedPath;
		fs::path configLocation;
	};

	struct MainMode
	{
		fs::path programDataPath;
	};

	struct AppConfig : IAppConfig
	{
		AppConfig();

		bool     portableMode() const override;
		fs::path dataPath() const override;
		fs::path programPath() const override;

		void save() override;

		std::string currentLanguageCode() const override;
		bool        setCurrentLanguageCode(const std::string& value) override;

		std::string selectedPlatform() const override;
		void        setSelectedPlatformCode(const std::string& newPlatform) override;

		fs::path getDataPath() const override;
		void     setDataPath(const fs::path& path) override;
		void     forgetDataPath(const fs::path& path) override;

		std::vector<fs::path> getKnownDataPathList() const override;

		void                 setMainWindowProperties(const MainWindowProperties& props) override;
		MainWindowProperties mainWindow() const override;

		bool dataPathHasStar(const fs::path& path) const override;
		void starDataPath(const fs::path& path, bool star = true) override;
		void unstarDataPath(const fs::path& path) override;

		UpdateCheckMode updateCheckMode() const override;
		void            updateCheckMode(UpdateCheckMode value) override;

		time_point lastUpdateCheck() const override;
		void       lastUpdateCheck(time_point value) override;

		ModDescriptionUsedControl modDescriptionUsedControl() const override;
		bool                      modDescriptionUsedControl(ModDescriptionUsedControl value) override;

		InterfaceSize interfaceSize() const override;
		bool          interfaceSize(InterfaceSize value) override;

		InterfaceLabel interfaceLabel() const override;
		bool           interfaceLabel(InterfaceLabel value) override;

	private:
		fs::path configFilePath() const;
		void     validate();

	private:
		std::variant<PortableMode, MainMode> _mode;

		nlohmann::json _data;
	};
}
