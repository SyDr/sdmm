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

		void mods(ModList mods);

		ModList const& mods() const override;

		std::optional<size_t> activePosition(const wxString& item) const override;
		void                  activate(const wxString& item) override;
		void                  deactivate(const wxString& item) override;
		void                  switchState(const wxString& item) override;

		bool canMove(const wxString& from, const wxString& to) const override;
		void move(const wxString& from, const wxString& to) override;

		bool canMoveUp(const wxString& item) const override;
		void moveUp(const wxString& item) override;

		bool canMoveDown(const wxString& item) const override;
		void moveDown(const wxString& item) override;

		void hide(const wxString& item) override;
		void show(const wxString& item) override;
		void switchVisibility(const wxString& item) override;

		void remove(const wxString& item) override;

		sigslot::signal<>& onListChanged() override;

	private:
		ModList& _list;

		sigslot::signal<> _listChanged;
	};
}
