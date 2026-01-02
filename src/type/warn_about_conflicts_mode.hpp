// SD Mod Manager

// Copyright (c) 2026 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <magic_enum.hpp>

namespace mm
{
	enum class WarnAboutConflictsMode
	{
		only_inform          = 0,
		warn_before_enabling = 1,
		do_nothing           = 2,
	};

	inline constexpr auto WarnAboutConflictsModeValues = magic_enum::enum_values<WarnAboutConflictsMode>();
}
