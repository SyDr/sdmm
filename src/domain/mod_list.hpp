// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <wx/string.h>

#include <compare>
#include <optional>
#include <set>
#include <vector>

namespace mm
{
	struct ModList
	{
		std::set<std::string>    available;
		std::set<std::string>    hidden;
		std::vector<std::string> active;

		std::vector<std::string> invalid;

		ModList() = default;
		ModList(
			std::set<std::string> available, std::vector<std::string> active, std::set<std::string> hidden);

		bool isActive(const std::string& item) const;
		bool isHidden(const std::string& item) const;

		std::optional<size_t> activePosition(const std::string& item) const;
		void                  activate(const std::string& item);
		void                  deactivate(const std::string& item);
		void                  switchState(const std::string& item);

		bool canMove(const std::string& from, const std::string& to) const;
		void move(const std::string& from, const std::string& to);

		bool canMoveUp(const std::string& item) const;
		void moveUp(const std::string& item);

		bool canMoveDown(const std::string& item) const;
		void moveDown(const std::string& item);

		void hide(const std::string& item);
		void show(const std::string& item);
		void switchVisibility(const std::string& item);

		void remove(const std::string& item);

		std::weak_ordering operator<=>(const ModList& other) const = default;
	};
}
