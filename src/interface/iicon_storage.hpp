// SD Mod Manager

// Copyright (c) 2020-2024 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "type/embedded_icon.h"

#include <string>

class wxBitmap;
class wxSize;

namespace mm
{
	struct IIconStorage
	{
		virtual ~IIconStorage() = default;

		[[nodiscard]] virtual wxBitmap get(
			IconPredefined icon, std::optional<IconPredefinedSize> targetSize = {}) = 0;
		[[nodiscard]] virtual wxBitmap get(
			const std::string& name, std::optional<IconPredefinedSize> resizeTo = {}) = 0;
	};
}
