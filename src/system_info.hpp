// SD Mod Manager

// Copyright (c) 2020-2025 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "version.hpp"

namespace mm::SystemInfo
{
	inline constexpr auto AppDataDirectory = "_MM_Data";
	inline constexpr auto BaseDirFile      = "base_dir.txt";
	inline constexpr auto SettingsFile     = "settings.json";

	inline constexpr auto DataDir         = "data";
	inline constexpr auto ModInfoFilename = "mod.json";
	inline constexpr auto DefaultLanguage = "en";

	inline constexpr auto DocDir     = "docs";
	inline constexpr auto ModInfoDoc = "mod.json.md";

	inline constexpr auto ProgramVersion = PROGRAM_NAME " [" PROGRAM_VERSION " - " PROGRAM_ALIAS "]";
}
