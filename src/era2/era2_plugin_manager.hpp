// SD Mod Manager

// Copyright (c) 2026 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "domain/plugin_list.hpp"
#include "interface/iplugin_manager.hpp"
#include "type/filesystem.hpp"

#include <deque>
#include <optional>
#include <set>
#include <unordered_set>
#include <vector>

namespace mm
{
	struct Application;
	struct Era2ModManager;

	struct Era2PluginManager : IPluginManager
	{
		explicit Era2PluginManager(
			Era2ModManager& modManager, const fs::path& modsDir);

		void plugins(PluginList items);

		PluginList const& plugins() const override;

		void enable(const PluginId& id) override;
		void disable(const PluginId& id) override;
		void reset(const PluginId& id) override;

		void save() override;

		sigslot::signal<>& onListChanged() override;

		static AvailablePlugins loadAvailablePlugins(const fs::path& basePath);

	private:
		Era2ModManager& _modManager;
		fs::path        _modsDir;

		PluginList _pluginList;

		sigslot::signal<> _listChanged;
	};
}
