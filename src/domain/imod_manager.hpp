// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <wigwag/signal.hpp>

#include <wx/string.h>

#include <optional>
#include <vector>

namespace mm
{
	struct ModData;
	struct ModList;

	struct IModManager
	{
		virtual ~IModManager() = default;

		virtual ModList const& mods() const          = 0;
		virtual void           setMods(ModList mods) = 0;

		virtual std::optional<size_t> activePosition(wxString const& item) const = 0;
		virtual void                  activate(wxString const& item)             = 0;
		virtual void                  deactivate(wxString const& item)           = 0;
		virtual void                  switchState(wxString const& item)          = 0;

		virtual bool canMove(wxString const& from, wxString const& to) const = 0;
		virtual void move(wxString const& from, wxString const& to)          = 0;

		virtual bool canMoveUp(wxString const& item) const = 0;
		virtual void moveUp(wxString const& item)          = 0;

		virtual bool canMoveDown(wxString const& item) const = 0;
		virtual void moveDown(wxString const& item)          = 0;

		virtual void hide(wxString const& item)             = 0;
		virtual void show(wxString const& item)             = 0;
		virtual void switchVisibility(wxString const& item) = 0;

		virtual void remove(wxString const& item) = 0;

		virtual wigwag::signal_connector<void()> onListChanged() const = 0;
	};
}
