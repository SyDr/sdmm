// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "plugin_list.hpp"
#include "utility/sdlexcept.h"

#include <fmt/format.h>

using namespace mm;

void PluginList::overrideState(const wxString& item, PluginState newState)
{
	overridden[item] = newState;
}

std::optional<ModPluginState> PluginList::defaultState(const wxString& item) const
{
	auto it = state.find(item);
	if (it != state.end())
		return it->second;

	return {};
}

std::optional<PluginState> PluginList::overriddenState(const wxString& item) const
{
	auto it = overridden.find(item);
	if (it != overridden.end())
		return it->second;

	return {};
}

void PluginList::reset(const wxString& item)
{
	overridden.erase(item);
	if (!defaultState(item))
		available.erase(item);
}
