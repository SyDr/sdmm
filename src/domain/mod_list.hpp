// SD Mod Manager

// Copyright (c) 2020-2024 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <compare>
#include <optional>
#include <set>
#include <vector>

namespace mm
{
	struct ModList
	{
		enum class ModState
		{
			active,
			inactive,
		};

		struct Mod
		{
			std::string id;
			ModState    state = ModState::active;

			Mod() = default;

			Mod(const std::string& id, ModState state)
				: id(id)
				, state(state) {};

			std::weak_ordering operator<=>(const Mod& other) const = default;

			std::string to_string() const
			{
				switch (state)
				{
				case ModState::inactive: return '*' + id;
				default: break;
				}
				return id;
			}
		};

		std::vector<Mod> data;

		std::set<std::string> rest;

		std::vector<std::string> invalid;

		ModList() = default;

		bool managed(const std::string& id) const;

		std::optional<size_t> position(const std::string& id) const;

		std::optional<ModState> state(const std::string& id) const;
		bool                    active(const std::string& id) const;
		bool                    inactive(const std::string& id) const;

		void activate(const std::string& id);
		void activate(const std::string& id, size_t at);
		void deactivate(const std::string& id);
		void reset(const std::string& id);

		void switchState(const std::string& id);

		bool canMove(const std::string& from, const std::string& to) const;
		bool canMoveUp(const std::string& id) const;
		bool canMoveDown(const std::string& id) const;

		void move(const std::string& from, const std::string& to);
		void move(size_t from, size_t to);
		void moveUp(const std::string& id);
		void moveDown(const std::string& id);

		void remove(const std::string& id);

		std::weak_ordering operator<=>(const ModList& other) const = default;
	};
}
