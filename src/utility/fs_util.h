// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <string>
#include <vector>

#include <wx/string.h>

#include "type/filesystem.hpp"

namespace mm
{
	void overwriteFileContent(const fs::path& path, const wxString& content);

	bool createDir(const std::filesystem::path& path);
	bool copyDir(const wxString& path, const wxString& newPath);

	std::vector<wxString> getAllDirs(const std::filesystem::path& path);
	std::vector<wxString> getAllFiles(const std::filesystem::path& path);

	std::vector<wxString> readFile(const fs::path& path);

	template <typename Container>
	void overwriteFileFromContainer(std::filesystem::path const& path, Container const& content)
	{
		std::stringstream stream;

		for (const auto& item : content)
			stream << item.ToUTF8() << '\n';

		overwriteFileContent(path, wxString::FromUTF8(stream.str()));
	}
}
