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

	inline std::string to_string(PluginLocation location)
	{
		switch (location)
		{
		case PluginLocation::root: return ".";
		case PluginLocation::before_wog: return "BeforeWoG";
		case PluginLocation::after_wog: return "AfterWoG";
		}

		return "";
	}

	struct PluginSource
	{
		std::string    modId;
		PluginLocation location = PluginLocation::root;
		std::string    name;

		PluginSource() = default;
		PluginSource(const std::string& modId, PluginLocation location, const std::string& name);

		std::string toString() const
		{
			std::string result =
				active() ? name : name.substr(0, name.size() - std::char_traits<const char>::length(".off"));

			if (location == PluginLocation::root)
				return result;

			return result + " (" + mm::to_string(location) + ")";
		}

		std::string toFileIdenity() const
		{
			std::string result =
				active() ? name : name.substr(0, name.size() - std::char_traits<const char>::length(".off"));

			if (location == PluginLocation::root)
				return result;

			return mm::to_string(location) + "/" + result;
		}

		bool active() const
		{
			return !boost::ends_with(name, ".off");
		}

		auto operator<=>(const PluginSource& other) const
		{
			return std::tie(location, name, modId) <=> std::tie(other.location, other.name, other.modId);
		}

		bool operator==(const PluginSource& other) const = default;
	};

	struct PluginList
	{
		std::set<PluginSource> available;
		std::set<PluginSource> managed;

		bool active(const PluginSource& item) const
		{
			return item.active() xor managed.contains(item);
		}

		void switchState(const PluginSource& item)
		{
			if (managed.contains(item))
				managed.erase(item);
			else
				managed.emplace(item);
		}

		std::weak_ordering operator<=>(const PluginList& other) const = default;
	};
}
