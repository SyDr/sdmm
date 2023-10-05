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

	struct IPluginManager
	{
		virtual ~IPluginManager() = default;

		virtual PluginList const& plugins() const           = 0;
		virtual void              plugins(PluginList items) = 0;

		virtual void updateBaseState(PluginList& plugins, const ModList& mods) const = 0;

		virtual void switchState(const wxString& plugin) = 0;

		virtual void save()          = 0;
		virtual bool changed() const = 0;
		virtual void revert()        = 0;

		virtual sigslot::signal<>& onListChanged() = 0;
	};
}
