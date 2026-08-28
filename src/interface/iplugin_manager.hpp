// SD Mod Manager

// Copyright (c) 2026 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "domain/mod_plugin.hpp"

#include <vector>

namespace mm
{
	struct ModData;

	struct IPluginManager
	{
		virtual ~IPluginManager() = default;

		[[nodiscard]] virtual std::vector<ModPlugin> list(const ModData& mod) const                      = 0;
		virtual void                                 toggle(const ModData& mod, const ModPlugin& plugin) = 0;
	};
}
