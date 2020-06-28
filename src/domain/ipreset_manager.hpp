// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <set>

#include <wx/string.h>

#include <wigwag/signal.hpp>

namespace mm
{
	struct ModList;

	struct IPresetManager
	{
		virtual ~IPresetManager() = default;

		virtual std::set<wxString> list() const = 0;

		virtual ModList loadPreset(wxString const& name)                      = 0;
		virtual void    savePreset(wxString const& name, ModList const& list) = 0;

		virtual bool exists(wxString const& name) const               = 0;
		virtual void copy(wxString const& from, wxString const& to)   = 0;
		virtual void rename(wxString const& from, wxString const& to) = 0;
		virtual void remove(wxString const& name)                     = 0;

		virtual wigwag::signal_connector<void()> onListChanged() const = 0;
	};
}
