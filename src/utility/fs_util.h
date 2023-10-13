// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <string>
#include <vector>

#include <wx/string.h>

#include "type/filesystem.hpp"

namespace mm
{
	std::string readFile(const fs::path& path);

	void overwriteFile(const fs::path& path, const std::string& content);
	void overwriteFileIfNeeded(const fs::path& path, const std::string& content);

	std::vector<wxString> getAllDirs(const fs::path& path);
	std::vector<wxString> getAllFiles(const fs::path& path);

	template <typename Container>
	void overwriteFileFromContainer(fs::path const& path, Container const& content)
	{
		std::stringstream stream;

		for (const auto& item : content)
			stream << item << '\n';

		overwriteFile(path, stream.str());
	}
}
