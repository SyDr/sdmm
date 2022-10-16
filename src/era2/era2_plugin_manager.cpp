// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "application.h"
#include "domain/mod_data.hpp"
#include "era2_config.h"
#include "era2_plugin_manager.hpp"
#include "utility/fs_util.h"
#include "utility/json_util.h"
#include "utility/sdlexcept.h"
#include "utility/shell_util.h"
#include "utility/string_util.h"

#include <wx/dir.h>

#include <unordered_set>

using namespace mm;

Era2PluginManager::Era2PluginManager(PluginList& items)
	: _items(items)
{
}

const PluginList& Era2PluginManager::plugins() const
{
	return _items;
}

void Era2PluginManager::switchState(const wxString& plugin)
{
	auto defaultState = _items.defaultState(plugin);
	auto currentState = _items.overriddenState(plugin);

	if (currentState.has_value() || !defaultState.has_value())
	{
		_items.reset(plugin);
	}
	else
	{
		_items.overrideState(plugin, switchPluginState(defaultState->state));
	}

	_listChanged();
}

sigslot::signal<>& Era2PluginManager::onListChanged()
{
	return _listChanged;
}

void Era2PluginManager::plugins(PluginList items)
{
	_items = std::move(items);

	_listChanged();
}
