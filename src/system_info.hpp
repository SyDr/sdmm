// SD Mod Manager

// Copyright (c) 2020-2021 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <array>
#include <set>

#define PROGRAM_NAME    "SD Mod Manager"
#define PROGRAM_ALIAS   "The Sparrow"
#define PROGRAM_VERSION "0.98.3.alpha"

namespace mm::constant
{
	constexpr const auto data_dir          = "data";
	constexpr const auto mod_info_filename = "mod.json";
	constexpr const auto default_language  = "en_US";

	constexpr const auto mm_managed_mod = "_MM_Managed_";

	constexpr const auto        pluginSubdir       = "EraPlugins";
	constexpr const auto        pluginOffExtension = ".off";
	constexpr const std::array  pluginDirs         = { ".", "BeforeWog", "AfterWog" };
	const std::set<std::string> pluginExts         = { ".dll", ".bin", ".era" };

	const std::unordered_set<std::wstring> pacExtensions = {
		L".lod",
		L".snd",
		L".vid",
		L".pac",
	};

	constexpr const auto program_full_version = PROGRAM_NAME " [" PROGRAM_VERSION " - " PROGRAM_ALIAS "]";
}
