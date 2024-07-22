// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "type/filesystem.hpp"

#include <set>
#include <string>
#include <unordered_map>

namespace mm
{
	struct ModMetadata
	{
		std::string name;
		fs::path    description;

		std::string icon;
		std::string author;
		std::string homepage;
		std::string category;
		std::string version;

		std::set<std::string> incompatible;
		std::set<std::string> requires_;
		std::set<std::string> load_after;
	};
}
