// SD Mod Manager

// Copyright (c) 2020-2024 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "type/icon.hpp"

#include <string>

class wxBitmap;
class wxSize;

namespace mm
{
	struct IIconStorage
	{
		virtual ~IIconStorage() = default;

		[[nodiscard]] virtual wxBitmap get(
			Icon::Stock icon, std::optional<Icon::Size> targetSize = {}) = 0;
		[[nodiscard]] virtual wxBitmap get(
			const std::string& name, std::optional<Icon::Size> resizeTo = {}) = 0;
	};
}
