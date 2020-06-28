// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "domain/imod_manager.hpp"
#include "domain/mod_list.hpp"

#include <deque>
#include <vector>
#include <set>
#include <unordered_set>
#include <optional>

#include <wigwag/token_pool.hpp>

namespace mm
{
	class Application;

	struct Era2ModManager : public IModManager
	{
		Era2ModManager() = default;
		explicit Era2ModManager(ModList mods);

		ModList const& mods() const override;
		void setMods(ModList mods) override;

		std::optional<size_t> activePosition(wxString const& item) const override;
		void activate(wxString const& item) override;
		void deactivate(wxString const& item) override;
		void switchState(wxString const& item) override;

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

		wigwag::signal_connector<void()> onListChanged() const override;

	private:
		ModList _mods;

		wigwag::signal<void()> _listChanged;
	};
}
