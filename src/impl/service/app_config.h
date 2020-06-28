// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "interface/service/iapp_config.h"

#include <nlohmann/json.hpp>

namespace mm
{
	class AppConfig : public IAppConfig
	{
	public:
		AppConfig();

		bool portableMode() const override;
		std::filesystem::path dataPath() const override;
		std::filesystem::path programPath() const override;

		void save() override;

		auto currentLanguageCode() const -> std::string override;
		void setCurrentLanguageCode(const wxString& lngCode) override;

		wxString selectedPlatform() const override;
		void setSelectedPlatformCode(const wxString& newPlatform) override;

		std::filesystem::path getDataPath() const override;
		void setDataPath(const std::filesystem::path& path) override;
		void forgetDataPath(const std::filesystem::path& path) override;

		std::vector<std::filesystem::path> getKnownDataPathList() const override;

		void setMainWindowProperties(const MainWindowProperties& props) override;
		MainWindowProperties mainWindow() const override;

		bool dataPathHasStar(const std::filesystem::path& path) const override;
		void starDataPath(const std::filesystem::path& path, bool star = true) override;
		void unstarDataPath(const std::filesystem::path& path) override;

	private:
		std::filesystem::path configFilePath() const;
		void validate();

	private:
		const std::filesystem::path _portableDataPath;
		const std::filesystem::path _userDataPath;

		const bool _portableMode = false;

		nlohmann::json _data;
	};
}
