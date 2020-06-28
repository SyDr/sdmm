// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "domain/imod_platform.hpp"
#include "era2_config.h"

#include <wigwag/token_pool.hpp>

#include <deque>
#include <unordered_set>
#include <vector>

namespace mm
{
	class Application;
	struct Era2LaunchHelper;
	struct Era2ModManager;
	struct Era2PresetManager;
	struct Era2ModDataProvider;
	struct ModList;

	struct Era2Platform : IModPlatform
	{
		explicit Era2Platform(const Application& app);

		std::filesystem::path getManagedPath() const override;
		std::filesystem::path getModsDirPath() const;

		IPresetManager*   getPresetManager() const override;
		Era2Config*       localConfig() const override;
		IModManager*      getModManager() const override;
		ILaunchHelper*    launchHelper() const override;
		IModDataProvider* modDataProvider() const override;

	private:
		std::filesystem::path getActiveListPath() const;
		std::filesystem::path getHiddenListPath() const;

		ModList load() const;
		void save();

	private:
		const Application&          _app;
		const std::filesystem::path _rootDir;

		std::unique_ptr<Era2Config>          _localConfig;
		std::unique_ptr<Era2LaunchHelper>    _launchHelper;
		std::unique_ptr<Era2ModManager>      _modManager;
		std::unique_ptr<Era2PresetManager>   _presetManager;
		std::unique_ptr<Era2ModDataProvider> _modDataProvider;

		wigwag::token_pool _connections;
	};
}
