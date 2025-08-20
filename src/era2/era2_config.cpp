// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "era2_config.hpp"

#include "system_info.hpp"
#include "type/mod_list_model_structs.hpp"
#include "type/program_version.hpp"
#include "utility/fs_util.h"
#include "utility/json_util.h"
#include "utility/sdlexcept.h"

#include <fstream>
#include <ranges>
#include <sstream>
#include <unordered_set>

#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/stdpaths.h>

using namespace mm;

namespace
{
	constexpr const auto st_active_preset         = "active_preset";
	constexpr const auto st_executable            = "executable";
	constexpr const auto st_conflict_resolve_mode = "conflict_resolve_mode";
	constexpr const auto st_managed_mods_display  = "managed_mods_display";
	constexpr const auto st_archived_mods_display = "archived_mods_display";
	constexpr const auto st_collapsed_categories  = "collapsed_categories";
	constexpr const auto st_hidden_categories     = "hidden_categories";
	constexpr const auto st_screenshots_expanded  = "screenshots_expanded";
}

namespace
{
	namespace Key
	{
		inline constexpr auto MMVersion   = "mm_version";
		inline constexpr auto ListColumns = "list_columns";
	}
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
	// create_directories(getTempPath());
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
	auto result = _data[Key::ListColumns].get<std::vector<int>>();

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
	_data[Key::ListColumns] = value;
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

std::set<ModListDsplayedData::GroupItemsBy> Era2Config::collapsedCategories() const
{
	std::set<ModListDsplayedData::GroupItemsBy> result;

	for (const auto& item : _data[st_collapsed_categories])
		result.emplace(ModListDsplayedData::StringToGroupItemsBy(item.get<std::string>()));

	return result;
}

void Era2Config::collapsedCategories(const std::set<ModListDsplayedData::GroupItemsBy>& value)
{
	_data[st_collapsed_categories] = nlohmann::json::array();

	for (const auto& item : value)
		_data[st_collapsed_categories].emplace_back(ModListDsplayedData::GroupItemsByToString(item));

	save();
}

std::set<std::string> Era2Config::hiddenCategories() const
{
	std::set<std::string> result;

	for (const auto& item : _data[st_hidden_categories])
		result.emplace(item.get<std::string>());

	return result;
}

void Era2Config::hiddenCategories(const std::set<std::string>& value)
{
	_data[st_hidden_categories] = nlohmann::json::array();

	for (const auto& item : value)
		_data[st_hidden_categories].emplace_back(item);

	save();
}

bool Era2Config::screenshotsExpanded() const
{
	return _data[st_screenshots_expanded].get<bool>();
}

void Era2Config::screenshotsExpanded(bool value)
{
	_data[st_screenshots_expanded] = value;
	save();
}

void Era2Config::validate()
{
	if (_data.is_discarded())
		_data = { { Key::MMVersion, PROGRAM_VERSION_BASE } };

	if (!_data.count(st_active_preset) || !_data[st_active_preset].is_string())
		_data[st_active_preset] = std::string();

	if (!_data.count(st_executable) || !_data[st_executable].is_string())
		_data[st_executable] = std::string();

	if (!_data.count(st_conflict_resolve_mode) || !_data[st_conflict_resolve_mode].is_number_unsigned() ||
		_data[st_conflict_resolve_mode] > static_cast<unsigned>(ConflictResolveMode::automatic))
		_data[st_conflict_resolve_mode] = ConflictResolveMode::automatic;

	if (!_data.count(Key::ListColumns) || !_data[Key::ListColumns].is_array())
		_data[Key::ListColumns] = nlohmann::json::array();

	auto& lc = _data[Key::ListColumns];

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

	if (!_data.count(st_collapsed_categories) || !_data[st_collapsed_categories].is_array())
		_data[st_collapsed_categories] = nlohmann::json::array();

	for (auto& item : _data[st_collapsed_categories])
		if (!item.is_string())
			item = "";

	if (!_data.count(st_hidden_categories) || !_data[st_hidden_categories].is_array())
	{
		_data[st_hidden_categories] = nlohmann::json::array();
		_data[st_hidden_categories].emplace_back("plugins");
	}

	for (auto& item : _data[st_hidden_categories])
		if (!item.is_string())
			item = "";

	if (!_data.count(st_screenshots_expanded) || !_data[st_screenshots_expanded].is_boolean())
		_data[st_screenshots_expanded] = true;

	if (!_data.count(Key::MMVersion) || !_data[Key::MMVersion].is_string())
		_data[Key::MMVersion] = "";

	auto cfgVersion = ProgramVersion(_data[Key::MMVersion].get<std::string>());
	if (cfgVersion < ProgramVersion(0, 98, 69))
	{
		// 0.98.69: added new column before author

		for (auto& value : lc)
		{
			auto v = value.get<int>();
			auto c = static_cast<ModListModelColumn>(std::abs(v));

			// author was 3 (or -3 if disabled) -> now it's 4 (or - 4)
			if (c >= ModListModelColumn::support)
			{
				v     = v > 0 ? v + 1 : v - 1;
				value = v;
			}
		}

		for (auto it = lc.begin(); it < lc.end(); ++it)
		{
			const auto v = it->get<int>();
			const auto c = static_cast<ModListModelColumn>(std::abs(v));

			if (c != ModListModelColumn::author)
				continue;

			lc.insert(it, static_cast<int>(ModListModelColumn::support) * (v > 0 ? 1 : -1));
			break;
		}
	}

	if (ProgramVersion(PROGRAM_VERSION_BASE) > cfgVersion)
		_data[Key::MMVersion] = PROGRAM_VERSION_BASE;
}
