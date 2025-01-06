// SD Mod Manager

// Copyright (c) 2025 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <array>

namespace mm
{
	enum class UpdateCheckMode
	{
		manual          = 0,
		on_every_launch = 1,
		once_per_day    = 2,
		once_per_week   = 3,
		once_per_month  = 4,
	};

	inline constexpr const std::array UpdateCheckModeValues = { UpdateCheckMode::manual,
		UpdateCheckMode::on_every_launch, UpdateCheckMode::once_per_day, UpdateCheckMode::once_per_week,
		UpdateCheckMode::once_per_month };

	inline std::string to_string(UpdateCheckMode value)
	{
		switch (value)
		{
			using enum UpdateCheckMode;
		case manual: return "manual";
		case on_every_launch: return "on_every_launch";
		case once_per_day: return "once_per_day";
		case once_per_week: return "once_per_week";
		case once_per_month: return "once_per_month";
		}

		return "";
	}
}
