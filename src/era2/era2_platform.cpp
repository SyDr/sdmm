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

using namespace mm;

namespace
{
	bool validateModId(const fs::path& modsPath, wxString& id)
	{
		id.Trim();
		if (id.empty())
			return false;

		if (id == mm::constant::mm_managed_mod)
			return false;

		const auto path = modsPath / id.ToStdWstring();
		if (!fs::exists(path) || !fs::is_directory(path))
			return false;

		return true;
	}

	ModList loadMods(const fs::path& activePath, const fs::path& hiddenPath, const fs::path& modsPath)
	{
		ModList items;

		auto activeMods = readFile(activePath);

		// active mods / ignore mm_managed_mod
		for (auto item : boost::adaptors::reverse(activeMods))
		{
			if (validateModId(modsPath, item))
			{
				items.active.emplace_back(item);
				items.available.emplace(item);
			}
			else if (!item.empty() && item != constant::mm_managed_mod)
			{
				items.invalid.emplace_back(item);
			}
		}

		// hidden mods in data dir
		for (auto item : readFile(hiddenPath))
			if (validateModId(modsPath, item))
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
				  const ModList& mods)
	{
		auto                 reversedRange = mods.active | boost::adaptors::reversed;
		std::deque<wxString> reversed(reversedRange.begin(), reversedRange.end());
		reversed.emplace_back(mm::constant::mm_managed_mod);
		std::copy(mods.invalid.begin(), mods.invalid.end(), std::back_inserter(reversed));

		overwriteFileFromContainer(activePath, reversed);
		overwriteFileFromContainer(hiddenPath, mods.hidden);

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
	_pluginManager = std::make_unique<Era2PluginManager>(*_modManager, getModsDirPath(), getPluginListPath());
}

std::filesystem::path Era2Platform::getManagedPath() const
{
	return _rootDir;
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

INonAutoApplicablePlatform* Era2Platform::nonAutoApplicable()
{
	return this;
}

bool Era2Platform::changed() const
{
	return _modList != _initalModList || _pluginManager->changed();
}

void Era2Platform::apply()
{
	saveMods(getActiveListPath(), getHiddenListPath(), getModsDirPath(), _modManager->mods());

	_initalModList = _modList;

	_pluginManager->save();
}

void Era2Platform::revert()
{
	_modManager->mods(_initalModList);
	_pluginManager->revert();
}
