// SD Mod Manager

// Copyright (c) 2025 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <array>

#include <magic_enum.hpp>

namespace mm
{
	enum class ModDescriptionUsedControl
	{
		try_to_use_webview2    = 0,
		use_wxhtml_control     = 1,
		use_plain_text_control = 2,
	};

	inline constexpr const auto ModDescriptionUsedControlValues =
		magic_enum::enum_values<ModDescriptionUsedControl>();
}
