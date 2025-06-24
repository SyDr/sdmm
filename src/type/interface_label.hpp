// SD Mod Manager

// Copyright (c) 2025 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <array>

#include <magic_enum.hpp>

#include "icon.hpp"

namespace mm
{
	enum class InterfaceLabel
	{
		show        = 0,
		dont_show   = 1,
	};

	inline constexpr const auto InterfaceLabelValues = magic_enum::enum_values<InterfaceLabel>();
}
