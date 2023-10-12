// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "type/filesystem.hpp"

#include <set>
#include <unordered_map>
#include <string>

namespace mm
{
	struct ModMetadata
	{
		std::string caption;

		std::string short_description;
		fs::path full_description;

		std::string icon_filename;
		size_t      icon_index = 0;

		std::string authors;
		std::string homepage_link;
		// std::string video_link;

		std::string category;

		std::string mod_platform;  // era?
		std::string mod_version;
		std::string info_version;

		struct Package
		{
			std::string filename;
			std::string caption;
			std::string description;
		};

		std::unordered_map<std::string, Package> plugins;

		std::set<std::string> incompatible;
		std::set<std::string> requires_;
		std::set<std::string> load_after;
	};
}
