// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <sigslot/signal.hpp>
#include <wx/string.h>

#include <optional>

namespace mm
{
	struct ModData;
	struct ModList;

	struct IModManager
	{
		virtual ~IModManager() = default;

		virtual ModList const& mods() const       = 0;

		virtual std::optional<size_t> activePosition(const wxString& item) const = 0;
		virtual void                  activate(const wxString& item)             = 0;
		virtual void                  deactivate(const wxString& item)           = 0;
		virtual void                  switchState(const wxString& item)          = 0;

		virtual bool canMove(const wxString& from, const wxString& to) const = 0;
		virtual void move(const wxString& from, const wxString& to)          = 0;

		virtual bool canMoveUp(const wxString& item) const = 0;
		virtual void moveUp(const wxString& item)          = 0;

		virtual bool canMoveDown(const wxString& item) const = 0;
		virtual void moveDown(const wxString& item)          = 0;

		virtual void hide(const wxString& item)             = 0;
		virtual void show(const wxString& item)             = 0;
		virtual void switchVisibility(const wxString& item) = 0;

		virtual void remove(const wxString& item) = 0;

		virtual sigslot::signal<>& onListChanged() = 0;
	};
}
