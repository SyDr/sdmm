// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "type/filesystem.hpp"

#include <map>
#include <string>
#include <vector>

#include <wx/string.h>

namespace mm
{
	struct Era2DirectoryEntry
	{
		bool raw = false;
		bool lod = false;

		Era2DirectoryEntry() = default;
		Era2DirectoryEntry(bool raw, bool lod)
			: raw(raw)
			, lod(lod) {};

		bool any() const
		{
			return raw || lod;
		}
	};

	struct Era2DirectoryStructure
	{
		std::vector<fs::path>    fileList;
		std::vector<std::string> modList;

		std::vector<std::vector<Era2DirectoryEntry>> entries;  // [path index][mod index] = entry
	};
}
