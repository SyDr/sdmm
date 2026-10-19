// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "era2_platform.h"
#include "application.h"
#include "era2_config.hpp"
#include "era2_launch_helper.hpp"
#include "era2_mod_data_provider.hpp"
#include "era2_mod_manager.hpp"
#include "era2_plugin_manager.hpp"
#include "era2_preset_manager.hpp"
#include "interface/iapp_config.hpp"
#include "utility/fs_util.h"
#include "system_info.hpp"

#include <fstream>

#include <boost/locale/conversion.hpp>
#include <boost/range/adaptor/reversed.hpp>

using namespace mm;

namespace
{
	std::unordered_map<std::string, std::string> loadFsMapNames(const fs::path& modsPath)
	{
		std::unordered_map<std::string, std::string> result;

		if (exists(modsPath))
		{
			using di = fs::directory_iterator;
			for (auto it = di(modsPath), end = di(); it != end; ++it)
			{
				if (!it->is_directory())
					continue;

				const auto item = it->path().filename().string();

				result[boost::locale::fold_case(item)] = item;
			}
		}

		return result;
	}

	bool validateModId(std::string& id, ModList::ModState& state)
	{
		boost::trim(id);
		if (id.empty())
			return false;

		state = ModList::ModState::enabled;
		if (id.starts_with('*'))
		{
			id    = id.substr(1);
			state = ModList::ModState::disabled;
		}

		if (id == boost::locale::fold_case(SystemInfo::ManagedMod))
			return false;

		return true;
	}

	ModList loadMods(const fs::path& activePath, const fs::path& modsPath)
	{
		ModList           items;
		ModList::ModState state = ModList::ModState::enabled;

		// active mods / ignore mm_managed_mod
		std::vector<std::string> activeMods;
		boost::split(activeMods, readFile(activePath), boost::is_any_of("\r\n"));

		for (auto& item : boost::adaptors::reverse(activeMods))
		{
			auto id = boost::locale::fold_case(item);

			if (validateModId(id, state) && !items.managed(id))
				items.data.emplace_back(id, state);
		}

		// remaining items from directory
		if (exists(modsPath))
		{
			using di = fs::directory_iterator;
			for (auto it = di(modsPath), end = di(); it != end; ++it)
			{
				if (!it->is_directory())
					continue;

				const auto item = it->path().filename().string();
				const auto id   = boost::locale::fold_case(item);

				if (!items.managed(id))
					items.rest.emplace(id);
			}
		}

		return items;
	}

	void saveMods(const fs::path& activePath, const fs::path& modsPath, const ModList& mods)
	{
		auto map = loadFsMapNames(modsPath);

		std::vector<std::string> toSave;

		for (const auto& item : boost::adaptors::reverse(mods.data))
		{
			auto value = map[item.id];
			if (value.empty())
				value = item.id;

			switch (item.state)
			{
			case ModList::ModState::enabled: toSave.emplace_back(value); break;
			case ModList::ModState::disabled: toSave.emplace_back('*' + value); break;
			}
		}

		toSave.emplace_back(SystemInfo::ManagedMod);

		overwriteFileFromContainer(activePath, toSave);
	}
}

Era2Platform::Era2Platform(const Application& app)
	: _app(app)
	, _rootDir(app.appConfig().getDataPath())
{
	_localConfig     = std::make_unique<Era2Config>(_rootDir);
	_presetManager   = std::make_unique<Era2PresetManager>(_localConfig->getPresetsPath(), modsDirPath());
	_launchHelper    = std::make_unique<Era2LaunchHelper>(*_localConfig);
	_modDataProvider = std::make_unique<Era2ModDataProvider>(modsDirPath(),
		loadFsMapNames(modsDirPath()), _app.appConfig().currentLanguageCode(), _app.i18nService());

	_modList    = loadMods(getActiveListPath(), modsDirPath());
	_modManager = std::make_unique<Era2ModManager>(_modList);
	_pluginManager = std::make_unique<Era2PluginManager>(*_modManager, modsDirPath());

	_modListChanged = _modManager->onListChanged().connect([this] { save(); });
	_pluginListChanged = _pluginManager->onListChanged().connect([this] { _pluginManager->save(); });
}

fs::path Era2Platform::managedPath() const
{
	return _rootDir;
}

void Era2Platform::reload(bool force)
{
	auto mods = loadMods(getActiveListPath(), modsDirPath());
	if (!force && mods == _modManager->mods())
		return;

	_modDataProvider->clear();
	auto block = _modListChanged.blocker();

	_modManager->mods(mods);
	_modManager->onListChanged()();
}

void Era2Platform::apply(ModList* mods, PluginList* plugins)
{
	auto block1 = _modListChanged.blocker();
	auto block2 = _pluginListChanged.blocker();

	if (mods)
	{
		std::vector<std::string> items;

		for (const auto& item : mods->data)
			if (item.state == ModList::ModState::enabled)
				items.emplace_back(item.id);

		_modManager->apply(items, _localConfig->useLegacyArchiving());
		_modManager->onListChanged()();
	}

	if (plugins)
	{
		_pluginManager->plugins(*plugins);
		_pluginManager->onListChanged()();
	}

	if (mods || plugins)
		save();
}

fs::path Era2Platform::modsDirPath() const
{
	return _rootDir / "Mods";
}

ILocalConfig* Era2Platform::localConfig() const
{
	return _localConfig.get();
}

IPresetManager* Era2Platform::getPresetManager() const
{
	return _presetManager.get();
}

IModManager* Era2Platform::modManager() const
{
	return _modManager.get();
}

ILaunchHelper* Era2Platform::launchHelper() const
{
	return _launchHelper.get();
}

IModDataProvider* Era2Platform::modDataProvider() const
{
	return _modDataProvider.get();
}

fs::path Era2Platform::getActiveListPath() const
{
	return modsDirPath() / "list.txt";
}

IPluginManager* Era2Platform::pluginManager() const
{
	return _pluginManager.get();
}

void Era2Platform::save()
{
	saveMods(getActiveListPath(), modsDirPath(), _modManager->mods());
	_pluginManager->save();
}
