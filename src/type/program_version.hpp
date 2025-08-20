// SD Mod Manager

// Copyright (c) 2025 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <string_view>

namespace mm
{
	struct ProgramVersion
	{
		size_t major = 0;
		size_t minor = 0;
		size_t patch = 0;

		ProgramVersion(std::string_view version);
		ProgramVersion(size_t major, size_t minor, size_t patch);

		auto operator<=>(const ProgramVersion& r) const = default;
		bool operator==(const ProgramVersion& r) const  = default;
	};
}
