// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <filesystem>

namespace mm
{
	struct ILaunchHelper;
	class ILocalConfig;
	struct IModManager;
	struct IModDataProvider;
	struct IPresetManager;

	struct IModPlatform
	{
		virtual ~IModPlatform() = default;

		virtual std::filesystem::path getManagedPath() const = 0;

		virtual ILaunchHelper*    launchHelper() const     = 0;
		virtual ILocalConfig*     localConfig() const   = 0;
		virtual IModManager*      getModManager() const    = 0;
		virtual IPresetManager*   getPresetManager() const = 0;
		virtual IModDataProvider* modDataProvider() const  = 0;
	};
}
