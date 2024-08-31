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
			enabled,
			disabled,
		};

		struct Mod
		{
			std::string id;
			ModState    state = ModState::enabled;

			Mod() = default;

			Mod(const std::string& id, ModState state)
				: id(id)
				, state(state) {};

			std::weak_ordering operator<=>(const Mod& other) const = default;

			std::string to_string() const
			{
				switch (state)
				{
				case ModState::disabled: return '*' + id;
				default: break;
				}
				return id;
			}
		};

		std::vector<Mod>      data;
		std::set<std::string> rest;

		ModList() = default;
		explicit ModList(const std::vector<std::string>& active);

		bool managed(const std::string& id) const;

		std::optional<size_t> position(const std::string& id) const;
		std::string           next(const std::string& id) const;

		std::optional<ModState>  state(const std::string& id) const;
		bool                     enabled(const std::string& id) const;
		bool                     disabled(const std::string& id) const;
		std::vector<std::string> enabled() const;

		void enable(const std::string& id);
		void enable(const std::string& id, size_t at);
		void disable(const std::string& id);
		void archive(const std::string& id);

		void apply(const std::vector<std::string>& ids);

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
