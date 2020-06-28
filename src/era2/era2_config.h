// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "interface/domain/ilocal_config.h"

#include <nlohmann/json.hpp>
#include <wx/string.h>

namespace mm
{
	struct Era2ConfigData;

	class Era2Config : public ILocalConfig
	{
	  public:
		explicit Era2Config(const std::filesystem::path& path);

		void save() override;

		std::filesystem::path getDataPath() const override;
		std::filesystem::path getTempPath() const override;

		std::filesystem::path getProgramDataPath() const;
		std::filesystem::path getPresetsPath() const;

		wxString getExecutable() const;
		void     setExecutable(const wxString& executable);

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
