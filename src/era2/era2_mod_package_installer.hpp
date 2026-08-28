// SD Mod Manager

// Copyright (c) 2026 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "interface/imod_package_installer.hpp"

#include <functional>

namespace mm
{
	struct II18nService;
	struct IModManager;

	struct Era2ModPackageInstaller : IModPackageInstaller
	{
		Era2ModPackageInstaller(fs::path gameRoot, fs::path programPath, std::string preferredLanguage,
			const II18nService& i18nService, IModManager& modManager, std::function<void()> reload);

		[[nodiscard]] ModPackageInspection inspect(
			const std::vector<fs::path>& packages, const ModPackageProgressCallback& progress) override;

		[[nodiscard]] ModPackageInstallResult install(const ModPackageInspection& inspection,
			const std::vector<std::size_t>&                                       selectedMods,
			const ModPackageProgressCallback&                                     progress) override;

		void discard(const ModPackageInspection& inspection) noexcept override;

	private:
		void recoverInterruptedJobs();

	private:
		fs::path _gameRoot;
		fs::path _programPath;
		fs::path _modsRoot;
		fs::path _jobsRoot;
		fs::path _sevenZip;

		std::string           _preferredLanguage;
		const II18nService&   _i18nService;
		IModManager&          _modManager;
		std::function<void()> _reload;
	};
}
