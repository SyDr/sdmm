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
		ModList     mods;

		std::string executable;

		std::weak_ordering operator<=>(const PresetData& other) const = default;

		PresetData(const ModList& mods, const std::string& executable)
			: mods(mods)
			, executable(executable)
		{}

		PresetData() = default;
	};
}
