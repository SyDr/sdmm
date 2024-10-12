// SD Mod Manager

// Copyright (c) 2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "icon_helper.hpp"
#include "interface/iicon_storage.hpp"
#include "type/embedded_icon.h"

wxBitmap mm::loadModIcon(IIconStorage& storage, const fs::path& parent, const std::string& filename)
{
	if (!filename.empty())
		return storage.get((parent / filename).lexically_normal().string());

	return storage.get(IconPredefined::circle);
}
