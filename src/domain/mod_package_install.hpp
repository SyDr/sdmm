// SD Mod Manager

// Copyright (c) 2026 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "domain/mod_data.hpp"
#include "type/filesystem.hpp"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

namespace mm
{
	enum class ModPackageDiagnosticCode
	{
		unsupported_type,
		missing_file,
		cannot_read,
		corrupt_archive,
		encrypted_content,
		unsafe_path,
		outside_mods,
		direct_mods_file,
		link_entry,
		invalid_windows_name,
		no_mods,
		duplicate_target,
		target_collision,
		extraction_failed,
		invalid_extracted_content,
	};

	struct ModPackageDiagnostic
	{
		fs::path                 package_path;
		ModPackageDiagnosticCode code = ModPackageDiagnosticCode::cannot_read;
		std::string              detail;
	};

	struct PackagedMod
	{
		fs::path package_path;
		fs::path extracted_directory;

		std::string directory;
		ModData     metadata;
		std::string description;

		bool        installed = false;
		fs::path    installed_directory;
		std::string installed_version;
		bool        selectable = true;
		bool        selected   = true;
	};

	struct ModPackageInspection
	{
		fs::path                          job_directory;
		std::vector<PackagedMod>          mods;
		std::vector<ModPackageDiagnostic> diagnostics;
	};

	enum class ModPackageProgressPhase
	{
		scanning,
		extracting,
		validating,
		staging,
		committing,
		rolling_back,
	};

	struct ModPackageProgress
	{
		ModPackageProgressPhase phase   = ModPackageProgressPhase::scanning;
		std::size_t             current = 0;
		std::size_t             total   = 0;
		std::string             item;
		bool                    cancellable = true;
	};

	struct ModPackageInstallResult
	{
		std::vector<std::string> installed_mod_ids;
	};

	class ModPackageOperationCancelled : public std::runtime_error
	{
	public:
		ModPackageOperationCancelled()
			: std::runtime_error("Mod package operation was cancelled")
		{}
	};
}
