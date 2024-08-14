// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <sigslot/signal.hpp>

#include <optional>

namespace mm
{
	struct ModData;
	struct ModList;

	struct IModManager
	{
		virtual ~IModManager() = default;

		[[nodiscard]] virtual ModList const& mods() const = 0;

		virtual void activate(const std::string& item)    = 0;
		virtual void deactivate(const std::string& item)  = 0;
		virtual void switchState(const std::string& item) = 0;
		virtual void reset(const std::string& item)       = 0;

		virtual void move(const std::string& from, const std::string& to) = 0;
		virtual void moveUp(const std::string& item)                      = 0;
		virtual void moveDown(const std::string& item)                    = 0;

		virtual void remove(const std::string& item) = 0;

		[[nodiscard]] virtual sigslot::signal<>& onListChanged() = 0;
	};
}
