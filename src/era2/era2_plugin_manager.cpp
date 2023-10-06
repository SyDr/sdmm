// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "application.h"
#include "domain/mod_data.hpp"
#include "era2_config.h"
#include "era2_mod_manager.h"
#include "era2_plugin_helper.hpp"
#include "era2_plugin_manager.hpp"
#include "utility/fs_util.h"
#include "utility/json_util.h"
#include "utility/sdlexcept.h"
#include "utility/shell_util.h"
#include "utility/string_util.h"

#include <wx/dir.h>

#include <unordered_set>

using namespace mm;

Era2PluginManager::Era2PluginManager(
	Era2ModManager& modManager, const fs::path& modsDir, const fs::path& listPath)
	: _modManager(modManager)
	, _modsDir(modsDir)
	, _listPath(listPath)
{
	_physicalStructure = era2_plugin_helper::loadPhysicalStructure(modsDir);
	_pluginList        = era2_plugin_helper::loadBaseState(_physicalStructure, _modManager.mods());
	era2_plugin_helper::loadManagedState(_pluginList, listPath);
	_initialPluginList = _pluginList;

	_modManager.onListChanged().connect([this] {
		era2_plugin_helper::updateBaseState(_pluginList, _physicalStructure, _modManager.mods());
		_listChanged();
	});
}

const PluginList& Era2PluginManager::plugins() const
{
	return _pluginList;
}

void Era2PluginManager::switchState(const wxString& plugin)
{
	auto defaultState = _pluginList.defaultState(plugin);
	auto currentState = _pluginList.overriddenState(plugin);

	if (currentState.has_value() || !defaultState.has_value())
	{
		_pluginList.reset(plugin);
	}
	else
	{
		_pluginList.overrideState(plugin, switchPluginState(defaultState->state));
	}

	_listChanged();
}

void Era2PluginManager::save()
{
	era2_plugin_helper::saveManagedState(_listPath, _modsDir, _pluginList);

	_initialPluginList = _pluginList;
}

bool Era2PluginManager::changed() const
{
	return _pluginList != _initialPluginList;
}

void Era2PluginManager::revert()
{
	plugins(_initialPluginList);
}

sigslot::signal<>& Era2PluginManager::onListChanged()
{
	return _listChanged;
}

void Era2PluginManager::plugins(PluginList items)
{
	_pluginList = std::move(items);

	_listChanged();
}

void Era2PluginManager::updateBaseState(PluginList& plugins, const ModList& mods) const
{
	era2_plugin_helper::updateBaseState(plugins, _physicalStructure, mods);
}
