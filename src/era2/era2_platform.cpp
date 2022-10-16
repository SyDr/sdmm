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
#include "era2_config.h"
#include "era2_directory_structure.hpp"
#include "era2_launch_helper.h"
#include "era2_mod_data_provider.hpp"
#include "era2_mod_manager.h"
#include "era2_plugin_manager.hpp"
#include "era2_preset_manager.h"
#include "interface/iapp_config.h"
#include "system_info.hpp"
#include "utility/fs_util.h"
#include "utility/json_util.h"
#include "utility/sdlexcept.h"
#include "utility/shell_util.h"
#include "utility/string_util.h"

#include "era2_plugin_helper.hpp"
#include <fmt/format.h>

using namespace mm;

namespace
{
	std::vector<wxString> readFile(const fs::path& path)
	{
		wxTextFile        file;
		if (!file.Open(path.wstring()))
			return {};

		std::vector<wxString> result;
		for (auto& str = file.GetFirstLine(); !file.Eof(); str = file.GetNextLine())
			result.emplace_back(str);

		file.Close();

		return result;
	}

	ModList loadMods(const fs::path& activePath, const fs::path& hiddenPath, const fs::path& modsPath)
	{
		ModList items;

		auto activeMods = readFile(activePath);

		// active mods / ignore mm_managed_mod
		for (const auto& item : boost::adaptors::reverse(activeMods))
		{
			if (item != mm::constant::mm_managed_mod)
			{
				items.active.emplace_back(item);
				items.available.emplace(item);
			}
		}

		// hidden mods in data dir
		for (const auto& item : readFile(hiddenPath))
			items.hidden.emplace(item);

		// remaining items from directory
		for (const auto& item : getAllDirs(modsPath))
		{
			if (const auto it = std::find(activeMods.begin(), activeMods.end(), item); it == activeMods.end())
				items.available.emplace(item);
		}

		items.available.erase(mm::constant::mm_managed_mod);

		return items;
	}

	void saveMods(const fs::path& activePath, const fs::path& hiddenPath, const fs::path& modsPath,
				  const std::vector<wxString>& activeContent, const std::set<wxString>& hiddenContent)
	{
		auto                 reversedRange = activeContent | boost::adaptors::reversed;
		std::deque<wxString> reversed(reversedRange.begin(), reversedRange.end());
		reversed.emplace_back(mm::constant::mm_managed_mod);

		overwriteFileFromContainer(activePath, reversed);
		overwriteFileFromContainer(hiddenPath, hiddenContent);

		std::filesystem::create_directories(modsPath / mm::constant::mm_managed_mod);
		std::filesystem::copy_file(std::filesystem::path(mm::constant::data_dir) / "mod.json",
								   modsPath / mm::constant::mm_managed_mod / "mod.json",
								   std::filesystem::copy_options::overwrite_existing);
		std::filesystem::copy_file(std::filesystem::path(mm::constant::data_dir) / "description.txt",
								   modsPath / mm::constant::mm_managed_mod / "description.txt",
								   std::filesystem::copy_options::overwrite_existing);
		std::filesystem::copy_file(std::filesystem::path(mm::constant::data_dir) / "description_rus.txt",
								   modsPath / mm::constant::mm_managed_mod / "description_rus.txt",
								   std::filesystem::copy_options::overwrite_existing);
	}
}

Era2Platform::Era2Platform(Application const& app)
	: _rootDir(app.appConfig().getDataPath())
	, _app(app)
{
	_localConfig   = std::make_unique<Era2Config>(_rootDir);
	_presetManager = std::make_unique<Era2PresetManager>(_localConfig->getPresetsPath());
	_launchHelper  = std::make_unique<Era2LaunchHelper>(*_localConfig, app.iconStorage());
	_modDataProvider =
		std::make_unique<Era2ModDataProvider>(getModsDirPath(), _app.appConfig().currentLanguageCode());

	_initalModList = _modList = loadMods(getActiveListPath(), getHiddenListPath(), getModsDirPath());
	_modManager               = std::make_unique<Era2ModManager>(_modList);

	_plugins    = era2_plugin_helper::loadPhysicalStructure(getModsDirPath());
	_pluginList = era2_plugin_helper::updateAvailability(_plugins, _modList);
	era2_plugin_helper::loadManagedState(_pluginList, getPluginListPath());
	_initialPluginList = _pluginList;
	_pluginManager     = std::make_unique<Era2PluginManager>(_pluginList);

	_modManager->onListChanged().connect(
		[this]
		{
			// TODO: make better
			era2_plugin_helper::updateAvailability(_pluginList, _plugins, _modList);
			_pluginManager->plugins(_pluginList);
		});
}

std::filesystem::path Era2Platform::getManagedPath() const
{
	return _rootDir;
}

std::filesystem::path Era2Platform::getModsDirPath() const
{
	return _rootDir / "Mods";
}

Era2Config* Era2Platform::localConfig() const
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

std::filesystem::path Era2Platform::getActiveListPath() const
{
	return getModsDirPath() / "list.txt";
}

std::filesystem::path Era2Platform::getHiddenListPath() const
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

INonAutoApplicablePlatform* Era2Platform::nonAutoApplicable()
{
	return this;
}

bool Era2Platform::changed() const
{
	return _modList != _initalModList || _pluginList != _initialPluginList;
}

void Era2Platform::apply()
{
	saveMods(getActiveListPath(), getHiddenListPath(), getModsDirPath(), _modManager->mods().active,
			 _modManager->mods().hidden);

	_initalModList = _modList;

	era2_plugin_helper::saveManagedState(getPluginListPath(), getModsDirPath(), _pluginList);

	_initialPluginList = _pluginList;
}

void Era2Platform::revert()
{
	_modManager->mods(_initalModList);
	_pluginManager->plugins(_initialPluginList);
}
