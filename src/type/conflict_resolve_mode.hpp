// SD Mod Manager

// Copyright (c) 2020-2025 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <magic_enum.hpp>

namespace mm
{
	enum class ConflictResolveMode
	{
		automatic = 0,
		manual    = 1,
	};

	inline constexpr auto ConflictResolveModeValues = magic_enum::enum_values<ConflictResolveMode>();
}
