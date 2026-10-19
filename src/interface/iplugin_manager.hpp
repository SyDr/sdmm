// SD Mod Manager

// Copyright (c) 2026 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <sigslot/signal.hpp>

#include <wx/string.h>

namespace mm
{
	struct PluginList;
	struct PluginId;

	struct IPluginManager
	{
		virtual ~IPluginManager() = default;

		[[nodiscard]] virtual PluginList const& plugins() const = 0;

		virtual void enable(const PluginId& id)  = 0;
		virtual void disable(const PluginId& id) = 0;
		virtual void reset(const PluginId& id)   = 0;

		virtual void save() = 0;

		[[nodiscard]] virtual sigslot::signal<>& onListChanged() = 0;
	};
}
