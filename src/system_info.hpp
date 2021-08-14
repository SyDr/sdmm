// SD Mod Manager

// Copyright (c) 2020-2021 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#define PROGRAM_NAME    "SD Mod Manager"
#define PROGRAM_ALIAS   "The Sparrow"
#define PROGRAM_VERSION "0.98.1.alpha"

namespace mm::constant
{
	constexpr auto data_dir          = "data";
	constexpr auto mod_info_filename = "mod.json";
	constexpr auto default_language  = "en_US";

	constexpr auto mm_managed_mod = "_MM_Managed_";

	constexpr auto program_full_version = PROGRAM_NAME " [" PROGRAM_VERSION " - " PROGRAM_ALIAS "]";
}
