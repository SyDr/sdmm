// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "mod_metadata.hpp"

#include <string>

namespace mm
{
	struct ModData : ModMetadata
	{
		wxString id;
		bool     virtual_mod = false;  // does this mod physically exist?
		fs::path data_path;
	};
}
