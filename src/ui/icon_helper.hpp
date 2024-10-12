// SD Mod Manager

// Copyright (c) 2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "type/filesystem.hpp"

class wxBitmap;

namespace mm
{
	struct IIconStorage;

	wxBitmap loadModIcon(IIconStorage& storage, const fs::path& parent, const std::string& filename);
}