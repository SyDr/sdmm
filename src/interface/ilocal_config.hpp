// SD Mod Manager

// Copyright (c) 2020-2024 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "type/conflict_resolve_mode.hpp"
#include "type/filesystem.hpp"
#include "type/mod_list_model_structs.hpp"

namespace mm
{
	struct ILocalConfig
	{
		virtual ~ILocalConfig() = default;

		virtual void save() = 0;

		[[nodiscard]] virtual fs::path getDataPath() const = 0;
		[[nodiscard]] virtual fs::path getTempPath() const = 0;

		[[nodiscard]] virtual ConflictResolveMode conflictResolveMode() const                    = 0;
		virtual void                              conflictResolveMode(ConflictResolveMode value) = 0;

		[[nodiscard]] virtual std::string getAcitvePreset() const                    = 0;
		virtual void                      setActivePreset(const std::string& preset) = 0;

		[[nodiscard]] virtual std::vector<int> listColumns() const                        = 0;
		virtual void                           listColumns(const std::vector<int>& value) = 0;

		[[nodiscard]] virtual ModListModelManagedMode managedModsDisplay() const                        = 0;
		virtual void                                  managedModsDisplay(ModListModelManagedMode value) = 0;

		[[nodiscard]] virtual ModListModelArchivedMode archivedModsDisplay() const = 0;
		virtual void archivedModsDisplay(ModListModelArchivedMode value)           = 0;
	};
}
