// SD Mod Manager

// Copyright (c) 2026 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "type/filesystem.hpp"

#include <stdexcept>
#include <string>
#include <utility>

namespace mm
{
	enum class PluginLocation
	{
		era_plugins,
		before_wog,
		after_wog,
	};

	struct PluginMetadata
	{
		std::string caption;
		std::string description;
	};

	struct ModPlugin
	{
		PluginLocation location = PluginLocation::era_plugins;

		std::string canonical_filename;
		std::string caption;
		std::string description;

		fs::path current_path;
		fs::path counterpart_path;

		bool enabled   = false;
		bool collision = false;
	};

	enum class PluginToggleErrorCode
	{
		collision,
		missing,
		filesystem,
	};

	class PluginToggleError : public std::runtime_error
	{
	public:
		PluginToggleError(PluginToggleErrorCode code, std::string message)
			: std::runtime_error(std::move(message))
			, _code(code)
		{}

		[[nodiscard]] PluginToggleErrorCode code() const noexcept
		{
			return _code;
		}

	private:
		PluginToggleErrorCode _code;
	};
}
