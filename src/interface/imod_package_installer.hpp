// SD Mod Manager

// Copyright (c) 2026 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "domain/mod_package_install.hpp"

#include <functional>

namespace mm
{
	using ModPackageProgressCallback = std::function<bool(const ModPackageProgress&)>;

	struct IModPackageInstaller
	{
		virtual ~IModPackageInstaller() = default;

		[[nodiscard]] virtual ModPackageInspection inspect(
			const std::vector<fs::path>& packages, const ModPackageProgressCallback& progress) = 0;

		[[nodiscard]] virtual ModPackageInstallResult install(const ModPackageInspection& inspection,
			const std::vector<std::size_t>& selectedMods, const ModPackageProgressCallback& progress) = 0;

		virtual void discard(const ModPackageInspection& inspection) noexcept = 0;
	};
}
