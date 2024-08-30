// SD Mod Manager

// Copyright (c) 2020-2024 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "type/embedded_icon.h"

#include <string>

class wxIcon;
class wxSize;

namespace mm
{
	struct IIconStorage
	{
		virtual ~IIconStorage() = default;

		[[nodiscard]] virtual wxIcon get(
			IconPredefined icon, IconPredefinedSize targetSize = IconPredefinedSize::x16) = 0;
		[[nodiscard]] virtual wxIcon get(const std::string& name)                         = 0;
	};
}
