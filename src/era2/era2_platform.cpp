// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "era2_platform.h"

#include <fstream>
#include <unordered_set>

#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/algorithm_ext/erase.hpp>

#include "application.h"
#include "domain/mod_data.hpp"
#include "era2_config.hpp"
#include "era2_directory_structure.hpp"
#include "era2_launch_helper.hpp"
#include "era2_mod_data_provider.hpp"
#include "era2_mod_manager.hpp"
#include "era2_preset_manager.hpp"
#include "interface/iapp_config.hpp"
#include "system_info.hpp"
#include "utility/fs_util.h"
#include "utility/json_util.h"
#include "utility/sdlexcept.h"

#include <boost/locale/conversion.hpp>

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

		overwriteFileFromContainer(activePath, toSave);
	}
}

Era2Platform::Era2Platform(const Application& app)
	: _app(app)
	, _rootDir(app.appConfig().getDataPath())
{
	_localConfig     = std::make_unique<Era2Config>(_rootDir);
	_presetManager   = std::make_unique<Era2PresetManager>(_localConfig->getPresetsPath(), getModsDirPath());
	_launchHelper    = std::make_unique<Era2LaunchHelper>(*_localConfig);
	_modDataProvider = std::make_unique<Era2ModDataProvider>(getModsDirPath(),
		loadFsMapNames(getModsDirPath()), _app.appConfig().currentLanguageCode(), _app.i18nService());

	_modList    = loadMods(getActiveListPath(), getModsDirPath());
	_modManager = std::make_unique<Era2ModManager>(_modList);

	_modListChanged = _modManager->onListChanged().connect([this] { save(); });
}

fs::path Era2Platform::managedPath() const
{
	return _rootDir;
}

void Era2Platform::reload(bool force)
{
	auto mods = loadMods(getActiveListPath(), getModsDirPath());
	if (!force && mods == _modManager->mods())
		return;

	_modDataProvider->clear();
	auto block = _modListChanged.blocker();

	_modManager->mods(mods);
	_modManager->onListChanged()();
}

void Era2Platform::apply(const std::vector<std::string>& active)
{
	auto block1 = _modListChanged.blocker();

	_modManager->apply(active);
	_modManager->onListChanged()();

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

fs::path Era2Platform::getPluginListPath() const
{
	return _localConfig->getProgramDataPath() / "plugins.json";
}

void Era2Platform::save()
{
	saveMods(getActiveListPath(), getModsDirPath(), _modManager->mods());
}
