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

#include <boost/locale.hpp>

using namespace mm;

namespace
{
	const std::set<std::string> PluginExts         = { ".dll", ".bin", ".era" };
	constexpr const auto        PluginOffExtension = ".off";
	constexpr const auto        PluginSubdir       = "EraPlugins";
	constexpr const std::array  PluginDirs         = { ".", "BeforeWog", "AfterWog" };

	bool isPlugin(fs::path path)
	{
		if (path.extension().string() == PluginOffExtension)
			path = path.parent_path() / path.filename().replace_extension();

		return path.has_extension() && PluginExts.contains(path.extension().string());
	}

	std::vector<Plugin> loadModPlugins(const fs::path& basePath, const std::string& modId)
	{
		const auto modPath = (basePath / modId / PluginSubdir).lexically_normal();

		std::vector<Plugin> result;

		using di = fs::directory_iterator;
		for (const auto& dir : PluginDirs)
		{
			const auto subPath = (modPath / dir).lexically_normal();

			if (!is_directory(subPath))
				continue;

			for (auto it = di(subPath), end = di(); it != end; ++it)
			{
				const auto& pluginPath = it->path();

				if (!is_regular_file(pluginPath))
					continue;

				if (!isPlugin(pluginPath))
					continue;

				const auto path = fs::relative(pluginPath, modPath);

				result.emplace_back(Plugin(modId, path));
			}
		}

		return result;
	}

	std::optional<Plugin> sourceMod(const PluginId& id, const PluginList& list, const ModList& mods)
	{
		const auto it = list.available.find(id);
		if (it == list.available.end())
			return {};

		for (const auto& [modId, v] : it->second)
			if (mods.enabled(modId))
				return Plugin(modId, v.v);

		return {};
	}

	bool inActiveSource(const PluginId& id, const PluginList& list, const ModList& mods)
	{
		return sourceMod(id, list, mods) != std::nullopt;
	}

	ManagedPlugins loadManagedState(const fs::path& modsPath)
	{
		ManagedPlugins result;

		const auto currentState = ::loadModPlugins(modsPath, SystemInfo::ManagedMod);

		for (const auto& item : currentState)
		{
			const auto state = fs::file_size(modsPath / item.modId / PluginSubdir / item.source.v)
								   ? PluginState::forced_enable
								   : PluginState::forced_disable;

			result[item.id] = state;
		}

		return result;
	}

	void saveManagedState(const fs::path& modsPath, const PluginList& list, const ModList& mods)
	{
		const auto targetPath = modsPath / SystemInfo::ManagedMod / PluginSubdir;

		for (const auto& dir : PluginDirs)
		{
			const auto subPath = targetPath / dir;
			create_directories(subPath);
		}

		const auto currentState = ::loadModPlugins(modsPath, SystemInfo::ManagedMod);

		/// remove all overridden
		/// FIXME: overridden to disable -> only check size
		/// FIXME: overridden to enable -> check number of sources (if 1 -> assume unchanged or check size)

		boost::system::error_code ec;
		for (const auto& plugin : currentState)
			remove(targetPath / plugin.source.v, ec);

		for (const auto& [id, state] : list.managed)
		{
			const auto source = sourceMod(id, list, mods);
			if (!source)
				continue;

			const auto copyFrom = modsPath / source->modId / PluginSubdir / source->source.v;
			const auto copyTo   = targetPath / id.v;

			if (state == PluginState::forced_enable)
				copy_file(copyFrom, copyTo, fs::copy_options::overwrite_existing);
			else
				boost::nowide::ofstream unused(copyTo);
		}
	}
}

Era2PluginManager::Era2PluginManager(Era2ModManager& modManager, const fs::path& modsDir)
	: _modManager { modManager }
	, _modsDir { modsDir }
	, _pluginList { .available = loadAvailablePlugins(modsDir), .managed = loadManagedState(modsDir) }
{
	_modManager.onListChanged().connect([this] {
		_pluginList.available = loadAvailablePlugins(_modsDir);
		_listChanged();
	});
}

const PluginList& Era2PluginManager::plugins() const
{
	return _pluginList;
}

void Era2PluginManager::save()
{
	saveManagedState(_modsDir, _pluginList, _modManager.mods());
}

sigslot::signal<>& Era2PluginManager::onListChanged()
{
	return _listChanged;
}

void Era2PluginManager::plugins(PluginList items)
{
	_pluginList = std::move(items);
}

AvailablePlugins Era2PluginManager::loadAvailablePlugins(const fs::path& basePath)
{
	AvailablePlugins result;

	if (!exists(basePath))
		return result;

	using di = fs::directory_iterator;
	for (auto it = di(basePath), end = di(); it != end; ++it)
	{
		if (!it->is_directory())
			continue;

		const auto modId = it->path().filename().string();
		if (modId == mm::SystemInfo::ManagedMod)
			continue;

		const auto fromMod = ::loadModPlugins(basePath, modId);

		const auto foldedId = boost::locale::fold_case(modId);

		for (const auto& item : fromMod)
		{
			result[item.id][foldedId] = item.source;
		}
	}

	return result;
}

void Era2PluginManager::enable(const PluginId& id)
{
	_pluginList.managed[id] = PluginState::forced_enable;

	save();

	_listChanged();
}

void Era2PluginManager::disable(const PluginId& id)
{
	_pluginList.managed[id] = PluginState::forced_disable;

	save();

	_listChanged();
}

void Era2PluginManager::reset(const PluginId& id)
{
	_pluginList.managed.erase(id);

	save();

	_listChanged();
}
