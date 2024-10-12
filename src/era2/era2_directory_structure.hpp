// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "type/filesystem.hpp"

#include <map>
#include <string>
#include <vector>

namespace mm
{
	struct Era2FileEntry
	{
		fs::path                 filePath;  // virtual file path
		std::string              gamePath;  // path to file in game, if available
		std::vector<std::string> modPaths;
	};

	struct Era2DirectoryStructure
	{
		std::vector<std::string>   mods;
		std::vector<Era2FileEntry> entries;
	};
}
