// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "era2_config.hpp"

#include <fstream>
#include <ranges>
#include <sstream>
#include <unordered_set>

#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/stdpaths.h>

#include "system_info.hpp"
#include "type/mod_list_model_structs.hpp"
#include "utility/fs_util.h"
#include "utility/json_util.h"
#include "utility/sdlexcept.h"

using namespace mm;

namespace
{
	constexpr const auto st_active_preset         = "active_preset";
	constexpr const auto st_executable            = "executable";
	constexpr const auto st_conflict_resolve_mode = "conflict_resolve_mode";
	constexpr const auto st_list_columns          = "list_columns";
	constexpr const auto st_managed_mods_display  = "managed_mods_display";
	constexpr const auto st_archived_mods_display = "archived_mods_display";
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

std::vector<int> Era2Config::listColumns() const
{
	auto result = _data[st_list_columns].get<std::vector<int>>();

	for (size_t i = 0; i < result.size(); ++i)
	{
		const auto v = static_cast<ModListModelColumn>(std::abs(result[i]));

		if (std::ranges::find(MainListColumns, v) == MainListColumns.end())
			result.erase(result.begin() + i);
		else
			++i;
	}

	for (const auto& i : MainListColumns)
		if (!std::ranges::any_of(result, [&](const int v) { return std::abs(v) == static_cast<int>(i); }))
			result.emplace_back(static_cast<int>(i));

	return result;
}

void Era2Config::listColumns(const std::vector<int>& value)
{
	_data[st_list_columns] = value;
	save();
}

ModListModelManagedMode Era2Config::managedModsDisplay() const
{
	return static_cast<ModListModelManagedMode>(_data[st_managed_mods_display]);
}

void Era2Config::managedModsDisplay(ModListModelManagedMode value)
{
	_data[st_managed_mods_display] = static_cast<unsigned>(value);
	save();
}

ModListModelArchivedMode Era2Config::archivedModsDisplay() const
{
	return static_cast<ModListModelArchivedMode>(_data[st_archived_mods_display]);
}

void Era2Config::archivedModsDisplay(ModListModelArchivedMode value)
{
	_data[st_archived_mods_display] = static_cast<unsigned>(value);
	save();
}

void Era2Config::validate()
{
	if (!_data.count(st_active_preset) || !_data[st_active_preset].is_string())
		_data[st_active_preset] = std::string();

	if (!_data.count(st_executable) || !_data[st_executable].is_string())
		_data[st_executable] = std::string();

	if (!_data.count(st_conflict_resolve_mode) || !_data[st_conflict_resolve_mode].is_number_unsigned() ||
		_data[st_conflict_resolve_mode] > static_cast<unsigned>(ConflictResolveMode::automatic))
		_data[st_conflict_resolve_mode] = ConflictResolveMode::automatic;

	if (!_data.count(st_list_columns) || !_data[st_list_columns].is_array())
		_data[st_list_columns] = nlohmann::json::array();

	auto& lc = _data[st_list_columns];

	size_t i = 0;
	while (i < lc.size())
	{
		if (!lc[i].is_number_integer())
			lc.erase(i);
		else
			++i;
	}

	if (!_data.count(st_managed_mods_display) || !_data[st_managed_mods_display].is_number_unsigned() ||
		_data[st_managed_mods_display] > static_cast<unsigned>(ModListModelManagedMode::as_group))
		_data[st_managed_mods_display] = 0;

	if (!_data.count(st_archived_mods_display) || !_data[st_archived_mods_display].is_number_unsigned() ||
		_data[st_archived_mods_display] >
			static_cast<unsigned>(ModListModelArchivedMode::as_individual_groups))
		_data[st_archived_mods_display] = 1;
}
