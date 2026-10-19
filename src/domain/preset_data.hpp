// SD Mod Manager

// Copyright (c) 2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "mod_list.hpp"
#include "plugin_list.hpp"

#include <string>

namespace mm
{
	struct PresetData
	{
		ModList mods;
		PluginList               plugins;
		std::string              executable;

		std::weak_ordering operator<=>(const PresetData& other) const = default;

		PresetData(ModList mods, PluginList plugins, std::string executable)
			: mods(std::move(mods))
			, plugins(std::move(plugins))
			, executable(std::move(executable))
		{}

		PresetData() = default;
	};
}
