// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <set>

#include <wx/string.h>

#include <sigslot/signal.hpp>

namespace mm
{
	struct ModList;
	struct PluginList;

	struct IPresetManager
	{
		virtual ~IPresetManager() = default;

		[[nodiscard]] virtual std::set<wxString> list() const = 0;

		[[nodiscard]] virtual std::pair<ModList, PluginList> loadPreset(const wxString& name)         = 0;
		virtual void savePreset(const wxString& name, const ModList& list, const PluginList& plugins) = 0;

		[[nodiscard]] virtual bool exists(const wxString& name) const               = 0;
		virtual void               copy(const wxString& from, const wxString& to)   = 0;
		virtual void               rename(const wxString& from, const wxString& to) = 0;
		virtual void               remove(const wxString& name)                     = 0;

		[[nodiscard]] virtual sigslot::signal<>& onListChanged() = 0;
	};
}
