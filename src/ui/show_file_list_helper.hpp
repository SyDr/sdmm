// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "domain/mod_list.hpp"

class wxWindow;

namespace mm
{
	struct IIconStorage;
	struct IModDataProvider;

	void showModFileList(
		wxWindow& parent, IIconStorage& iconStorage, IModDataProvider& modDataProvider, ModList list);
}
