// SD Mod Manager

// Copyright (c) 2026 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "interface/iplugin_manager.hpp"

namespace mm
{
	struct Era2PluginManager : IPluginManager
	{
		explicit Era2PluginManager(std::string noDescriptionText);

		[[nodiscard]] std::vector<ModPlugin> list(const ModData& mod) const override;
		void                                 toggle(const ModData& mod, const ModPlugin& plugin) override;

	private:
		std::string _noDescriptionText;
	};
}
