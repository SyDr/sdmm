// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "domain/mod_list.hpp"
#include "domain/plugin_list.hpp"
#include "era2_config.h"
#include "interface/imod_platform.hpp"
#include "type/filesystem.hpp"

#include <deque>
#include <unordered_set>
#include <vector>

namespace mm
{
	struct Application;
	struct Era2LaunchHelper;
	struct Era2ModManager;
	struct Era2PresetManager;
	struct Era2ModDataProvider;
	struct Era2PluginManager;
	struct ModList;

	struct Era2Platform : IModPlatform
	{
		explicit Era2Platform(Application const& app);

		fs::path managedPath() const override;
		void     reload() override;

		void apply(ModList* mods, PluginList* plugins) override;

		IPresetManager*   getPresetManager() const override;
		ILocalConfig*     localConfig() const override;
		IModManager*      modManager() const override;
		ILaunchHelper*    launchHelper() const override;
		IModDataProvider* modDataProvider() const override;
		IPluginManager*   pluginManager() const override;

	private:
		fs::path getModsDirPath() const;
		fs::path getActiveListPath() const;
		fs::path getHiddenListPath() const;
		fs::path getPluginListPath() const;

		void save();

	private:
		const Application& _app;
		const fs::path     _rootDir;

		std::unique_ptr<Era2Config>          _localConfig;
		std::unique_ptr<Era2LaunchHelper>    _launchHelper;
		std::unique_ptr<Era2ModDataProvider> _modDataProvider;
		std::unique_ptr<Era2ModManager>      _modManager;
		std::unique_ptr<Era2PluginManager>   _pluginManager;
		std::unique_ptr<Era2PresetManager>   _presetManager;

		ModList _modList;

		sigslot::scoped_connection _modListChanged;
		sigslot::scoped_connection _pluginListChanged;
	};
}
