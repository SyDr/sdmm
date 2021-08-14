// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <filesystem>

namespace mm
{
	struct ILaunchHelper;
	struct ILocalConfig;
	struct IModDataProvider;
	struct IModManager;
	struct IPluginManager;
	struct IPresetManager;
	struct INonAutoApplicablePlatform;

	struct IModPlatform
	{
		virtual ~IModPlatform() = default;

		virtual std::filesystem::path getManagedPath() const = 0;

		virtual INonAutoApplicablePlatform* nonAutoApplicable() = 0;

		virtual ILaunchHelper*    launchHelper() const     = 0;
		virtual ILocalConfig*     localConfig() const      = 0;
		virtual IModManager*      modManager() const       = 0;
		virtual IPluginManager*   pluginManager() const    = 0;
		virtual IPresetManager*   getPresetManager() const = 0;
		virtual IModDataProvider* modDataProvider() const  = 0;
	};
}
