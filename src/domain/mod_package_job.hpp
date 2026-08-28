// SD Mod Manager

// Copyright (c) 2026 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "type/filesystem.hpp"

#include <cstdint>

namespace mm
{
	struct ModPackageJob
	{
		std::uint32_t process_id = 0;
		fs::path      job_directory;
		fs::path      output_path;
	};
}
