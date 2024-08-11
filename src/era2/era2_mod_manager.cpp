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

void Era2ModManager::activate(const std::string& item)
{
	_list.activate(item);

	_listChanged();
}

void Era2ModManager::deactivate(const std::string& item)
{
	_list.deactivate(item);

	_listChanged();
}

void Era2ModManager::switchState(const std::string& item)
{
	if (!_list.active(item))
		activate(item);
	else
		deactivate(item);
}

void Era2ModManager::reset(const std::string& item)
{
	_list.reset(item);

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

void Era2ModManager::hide(const std::string& item)
{
	_list.hide(item);

	_listChanged();
}

void Era2ModManager::show(const std::string& item)
{
	_list.show(item);

	_listChanged();
}

void Era2ModManager::switchVisibility(const std::string& item)
{
	if (_list.hidden(item))
		show(item);
	else
		hide(item);
}

void Era2ModManager::remove(const std::string& item)
{
	_list.remove(item);

	_listChanged();
}
