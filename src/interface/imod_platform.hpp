// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "type/filesystem.hpp"

namespace mm
{
	struct ILaunchHelper;
	struct ILocalConfig;
	struct IModDataProvider;
	struct IModManager;
	struct IPluginManager;
	struct IPresetManager;

	struct ModList;
	struct PluginList;

	struct IModPlatform
	{
		virtual ~IModPlatform() = default;

		[[nodiscard]] virtual fs::path managedPath() const = 0;
		virtual void                   reload()            = 0;

		virtual void apply(ModList* mods, PluginList* plugins) = 0;

		[[nodiscard]] virtual ILaunchHelper*    launchHelper() const     = 0;
		[[nodiscard]] virtual ILocalConfig*     localConfig() const      = 0;
		[[nodiscard]] virtual IModManager*      modManager() const       = 0;
		[[nodiscard]] virtual IPluginManager*   pluginManager() const    = 0;
		[[nodiscard]] virtual IPresetManager*   getPresetManager() const = 0;
		[[nodiscard]] virtual IModDataProvider* modDataProvider() const  = 0;
	};
}
