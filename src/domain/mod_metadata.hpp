// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <wx/string.h>

#include <filesystem>
#include <set>
#include <unordered_map>

namespace mm
{
	struct ModMetadata
	{
		wxString caption;

		wxString              short_description;
		std::filesystem::path full_description;

		std::string icon_filename;
		size_t      icon_index = 0;

		wxString authors;
		wxString homepage_link;
		// wxString video_link;

		wxString category;

		wxString mod_platform;  // era?
		wxString mod_version;
		wxString info_version;

		struct Package
		{
			wxString filename;
			wxString caption;
			wxString description;
		};

		std::unordered_map<std::string, Package> plugins;

		std::set<wxString> incompatible;
		std::set<wxString> requires_;
		std::set<wxString> load_after;
	};
}
