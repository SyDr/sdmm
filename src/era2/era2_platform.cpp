// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "era2_platform.h"

#include <fstream>
#include <unordered_set>

#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#include <wx/dir.h>

#include "application.h"
#include "domain/mod_data.hpp"
#include "era2_config.h"
#include "era2_directory_structure.hpp"
#include "era2_launch_helper.h"
#include "era2_mod_data_provider.hpp"
#include "era2_mod_manager.h"
#include "era2_preset_manager.h"
#include "interface/service/iapp_config.h"
#include "utility/fs_util.h"
#include "utility/json_util.h"
#include "utility/sdlexcept.h"
#include "utility/shell_util.h"
#include "utility/string_util.h"

#include <fmt/format.h>

namespace
{
	// TODO: move somewhere else
	template <typename C>
	void rewriteFile(std::filesystem::path const& path, C const& content)
	{
		if (std::filesystem::exists(path))
			std::filesystem::remove(path);

		std::ofstream listFile(path.u8string());
		for (const auto& item : content)
			listFile << item.ToUTF8() << '\n';
	}

	std::vector<wxString> readFile(std::filesystem::path const& path)
	{
		std::ifstream listFile(path.u8string());

		std::string tmp;

		std::vector<wxString> result;
		while (std::getline(listFile, tmp))
			result.emplace_back(wxString::FromUTF8(tmp));

		return result;
	}
}

using namespace mm;

Era2Platform::Era2Platform(const Application& app)
	: _rootDir(app.appConfig().getDataPath())
	, _app(app)
{
	_localConfig   = std::make_unique<Era2Config>(_rootDir);
	_presetManager = std::make_unique<Era2PresetManager>(_localConfig->getPresetsPath());
	_launchHelper  = std::make_unique<Era2LaunchHelper>(*_localConfig, app.iconStorage());
	_modDataProvider =
		std::make_unique<Era2ModDataProvider>(getModsDirPath(), _app.appConfig().currentLanguageCode());

	_modManager = std::make_unique<Era2ModManager>(load());

	_connections += _modManager->onListChanged().connect([this] { save(); });
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

IModManager* Era2Platform::getModManager() const
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

ModList Era2Platform::load() const
{
	ModList items;

	auto activeMods = readFile(getActiveListPath());

	for (const auto& item : boost::adaptors::reverse(activeMods))
	{
		items.active.emplace_back(item);
		items.available.emplace(item);
	}

	for (const auto& item : readFile(getHiddenListPath()))
		items.hidden.emplace(item);

	for (const auto& item : getAllDirs(getModsDirPath()))
	{
		if (const auto it = std::find(activeMods.begin(), activeMods.end(), item); it == activeMods.end())
			items.available.emplace(item);
	}

	return items;
}

void Era2Platform::save()
{
	std::vector<wxString> activeMods;
	for (const auto& item : boost::adaptors::reverse(_modManager->mods().active))
		activeMods.emplace_back(item);

	rewriteFile(getActiveListPath(), activeMods);
	rewriteFile(getHiddenListPath(), _modManager->mods().hidden);
}

std::filesystem::path Era2Platform::getActiveListPath() const
{
	return getModsDirPath() / "list.txt";
}

std::filesystem::path Era2Platform::getHiddenListPath() const
{
	return getModsDirPath() / "hidden.txt";
}
