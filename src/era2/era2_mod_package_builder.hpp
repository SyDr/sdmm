// SD Mod Manager

// Copyright (c) 2026 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "interface/imod_package_builder.hpp"

#include <thread>
#include <vector>

namespace mm
{
	struct Era2ModPackageBuilder : IModPackageBuilder
	{
		Era2ModPackageBuilder(fs::path gameRoot, fs::path programPath);
		~Era2ModPackageBuilder() override = default;

		[[nodiscard]] ModPackageJob start(const ModData& mod, const fs::path& outputPath) override;

	private:
		void cleanStaleJobs() const;

	private:
		fs::path _gameRoot;
		fs::path _programPath;
		fs::path _jobsRoot;

		std::vector<std::jthread> _monitors;
	};
}
