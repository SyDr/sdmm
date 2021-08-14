// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "domain/iplugin_manager.hpp"
#include "domain/plugin_list.hpp"

#include <deque>
#include <optional>
#include <set>
#include <unordered_set>
#include <vector>

namespace mm
{
	struct Application;

	struct Era2PluginManager : IPluginManager
	{
		Era2PluginManager() = default;
		explicit Era2PluginManager(PluginList& items);

		PluginList const& plugins() const override;
		void              plugins(PluginList items) override;

		void switchState(const wxString& plugin) override;

		sigslot::signal<>& onListChanged() override;

	private:
		PluginList& _items;

		sigslot::signal<> _listChanged;
	};
}
