// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "era2_config.hpp"

#include <fstream>
#include <sstream>
#include <unordered_set>

#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/stdpaths.h>

#include "utility/fs_util.h"
#include "utility/sdlexcept.h"
#include "utility/json_util.h"
#include "system_info.hpp"

using namespace mm;

namespace
{
	constexpr const auto st_active_preset         = "active_preset";
	constexpr const auto st_executable            = "executable";
	constexpr const auto st_show_hidden           = "show_hidden";
	constexpr const auto st_conflict_resolve_mode = "conflict_resolve_mode";
}

Era2Config::Era2Config(const fs::path& path)
	: _path(path)
{
	createDirectories();

	_data = loadJsonFromFile(getConfigFilePath(), true);
	validate();
}

void Era2Config::createDirectories() const
{
	create_directories(getProgramDataPath());
	create_directories(getPresetsPath());
	create_directories(getTempPath());
}

void Era2Config::save()
{
	overwriteFile(getConfigFilePath(), _data.dump(2));
}

fs::path Era2Config::getDataPath() const
{
	return _path;
}

fs::path Era2Config::getProgramDataPath() const
{
	return getDataPath() / SystemInfo::AppDataDirectory;
}

fs::path Era2Config::getPresetsPath() const
{
	return getProgramDataPath() / "Profiles";
}

fs::path Era2Config::getTempPath() const
{
	return getProgramDataPath() / "Temp";
}

fs::path Era2Config::getConfigFilePath() const
{
	return getProgramDataPath() / "config.json";
}

std::string Era2Config::getLaunchString() const
{
	return (getDataPath() / getExecutable()).string();
}

std::string Era2Config::getExecutable() const
{
	return _data[st_executable].get<std::string>();
}

void Era2Config::setExecutable(const std::string& executable)
{
	_data[st_executable] = executable;
	save();
}

std::string Era2Config::getAcitvePreset() const
{
	return _data[st_active_preset].get<std::string>();
}

void Era2Config::setActivePreset(const std::string& preset)
{
	_data[st_active_preset] = preset;
	save();
}

bool Era2Config::showHiddenMods() const
{
	return _data[st_show_hidden].get<bool>();
}

void Era2Config::showHiddenMods(bool value)
{
	_data[st_show_hidden] = value;
	save();
}

ConflictResolveMode Era2Config::conflictResolveMode() const
{
	switch (auto mode = static_cast<ConflictResolveMode>(_data[st_conflict_resolve_mode].get<int>()))
	{
	case ConflictResolveMode::automatic:
	case ConflictResolveMode::manual: return mode;
	}

	return ConflictResolveMode::automatic;
}

void Era2Config::conflictResolveMode(ConflictResolveMode value)
{
	_data[st_conflict_resolve_mode] = static_cast<int>(value);
	save();
}

void Era2Config::validate()
{
	if (!_data.count(st_active_preset) || !_data[st_active_preset].is_string())
		_data[st_active_preset] = std::string();

	if (!_data.count(st_executable) || !_data[st_executable].is_string())
		_data[st_executable] = std::string();

	if (!_data.count(st_show_hidden) || !_data[st_show_hidden].is_boolean())
		_data[st_show_hidden] = false;

	if (!_data.count(st_conflict_resolve_mode) || !_data[st_conflict_resolve_mode].is_number_integer())
		_data[st_conflict_resolve_mode] = ConflictResolveMode::automatic;
}
