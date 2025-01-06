// SD Mod Manager

// Copyright (c) 2025 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <chrono>

namespace mm
{
	using clock      = std::chrono::system_clock;
	using time_point = std::chrono::time_point<clock>;

	inline constexpr const auto TimeOutputFormat = "{:%Y-%m-%dT%H:%M:%S}";
	inline constexpr const auto TimeInputFormat  = "%Y-%m-%dT%H:%M:%S";
}
