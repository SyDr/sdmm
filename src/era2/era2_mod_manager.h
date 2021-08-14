// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "domain/mod_list.hpp"
#include "interface/imod_manager.hpp"

#include <deque>
#include <optional>
#include <set>
#include <unordered_set>
#include <vector>

namespace mm
{
	struct Application;

	struct Era2ModManager : IModManager
	{
		explicit Era2ModManager(ModList& mods);

		ModList const& mods() const override;
		void           mods(ModList mods) override;

		std::optional<size_t> activePosition(wxString const& item) const override;
		void                  activate(wxString const& item) override;
		void                  deactivate(wxString const& item) override;
		void                  switchState(wxString const& item) override;

		bool canMove(wxString const& from, wxString const& to) const override;
		void move(wxString const& from, wxString const& to) override;

		bool canMoveUp(wxString const& item) const override;
		void moveUp(wxString const& item) override;

		bool canMoveDown(wxString const& item) const override;
		void moveDown(wxString const& item) override;

		void hide(wxString const& item) override;
		void show(wxString const& item) override;
		void switchVisibility(wxString const& item) override;

		void remove(wxString const& item) override;

		sigslot::signal<>& onListChanged() override;

	private:
		ModList& _list;

		sigslot::signal<> _listChanged;
	};
}
