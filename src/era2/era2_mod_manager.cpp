// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "era2_mod_manager.hpp"

#include "application.h"
#include "domain/mod_data.hpp"
#include "era2_config.hpp"
#include "utility/fs_util.h"
#include "utility/json_util.h"
#include "utility/sdlexcept.h"

#include <unordered_set>

using namespace mm;

Era2ModManager::Era2ModManager(ModList& mods)
	: _list(mods)
{}

sigslot::signal<>& Era2ModManager::onListChanged()
{
	return _listChanged;
}

ModList const& Era2ModManager::mods() const
{
	return _list;
}

void Era2ModManager::mods(ModList mods)
{
	_list = std::move(mods);
}

void Era2ModManager::enable(const std::string& item)
{
	_list.enable(item);

	_listChanged();
}

void Era2ModManager::disable(const std::string& item)
{
	_list.disable(item);

	_listChanged();
}

void Era2ModManager::switchState(const std::string& item)
{
	if (!_list.enabled(item))
		enable(item);
	else
		disable(item);
}

void Era2ModManager::apply(const std::vector<std::string>& items)
{
	_list.apply(items);

	_listChanged();
}

void Era2ModManager::archive(const std::string& item)
{
	_list.archive(item);

	_listChanged();
}

void Era2ModManager::move(const std::string& from, const std::string& to)
{
	if (from == to)
		return;

	_list.move(from, to);

	_listChanged();
}

void Era2ModManager::moveUp(const std::string& item)
{
	_list.moveUp(item);

	_listChanged();
}

void Era2ModManager::moveDown(const std::string& item)
{
	_list.moveDown(item);

	_listChanged();
}

void Era2ModManager::remove(const std::string& item)
{
	_list.remove(item);

	_listChanged();
}
