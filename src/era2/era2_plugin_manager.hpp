// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "interface/iplugin_manager.hpp"
#include "domain/plugin_list.hpp"
#include "era2_plugin_helper.hpp"

#include <deque>
#include <optional>
#include <set>
#include <unordered_set>
#include <vector>

namespace mm
{
	struct Application;
	struct Era2ModManager;
	struct Era2PLuginListPhysicalStructure;

	struct Era2PluginManager : IPluginManager
	{
		explicit Era2PluginManager(Era2ModManager& modManager, const fs::path& modsDir,
								   const fs::path& listPath);

		PluginList const& plugins() const override;
		void              plugins(PluginList items) override;

		void updateBaseState(PluginList& plugins, const ModList& mods) const override;

		void switchState(const wxString& plugin) override;

		void save() override;
		bool changed() const override;
		void revert() override;

		sigslot::signal<>& onListChanged() override;

	private:
		Era2ModManager& _modManager;
		fs::path        _modsDir;
		fs::path        _listPath;

		Era2PLuginListPhysicalStructure _physicalStructure;

		PluginList _initialPluginList;
		PluginList _pluginList;

		sigslot::signal<> _listChanged;
	};
}
