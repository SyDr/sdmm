// SD Mod Manager

// Copyright (c) 2020-2024 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "domain/mod_list.hpp"

namespace mm
{
	struct IModDataProvider;

	std::vector<std::string> ResolveModConflicts(const ModList& mods, mm::IModDataProvider& modDataProvider,
		const std::string& enablingMod, const std::string& disablingMod);
}
