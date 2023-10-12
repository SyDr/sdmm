// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "type/conflict_resolve_mode.hpp"
#include "type/filesystem.hpp"

namespace mm
{
	struct ILocalConfig
	{
		virtual ~ILocalConfig() = default;

		virtual void save() = 0;

		[[nodiscard]] virtual fs::path getDataPath() const = 0;
		[[nodiscard]] virtual fs::path getTempPath() const = 0;

		[[nodiscard]] virtual bool showHiddenMods() const     = 0;
		virtual void               showHiddenMods(bool value) = 0;

		[[nodiscard]] virtual ConflictResolveMode conflictResolveMode() const                    = 0;
		virtual void                              conflictResolveMode(ConflictResolveMode value) = 0;

		[[nodiscard]] virtual std::string getAcitvePreset() const                    = 0;
		virtual void                      setActivePreset(const std::string& preset) = 0;
	};
}
