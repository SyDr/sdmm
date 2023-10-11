// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "interface/ilocal_config.h"

#include <nlohmann/json.hpp>
#include <wx/string.h>

namespace mm
{
	struct Era2ConfigData;

	struct Era2Config : public ILocalConfig
	{
		explicit Era2Config(const fs::path& path);

		void save() override;

		fs::path getDataPath() const override;
		fs::path getTempPath() const override;

		fs::path getProgramDataPath() const;
		fs::path getPresetsPath() const;

		std::string getExecutable() const;
		void        setExecutable(const std::string& executable);

		wxString getLaunchString() const;

		wxString getAcitvePreset() const override;
		void     setActivePreset(const wxString& preset) override;

		bool showHiddenMods() const override;
		void showHiddenMods(bool value) override;

		ConflictResolveMode conflictResolveMode() const override;
		void                conflictResolveMode(ConflictResolveMode value) override;

	private:
		std::filesystem::path getConfigFilePath() const;
		void                  createDirectories() const;

		void validate();

	private:
		const std::filesystem::path _path;
		nlohmann::json              _data;
	};
}
