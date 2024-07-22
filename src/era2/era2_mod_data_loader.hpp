// SD Mod Manager

// Copyright (c) 2020-2024 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "domain/mod_data.hpp"

#include <set>
#include <string>

namespace mm
{
	struct II18nService;
}

namespace mm::Era2ModDataLoader
{

	ModData load(const fs::path& loadFrom, const std::string& preferredLng,
		const std::set<std::string>& defaultIncompatible, const std::set<std::string>& defaultRequires,
		const std::set<std::string>& defaultLoadAfter, const II18nService& i18Service);
}
