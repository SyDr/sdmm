// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "domain/imod_platform.hpp"

#include <memory>

namespace mm
{
	struct BaseModPlatform : IModPlatform
	{
		std::filesystem::path getManagedPath() const override;

		ILaunchHelper* launchHelper() const override;
		IPresetManager* getPresetManager() const override;
		IModDataProvider* modDataProvider() const override;
	};
}
