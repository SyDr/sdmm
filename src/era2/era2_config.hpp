// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "interface/ilocal_config.hpp"

#include <nlohmann/json.hpp>

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

		std::string getLaunchString() const;

		std::string getAcitvePreset() const override;
		void        setActivePreset(const std::string& preset) override;

		ConflictResolveMode conflictResolveMode() const override;
		void                conflictResolveMode(ConflictResolveMode value) override;

		std::vector<int> listColumns() const override;
		void             listColumns(const std::vector<int>& value) override;

		ModListModelManagedMode managedModsDisplay() const override;
		void                    managedModsDisplay(ModListModelManagedMode value) override;

		ModListModelArchivedMode archivedModsDisplay() const override;
		void                     archivedModsDisplay(ModListModelArchivedMode value) override;

		std::set<ModListDsplayedData::GroupItemsBy> collapsedCategories() const override;
		void collapsedCategories(const std::set<ModListDsplayedData::GroupItemsBy>& value) override;

		std::set<std::string> hiddenCategories() const override;
		void                  hiddenCategories(const std::set<std::string>& value) override;

		bool screenshotsExpanded() const override;
		void screenshotsExpanded(bool value) override;

	private:
		fs::path getConfigFilePath() const;
		void     createDirectories() const;

		void validate();

	private:
		const fs::path _path;
		nlohmann::json _data;
	};
}
