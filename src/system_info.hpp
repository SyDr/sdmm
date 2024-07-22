// SD Mod Manager

// Copyright (c) 2020-2024 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "version.hpp"

namespace mm::SystemInfo
{
	constexpr const auto DataDir               = "data";
	constexpr const auto ModInfoFilename       = "mod.json";
	constexpr const auto DefaultLanguage       = "en";
	constexpr const auto DefaultLanguageLegacy = "en_US";

	constexpr const auto ManagedMod = "_MM_Managed_";

	constexpr const auto ProgramVersion = PROGRAM_NAME " [" PROGRAM_VERSION " - " PROGRAM_ALIAS "]";
}
