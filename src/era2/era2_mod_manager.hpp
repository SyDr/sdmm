// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
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

		void activate(const std::string& item) override;
		void deactivate(const std::string& item) override;
		void switchState(const std::string& item) override;
		void reset(const std::string& item) override;

		void move(const std::string& from, const std::string& to) override;
		void moveUp(const std::string& item) override;
		void moveDown(const std::string& item) override;

		void remove(const std::string& item) override;

		sigslot::signal<>& onListChanged() override;

	private:
		ModList& _list;

		sigslot::signal<> _listChanged;
	};
}
