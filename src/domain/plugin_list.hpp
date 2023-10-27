// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <wx/string.h>

#include <compare>
#include <set>

#include "type/filesystem.hpp"

namespace mm
{
	enum class PluginLocation
	{
		root       = 0,
		before_wog = 1,
		after_wog  = 2,
	};

	std::string to_string(PluginLocation location);

	struct PluginSource
	{
		std::string    modId;
		PluginLocation location = PluginLocation::root;
		std::string    name;

		PluginSource() = default;
		PluginSource(const std::string& modId, PluginLocation location, const std::string& name)
			: modId(modId)
			, location(location)
			, name(name)
		{}

		std::string toString() const;
		std::string toFileIdenity() const;

		bool active() const;

		std::strong_ordering operator<=>(const PluginSource& other) const;
		bool                 operator==(const PluginSource& other) const = default;
	};

	struct PluginList
	{
		std::set<PluginSource> available;
		std::set<PluginSource> managed;

		bool active(const PluginSource& item) const;

		void switchState(const PluginSource& item);

		std::weak_ordering operator<=>(const PluginList& other) const = default;
	};
}
