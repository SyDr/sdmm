// SD Mod Manager

// Copyright (c) 2020-2024 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "version.hpp"

namespace mm::SystemInfo
{
	inline constexpr const auto AppDataDirectory = "_MM_Data";
	inline constexpr const auto BaseDirFile      = "base_dir.txt";
	inline constexpr const auto SettingsFile     = "settings.json";

	inline constexpr const auto DataDir         = "data";
	inline constexpr const auto ModInfoFilename = "mod.json";
	inline constexpr const auto DefaultLanguage = "en";

	inline constexpr const auto ProgramVersion = PROGRAM_NAME " [" PROGRAM_VERSION " - " PROGRAM_ALIAS "]";
}
