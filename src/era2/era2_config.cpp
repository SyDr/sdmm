// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "era2_config.h"

#include <fstream>
#include <sstream>
#include <unordered_set>

#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/stdpaths.h>

#include "utility/fs_util.h"
#include "utility/sdlexcept.h"
#include "utility/string_util.h"

using namespace mm;

namespace
{
	constexpr const auto st_active_preset         = "active_preset";
	constexpr const auto st_executable            = "executable";
	constexpr const auto st_show_hidden           = "show_hidden";
	constexpr const auto st_conflict_resolve_mode = "conflict_resolve_mode";
}

Era2Config::Era2Config(const std::filesystem::path& path)
	: _path(path)
{
	createDirectories();

	if (std::ifstream datafile(getConfigFilePath().string()); datafile)
	{
		/* std::stringstream stream;
		stream << datafile.rdbuf();
		datafile.close();*/

		try
		{
			_data = nlohmann::json::parse(datafile);
		}
		catch (...)
		{
			wxLogDebug("Can't parse config file. Default config is used instead");
		}
	}
	else
		wxLogDebug("Can't load config file. Default config is used instead");

	validate();
}

void Era2Config::createDirectories() const
{
	std::vector<std::filesystem::path> dirs;
	dirs.emplace_back(getProgramDataPath());
	dirs.emplace_back(getPresetsPath());
	dirs.emplace_back(getTempPath());

	for (const auto& dir : dirs)
		createDir(dir);
}

void Era2Config::save()
{
	overwriteFileContent(getConfigFilePath(), wxString::FromUTF8(_data.dump(2)));
}

std::filesystem::path Era2Config::getDataPath() const
{
	return _path;
}

std::filesystem::path Era2Config::getProgramDataPath() const
{
	return getDataPath() / "_MM_Data";
}

std::filesystem::path Era2Config::getPresetsPath() const
{
	return getProgramDataPath() / "Profiles";
}

std::filesystem::path Era2Config::getTempPath() const
{
	return getProgramDataPath() / "Temp";
}

std::filesystem::path Era2Config::getConfigFilePath() const
{
	return getProgramDataPath() / "config.json";
}

wxString Era2Config::getLaunchString() const
{
	return (getDataPath() / getExecutable().ToStdString()).string();
}

wxString Era2Config::getExecutable() const
{
	return utf8_to_wxString(_data[st_executable].get<std::string>());
}

void Era2Config::setExecutable(const wxString& executable)
{
	_data[st_executable] = wxString_to_utf8(executable);
	save();
}

wxString Era2Config::getAcitvePreset() const
{
	return _data[st_active_preset].get<std::string>();
}

void Era2Config::setActivePreset(const wxString& preset)
{
	_data[st_active_preset] = preset.ToStdString();
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
	case ConflictResolveMode::undefined:
	case ConflictResolveMode::automatic:
	case ConflictResolveMode::manual: return mode;
	}

	return ConflictResolveMode::undefined;
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
		_data[st_conflict_resolve_mode] = ConflictResolveMode::undefined;
}
