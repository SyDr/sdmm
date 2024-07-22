// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "interface/imod_data_provider.hpp"

#include "type/filesystem.hpp"
#include "wx/string.h"

#include <map>
#include <set>

namespace mm
{
	struct Application;
	struct II18nService;

	struct Era2ModDataProvider : IModDataProvider
	{
		Era2ModDataProvider(fs::path basePath, std::string preferredLng, const II18nService& i18Service);

		const ModData& modData(const std::string& id) override;
		void           clear();

	private:
		void loadDefaults();

	private:
		const fs::path      _basePath;
		const std::string   _preferredLng;
		const II18nService& _i18Service;

		std::map<std::string, ModData> _data;

		std::map<std::string, std::set<std::string>> _defaultIncompatible;
		std::map<std::string, std::set<std::string>> _defaultRequires;
		std::map<std::string, std::set<std::string>> _defaultLoadAfter;
	};
}
