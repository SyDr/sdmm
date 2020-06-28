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

Era2ModManager::Era2ModManager(ModList mods)
	: _mods(std::move(mods))
{
}

wigwag::signal_connector<void()> Era2ModManager::onListChanged() const
{
	return _listChanged.connector();
}

ModList const& Era2ModManager::mods() const
{
	return _mods;
}

void Era2ModManager::setMods(ModList mods)
{
	_mods = std::move(mods);
	_listChanged();
}

std::optional<size_t> Era2ModManager::activePosition(wxString const& item) const
{
	return _mods.activePosition(item);
}

void Era2ModManager::activate(wxString const& item)
{
	const auto size = _mods.active.size();

	_mods.activate(item);

	if (size != _mods.active.size())
		_listChanged();
}

void Era2ModManager::deactivate(wxString const& item)
{
	const auto size = _mods.active.size();

	_mods.deactivate(item);

	if (size != _mods.active.size())
		_listChanged();
}

void Era2ModManager::switchState(wxString const& item)
{
	if (!_mods.isActive(item))
		activate(item);
	else
		deactivate(item);
}

bool Era2ModManager::canMove(wxString const& from, wxString const& to) const
{
	return _mods.canMove(from, to);
}

void Era2ModManager::move(wxString const& from, wxString const& to)
{
	if (from == to)
		return;

	_mods.move(from, to);

	_listChanged();
}

bool Era2ModManager::canMoveUp(wxString const& item) const
{
	return _mods.canMoveUp(item);
}

void Era2ModManager::moveUp(wxString const& item)
{
	_mods.moveUp(item);

	_listChanged();
}

bool Era2ModManager::canMoveDown(wxString const& item) const
{
	return _mods.canMoveDown(item);
}

void Era2ModManager::moveDown(wxString const& item)
{
	_mods.moveDown(item);

	_listChanged();
}

void Era2ModManager::hide(wxString const& item)
{
	const auto size = _mods.hidden.size();

	_mods.hide(item);

	if (size != _mods.hidden.size())
		_listChanged();
}

void Era2ModManager::show(wxString const& item)
{
	const auto size = _mods.hidden.size();

	_mods.show(item);

	if (size != _mods.hidden.size())
		_listChanged();
}

void Era2ModManager::switchVisibility(wxString const& item)
{
	if (_mods.isHidden(item))
		show(item);
	else
		hide(item);
}

void Era2ModManager::remove(wxString const& id)
{
	_mods.remove(id);

	_listChanged();
}
