// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "application.h"
#include "domain/mod_data.hpp"
#include "era2_config.h"
#include "era2_mod_manager.h"
#include "utility/fs_util.h"
#include "utility/json_util.h"
#include "utility/sdlexcept.h"
#include "utility/shell_util.h"
#include "utility/string_util.h"

#include <wx/dir.h>

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
	_listChanged();
}

std::optional<size_t> Era2ModManager::activePosition(const wxString& item) const
{
	return _list.activePosition(item);
}

void Era2ModManager::activate(const wxString& item)
{
	const auto size = _list.active.size();

	_list.activate(item);

	if (size != _list.active.size())
		_listChanged();
}

void Era2ModManager::deactivate(const wxString& item)
{
	const auto size = _list.active.size();

	_list.deactivate(item);

	if (size != _list.active.size())
		_listChanged();
}

void Era2ModManager::switchState(const wxString& item)
{
	if (!_list.isActive(item))
		activate(item);
	else
		deactivate(item);
}

bool Era2ModManager::canMove(const wxString& from, const wxString& to) const
{
	return _list.canMove(from, to);
}

void Era2ModManager::move(const wxString& from, const wxString& to)
{
	if (from == to)
		return;

	_list.move(from, to);

	_listChanged();
}

bool Era2ModManager::canMoveUp(const wxString& item) const
{
	return _list.canMoveUp(item);
}

void Era2ModManager::moveUp(const wxString& item)
{
	_list.moveUp(item);

	_listChanged();
}

bool Era2ModManager::canMoveDown(const wxString& item) const
{
	return _list.canMoveDown(item);
}

void Era2ModManager::moveDown(const wxString& item)
{
	_list.moveDown(item);

	_listChanged();
}

void Era2ModManager::hide(const wxString& item)
{
	const auto size = _list.hidden.size();

	_list.hide(item);

	if (size != _list.hidden.size())
		_listChanged();
}

void Era2ModManager::show(const wxString& item)
{
	const auto size = _list.hidden.size();

	_list.show(item);

	if (size != _list.hidden.size())
		_listChanged();
}

void Era2ModManager::switchVisibility(const wxString& item)
{
	if (_list.isHidden(item))
		show(item);
	else
		hide(item);
}

void Era2ModManager::remove(const wxString& item)
{
	_list.remove(item);

	_listChanged();
}
