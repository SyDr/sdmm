// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "type/filesystem.hpp"

#include <string>

class wxIcon;

namespace mm
{
	struct IIconStorage
	{
		virtual ~IIconStorage() = default;

		virtual wxIcon get(const std::string& name) = 0;
	};
}
