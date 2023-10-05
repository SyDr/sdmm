// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "type/filesystem.hpp"

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
		Era2PLuginListPhysicalStructure loadPhysicalStructure(const fs::path& base);

		PluginList loadBaseState(const Era2PLuginListPhysicalStructure& structure, const ModList& mods);
		void       updateBaseState(PluginList& current, const Era2PLuginListPhysicalStructure& structure,
								   const ModList& mods);

		void loadManagedState(PluginList& target, const fs::path& pluginPath);
		void saveManagedState(const fs::path& pluginPath, const fs::path& modsPath, const PluginList& list);
	}
}
