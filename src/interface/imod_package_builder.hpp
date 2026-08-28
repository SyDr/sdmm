// SD Mod Manager

// Copyright (c) 2026 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "domain/mod_package_job.hpp"

namespace mm
{
	struct ModData;

	struct IModPackageBuilder
	{
		virtual ~IModPackageBuilder() = default;

		[[nodiscard]] virtual ModPackageJob start(const ModData& mod, const fs::path& outputPath) = 0;
	};
}
