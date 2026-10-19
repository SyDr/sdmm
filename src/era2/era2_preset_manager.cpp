// SD Mod Manager

// Copyright (c) 2020-2025 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "era2_preset_manager.hpp"

#include "era2_plugin_manager.hpp"
#include "system_info.hpp"
#include "utility/json_util.h"

#include <boost/locale/conversion.hpp>

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
	const auto path = toPath(_rootPath, name);
	const auto data = loadJsonFromFile(path, true);

	return loadPreset(data);
}

PresetData Era2PresetManager::loadPreset(const nlohmann::json& data)
{
	PresetData result;

	if (!data.is_object())
		return result;

	if (auto list = data.find("list"); list != data.end() && list->is_array())
		for (const auto& item : *list)
			result.mods.data.emplace_back(
				ModList::Mod(boost::locale::fold_case(item.get<std::string>()), ModList::ModState::enabled));

//	result.plugins.available = Era2PluginManager::loadAvailablePlugins(_modsPath, result.mods);
//	if (auto plugins = data.find("plugins"); plugins != data.end())
//		result.plugins.managed = Era2PluginManager::loadManagedState(plugins->get<std::string>());

//	erase_if(result.plugins.available,
//		[&](const Plugin& item) { return !result.plugins.managed.contains(item); });

	if (auto exe = data.find("exe"); exe != data.end() && exe->is_string())
		result.executable = exe->get<std::string>();

	return result;
}

nlohmann::json Era2PresetManager::savePreset(const PresetData& preset)
{
	nlohmann::json data;

	data["mm_version"] = PROGRAM_VERSION;
	if (!preset.executable.empty())
		data["exe"] = preset.executable;

	auto& list = data["list"] = nlohmann::json::array();
	for (const auto& item : preset.mods.data)
		if (item.state == ModList::ModState::enabled)
			list.emplace_back(item.id);

//	if (!preset.plugins.managed.empty())
//	{
//		auto& plugins = data["plugins"] = nlohmann::json::array();
//		for (const auto& source : preset.plugins.managed)
//		{
//			const auto path =
//				(fs::path(source.mod) / source.id.name).lexically_normal();
//			plugins.emplace_back(path.string());
//		}
//	}

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
