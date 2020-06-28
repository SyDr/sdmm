// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <string>
#include <vector>

#include <wx/string.h>

namespace mm
{
	bool createDir(const std::filesystem::path& path);
	bool copyDir(const wxString& path, const wxString& newPath);

	std::vector<std::string> getAllDirs(const std::filesystem::path& path);
}
