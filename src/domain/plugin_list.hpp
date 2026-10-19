// SD Mod Manager

// Copyright (c) 2026 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <compare>
#include <set>
#include <string>

#include "type/filesystem.hpp"

namespace mm
{
	struct PluginId
	{
		std::string v;  /// folded, doesn't include fixed paths (i.e. EraPlugins/AfterWoG/Test.bin.off ->
						/// afterwog/test.bin)

		bool                 operator==(const PluginId&) const        = default;
		std::strong_ordering operator<=>(const PluginId& other) const = default;
	};

	struct PluginSource
	{
		fs::path v;  /// relative path

		bool enabled() const
		{
			return v.extension() != ".off";
		}

		bool                 operator==(const PluginSource&) const        = default;
		std::strong_ordering operator<=>(const PluginSource& other) const = default;
	};

	struct Plugin
	{
		PluginId     id;
		PluginSource source;

		std::string modId;

		Plugin(std::string modId, fs::path from);

		bool                 operator==(const Plugin&) const        = default;
		std::strong_ordering operator<=>(const Plugin& other) const = default;
	};

	using AvailablePlugins = std::map<PluginId, std::map<std::string, PluginSource>>;

	enum class PluginState
	{
		forced_enable,
		forced_disable,
	};
	using ManagedPlugins = std::map<PluginId, PluginState>;

	struct PluginList
	{
		AvailablePlugins available;
		ManagedPlugins   managed;
	};
}
