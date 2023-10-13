// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "era2_platform.h"

#include <fstream>
#include <unordered_set>

#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/range/combine.hpp>
#include <wx/dir.h>
#include <wx/textfile.h>

#include "application.h"
#include "domain/mod_data.hpp"
#include "era2_config.hpp"
#include "era2_directory_structure.hpp"
#include "era2_launch_helper.hpp"
#include "era2_mod_data_provider.hpp"
#include "era2_mod_manager.hpp"
#include "era2_plugin_manager.hpp"
#include "era2_preset_manager.hpp"
#include "interface/iapp_config.hpp"
#include "system_info.hpp"
#include "utility/fs_util.h"
#include "utility/json_util.h"
#include "utility/sdlexcept.h"
#include "utility/shell_util.h"

using namespace mm;

namespace
{
	bool validateModId(const fs::path& modsPath, std::string& id)
	{
		boost::trim(id);
		if (id.empty())
			return false;

		if (id == mm::SystemInfo::ManagedMod)
			return false;

		const auto path = modsPath / id;
		if (!exists(path) || !is_directory(path))
			return false;

		return true;
	}

	ModList loadMods(const fs::path& activePath, const fs::path& hiddenPath, const fs::path& modsPath)
	{
		ModList items;

		// active mods / ignore mm_managed_mod
		std::vector<std::string> activeMods;
		boost::split(activeMods, readFile(activePath), boost::is_any_of("\r\n"));
		for (auto item : boost::adaptors::reverse(activeMods))
		{
			if (validateModId(modsPath, item))
			{
				items.active.emplace_back(item);
				items.available.emplace(item);
			}
			else if (!item.empty() && item != SystemInfo::ManagedMod)
			{
				items.invalid.emplace_back(item);
			}
		}

		// hidden mods in data dir
		std::vector<std::string> hiddenMods;
		boost::split(hiddenMods, readFile(hiddenPath), boost::is_any_of("\\n"));
		for (auto item : hiddenMods)
			if (validateModId(modsPath, item))
				items.hidden.emplace(item);

		// remaining items from directory
		using di = fs::directory_iterator;
		for (auto it = di(modsPath), end = di(); it != end; ++it)
		{
			if (!it->is_directory())
				continue;
			items.available.emplace(it->path().filename().string());
		}

		items.available.erase(mm::SystemInfo::ManagedMod);

		return items;
	}

	void saveMods(
		const fs::path& activePath, const fs::path& hiddenPath, const fs::path& modsPath, const ModList& mods)
	{
		auto                 reversedRange = mods.active | boost::adaptors::reversed;
		std::deque<std::string> reversed(reversedRange.begin(), reversedRange.end());
		reversed.emplace_back(mm::SystemInfo::ManagedMod);
		std::copy(mods.invalid.begin(), mods.invalid.end(), std::back_inserter(reversed));

		overwriteFileFromContainer(activePath, reversed);
		overwriteFileFromContainer(hiddenPath, mods.hidden);

		const auto managedPath = modsPath / mm::SystemInfo::ManagedMod;

		create_directories(modsPath / mm::SystemInfo::ManagedMod);
		copy_file(fs::path(mm::SystemInfo::DataDir) / SystemInfo::ModInfoFilename,
			managedPath / SystemInfo::ModInfoFilename, fs::copy_options::overwrite_existing);
		copy_file(fs::path(mm::SystemInfo::DataDir) / "description.txt", managedPath / "description.txt",
			fs::copy_options::overwrite_existing);
		copy_file(fs::path(mm::SystemInfo::DataDir) / "description_rus.txt",
			managedPath / "description_rus.txt", fs::copy_options::overwrite_existing);
	}
}

Era2Platform::Era2Platform(Application const& app)
	: _app(app)
	, _rootDir(app.appConfig().getDataPath())
{
	_localConfig   = std::make_unique<Era2Config>(_rootDir);
	_presetManager = std::make_unique<Era2PresetManager>(_localConfig->getPresetsPath(), getModsDirPath());
	_launchHelper  = std::make_unique<Era2LaunchHelper>(*_localConfig, app.iconStorage());
	_modDataProvider =
		std::make_unique<Era2ModDataProvider>(getModsDirPath(), _app.appConfig().currentLanguageCode());

	_modList       = loadMods(getActiveListPath(), getHiddenListPath(), getModsDirPath());
	_modManager    = std::make_unique<Era2ModManager>(_modList);
	_pluginManager = std::make_unique<Era2PluginManager>(*_modManager, getModsDirPath(), getPluginListPath());

	_modListChanged    = _modManager->onListChanged().connect([this] { save(); });
	_pluginListChanged = _pluginManager->onListChanged().connect([this] { _pluginManager->save(); });
}

fs::path Era2Platform::managedPath() const
{
	return _rootDir;
}

void Era2Platform::reload()
{
	_modDataProvider->clear();
	auto block = _modListChanged.blocker();

	_modManager->mods(loadMods(getActiveListPath(), getHiddenListPath(), getModsDirPath()));
	_modManager->onListChanged()();
}

void Era2Platform::apply(ModList* mods, PluginList* plugins)
{
	auto block1 = _modListChanged.blocker();
	auto block2 = _pluginListChanged.blocker();

	if (mods)
	{
		_modManager->mods(*mods);
		if (plugins)
			_pluginManager->plugins(*plugins);

		_modManager->onListChanged()();
	}
	else if (plugins)
	{
		_pluginManager->plugins(*plugins);
		_pluginManager->onListChanged()();
	}

	if (mods || plugins)
		save();
}

fs::path Era2Platform::getModsDirPath() const
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
	return getModsDirPath() / "list.txt";
}

fs::path Era2Platform::getHiddenListPath() const
{
	return _localConfig->getProgramDataPath() / "hidden_mods.txt";
}

IPluginManager* Era2Platform::pluginManager() const
{
	return _pluginManager.get();
}

fs::path Era2Platform::getPluginListPath() const
{
	return _localConfig->getProgramDataPath() / "plugins.json";
}

void Era2Platform::save()
{
	saveMods(getActiveListPath(), getHiddenListPath(), getModsDirPath(), _modManager->mods());
	_pluginManager->save();
}
