// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "plugin_list.hpp"

using namespace mm;

std::strong_ordering mm::PluginSource::operator<=>(const PluginSource& other) const
{
	return std::tie(location, name, modId) <=> std::tie(other.location, other.name, other.modId);
}

std::string mm::to_string(PluginLocation location)
{
	switch (location)
	{
	case PluginLocation::root: return ".";
	case PluginLocation::before_wog: return "BeforeWoG";
	case PluginLocation::after_wog: return "AfterWoG";
	}

	return "";
}

std::string mm::PluginSource::toString() const
{
	std::string result =
		active() ? name : name.substr(0, name.size() - std::char_traits<const char>::length(".off"));

	if (location == PluginLocation::root)
		return result;

	return result + " (" + mm::to_string(location) + ")";
}

std::string mm::PluginSource::toFileIdenity() const
{
	std::string result =
		active() ? name : name.substr(0, name.size() - std::char_traits<const char>::length(".off"));

	if (location == PluginLocation::root)
		return result;

	return mm::to_string(location) + "/" + result;
}

bool mm::PluginSource::active() const
{
	return !boost::ends_with(name, ".off");
}

bool mm::PluginList::active(const PluginSource& item) const
{
	return item.active() xor managed.contains(item);
}

void mm::PluginList::switchState(const PluginSource& item)
{
	if (managed.contains(item))
		managed.erase(item);
	else
		managed.emplace(item);
}
