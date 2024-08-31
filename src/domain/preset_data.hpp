// SD Mod Manager

// Copyright (c) 2023-2024 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "mod_list.hpp"

#include <string>

namespace mm
{
	struct PresetData
	{
		std::vector<std::string> mods;
		std::string              executable;

		std::weak_ordering operator<=>(const PresetData& other) const = default;

		PresetData(std::vector<std::string> mods, std::string executable)
			: mods(std::move(mods))
			, executable(std::move(executable))
		{}

		PresetData() = default;
	};
}
