// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "types/filesystem.hpp"

namespace mm
{
	struct PluginList;
	struct ModList;

	struct Era2PLuginListPhysicalStructure
	{
		using Era2PluginList = std::unordered_set<wxString>;
		using Map            = std::unordered_map<wxString, Era2PluginList>;

		Map data;  // modId -> plugin ids
	};

	namespace era2_plugin_helper
	{
		Era2PLuginListPhysicalStructure loadPhysicalStructure(const fspath& base);
		void load(PluginList& current, const Era2PLuginListPhysicalStructure& structure, const ModList& mods);
		PluginList load(const Era2PLuginListPhysicalStructure& structure, const ModList& mods);
	}
}
