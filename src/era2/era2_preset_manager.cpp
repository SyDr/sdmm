// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "era2_preset_manager.hpp"

#include "application.h"
#include "era2_config.hpp"
#include "era2_mod_manager.hpp"
#include "era2_platform.h"
#include "era2_plugin_manager.hpp"
#include "utility/fs_util.h"
#include "utility/sdlexcept.h"
#include "utility/json_util.h"
#include "system_info.hpp"

#include <wx/dir.h>
#include <wx/file.h>

using namespace mm;

namespace
{
	fs::path toPath(const fs::path& base, const std::string& name)
	{
		return base / (name + ".json");
	}
}

Era2PresetManager::Era2PresetManager(fs::path rootPath, fs::path modsPath)
	: _rootPath(std::move(rootPath))
	, _modsPath(std::move(modsPath))
{}

void Era2PresetManager::copy(const std::string& from, const std::string& to)
{
	const auto pathFrom = toPath(_rootPath, from);
	const auto pathTo   = toPath(_rootPath, to);

	fs::copy(pathFrom, pathTo);

	_listChanged();
}

void Era2PresetManager::remove(const std::string& name)
{
	const auto path = toPath(_rootPath, name);

	fs::remove(path);

	_listChanged();
}

std::set<std::string> Era2PresetManager::list() const
{
	std::set<std::string> result;

	fs::directory_iterator di(_rootPath);

	for (const auto end = fs::directory_iterator(); di != end; ++di)
	{
		if (fs::is_regular_file(*di) && di->path().extension() == ".json")
			result.emplace(di->path().stem().string());
	}

	return result;
}

void Era2PresetManager::rename(const std::string& from, const std::string& to)
{
	const auto pathFrom = toPath(_rootPath, from);
	const auto pathTo   = toPath(_rootPath, to);

	fs::rename(pathFrom, pathTo);

	_listChanged();
}

sigslot::signal<>& Era2PresetManager::onListChanged()
{
	return _listChanged;
}

PresetData Era2PresetManager::loadPreset(const std::string& name)
{
	PresetData result;

	const auto path = toPath(_rootPath, name);
	const auto data = loadJsonFromFile(path, true);

	if (!data.is_object())
		return result;

	if (auto active = data.find("list"); active != data.end() && active->is_array())
		for (const auto& item : *active)
			result.mods.active.emplace_back(item.get<std::string>());

	if (auto hidden = data.find("hidden"); hidden != data.end() && hidden->is_array())
		for (const auto& item : *hidden)
			result.mods.hidden.emplace(item.get<std::string>());

	result.plugins.available = Era2PluginManager::loadAvailablePlugins(_modsPath, result.mods);
	if (auto plugins = data.find("plugins"); plugins != data.end())
		result.plugins.managed = Era2PluginManager::loadManagedState(*plugins);

	erase_if(result.plugins.available,
		[&](const PluginSource& item) { return !result.plugins.managed.contains(item); });

	if (auto exe = data.find("exe"); exe != data.end() && exe->is_string())
		result.executable = exe->get<std::string>();

	return result;
}

nlohmann::json Era2PresetManager::savePreset(const PresetData& preset)
{
	nlohmann::json data;

	data["mm_version"]     = PROGRAM_VERSION;

	if (!preset.mods.active.empty())
	{
		auto& ref = data["list"] = nlohmann::json::array();
		for (const auto& item : preset.mods.active)
			ref.emplace_back(item);
	}
	if (!preset.mods.hidden.empty())
	{
		auto& ref = data["hidden"] = nlohmann::json::array();
		for (const auto& item : preset.mods.hidden)
			ref.emplace_back(item);
	}

	if (!preset.plugins.managed.empty())
	{
		auto& ref = data["plugins"] = nlohmann::json::array();
		for (const auto& source : preset.plugins.managed)
		{
			const auto path =
				(fs::path(source.modId) / to_string(source.location) / source.name).lexically_normal();
			ref.emplace_back(path.string());
		}
	}

	if (!preset.executable.empty())
		data["exe"] = preset.executable;

	return data;
}

void Era2PresetManager::savePreset(const std::string& name, const PresetData& preset)
{
	nlohmann::json data = savePreset(preset);

	const auto path          = toPath(_rootPath, name);
	const bool already_exist = fs::exists(path);

	boost::nowide::ofstream datafile(path);
	datafile << data.dump(2);
	datafile.close();

	if (!already_exist)
		_listChanged();
}

bool Era2PresetManager::exists(const std::string& name) const
{
	return fs::exists(toPath(_rootPath, name));
}
