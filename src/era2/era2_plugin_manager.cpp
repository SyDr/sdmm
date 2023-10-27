// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "application.h"
#include "domain/mod_data.hpp"
#include "era2_config.hpp"
#include "era2_mod_manager.hpp"
#include "era2_plugin_manager.hpp"
#include "system_info.hpp"
#include "utility/fs_util.h"
#include "utility/json_util.h"
#include "utility/sdlexcept.h"

using namespace mm;

namespace
{
	const std::set<std::string> PluginExts         = { ".dll", ".bin", ".era" };
	constexpr const auto        PluginOffExtension = ".off";
	constexpr const auto        PluginSubdir       = "EraPlugins";
	constexpr const std::array  PluginDirs         = { ".", "BeforeWog", "AfterWog" };

	PluginLocation fromPluginDir(const std::string& value)
	{
		if (boost::iequals(value, "BeforeWog"))
			return PluginLocation::before_wog;
		if (boost::iequals(value, "AfterWog"))
			return PluginLocation::after_wog;

		return PluginLocation::root;
	}

	bool isPlugin(fs::path path)
	{
		if (path.extension().string() == PluginOffExtension)
			path = path.filename().replace_extension();

		return path.has_extension() && PluginExts.contains(path.extension().string());
	}

	std::set<PluginSource> loadManagedState(const fs::path& pluginPath)
	{
		boost::nowide::ifstream datafile(pluginPath.string());
		if (!datafile)
			return {};

		auto data = nlohmann::json::parse(datafile, nullptr, false);

		return Era2PluginManager::loadManagedState(data);
	}

	std::set<PluginSource> loadAvailablePlugins(const fs::path& basePath, const std::string& modId)
	{
		std::set<PluginSource> result;

		using di = fs::directory_iterator;
		for (const auto& dir : PluginDirs)
		{
			const auto subPath = basePath / modId / PluginSubdir / dir;
			if (!is_directory(subPath))
				continue;

			for (auto it = di(subPath), end = di(); it != end; ++it)
			{
				auto& pluginPath = it->path();

				if (!is_regular_file(pluginPath))
					continue;

				if (!isPlugin(pluginPath))
					continue;

				result.emplace(PluginSource(modId, fromPluginDir(dir), pluginPath.filename().string()));
			}
		}

		return result;
	}

	void saveManagedState(
		const fs::path& pluginPath, const fs::path& modsPath, const PluginList& list, const ModList& modList)
	{
		const auto targetPath = modsPath / SystemInfo::ManagedMod / PluginSubdir;
		for (const auto& dir : PluginDirs)
		{
			const auto subPath = targetPath / dir;
			create_directories(subPath);
		}

		//const auto currentState = ::loadAvailablePlugins(modsPath, SystemInfo::ManagedMod);
		const auto managedState = ::loadManagedState(pluginPath);

		boost::system::error_code ec;
		for (const auto& source : managedState)
		{
			const auto target = targetPath / source.toFileIdenity();
			if (!exists(target))
				continue;

			// remove if not managed anymore
			// or parent mod is not active
			if (list.managed.contains(source) && modList.isActive(source.modId))
				continue;

			remove(target, ec);
		}

		for (const auto& source : list.managed)
		{
			const auto copyFrom =
				modsPath / source.modId / PluginSubdir / to_string(source.location) / source.name;
			const auto copyTo = targetPath / source.toFileIdenity();

			// assume already copied
			if (exists(copyTo) && managedState.contains(source))
				continue;

			if (!source.active())
				copy_file(copyFrom, copyTo, fs::copy_options::overwrite_existing);
			else
				boost::nowide::ofstream unused(copyTo);
		}

		nlohmann::json data = nlohmann::json::array();
		for (const auto& source : list.managed)
		{
			const auto path =
				(fs::path(source.modId) / to_string(source.location) / source.name).lexically_normal();
			data.emplace_back(path.string());
		}

		overwriteFile(pluginPath, data.dump(2));
	}
}

Era2PluginManager::Era2PluginManager(
	Era2ModManager& modManager, const fs::path& modsDir, const fs::path& listPath)
	: _modManager(modManager)
	, _modsDir(modsDir)
	, _listPath(listPath)
{
	_pluginList.available = loadAvailablePlugins(_modsDir, _modManager.mods());
	_pluginList.managed   = ::loadManagedState(_listPath);

	_modManager.onListChanged().connect([this] {
		_pluginList.available = loadAvailablePlugins(_modsDir, _modManager.mods());
		_listChanged();
	});
}

const PluginList& Era2PluginManager::plugins() const
{
	return _pluginList;
}

void Era2PluginManager::switchState(const PluginSource& plugin)
{
	_pluginList.switchState(plugin);

	_listChanged();
}

void Era2PluginManager::save()
{
	saveManagedState(_listPath, _modsDir, _pluginList, _modManager.mods());
}

sigslot::signal<>& Era2PluginManager::onListChanged()
{
	return _listChanged;
}

void Era2PluginManager::plugins(PluginList items)
{
	_pluginList = std::move(items);
}

std::set<PluginSource> Era2PluginManager::loadAvailablePlugins(const fs::path& basePath, const ModList& mods)
{
	std::set<PluginSource> result;

	using di = fs::directory_iterator;
	for (auto it = di(basePath), end = di(); it != end; ++it)
	{
		if (!it->is_directory())
			continue;

		const auto modId = it->path().filename().string();
		if (modId == mm::SystemInfo::ManagedMod || !mods.isActive(modId))
			continue;

		const auto fromMod = ::loadAvailablePlugins(basePath, modId);
		result.insert(fromMod.cbegin(), fromMod.cend());
	}

	return result;
}

std::set<PluginSource> Era2PluginManager::loadManagedState(const nlohmann::json& from)
{
	if (!from.is_array())
		return {};

	std::set<PluginSource> result;
	for (const auto& item : from)
	{
		fs::path asPath(item.get<std::string>());

		std::vector<fs::path> items;
		for (const auto& path : asPath)
			items.emplace_back(path);

		if (items.size() != 2 && items.size() != 3)
			continue;

		result.emplace(PluginSource(items.front().string(),
			items.size() == 3 ? fromPluginDir(items[1].string()) : PluginLocation::root,
			items.back().string()));
	}

	return result;
}
