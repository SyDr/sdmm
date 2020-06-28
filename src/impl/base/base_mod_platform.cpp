// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "base_mod_platform.h"

using namespace mm;

std::filesystem::path BaseModPlatform::getManagedPath() const
{
	return {};
}

IPresetManager* BaseModPlatform::getPresetManager() const
{
	return nullptr;
}

IModDataProvider* BaseModPlatform::modDataProvider() const
{
	return nullptr;
}

ILaunchHelper* BaseModPlatform::launchHelper() const
{
	return nullptr;
}
