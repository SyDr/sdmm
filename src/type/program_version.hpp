// SD Mod Manager

// Copyright (c) 2025-2026 Aliaksei Karalenka <sydr1991@gmail.com>.
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

		explicit ProgramVersion(std::string_view version);
		explicit ProgramVersion(size_t major, size_t minor, size_t patch);

		auto operator<=>(const ProgramVersion& r) const = default;
		bool operator==(const ProgramVersion& r) const  = default;

		static ProgramVersion current();
	};
}
