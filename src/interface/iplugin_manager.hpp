// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <sigslot/signal.hpp>

#include <wx/string.h>

namespace mm
{
	struct PluginList;
	struct ModList;
	struct PluginSource;

	struct IPluginManager
	{
		virtual ~IPluginManager() = default;

		virtual PluginList const& plugins() const           = 0;
		virtual void              plugins(PluginList items) = 0;

		virtual void switchState(const PluginSource& plugin) = 0;

		virtual void save() = 0;

		virtual sigslot::signal<>& onListChanged() = 0;
	};
}
