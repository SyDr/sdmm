// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "era2_plugin_helper.hpp"

#include "application.h"
#include "domain/mod_list.hpp"
#include "domain/plugin_list.hpp"
#include "system_info.hpp"
#include "utility/json_util.h"

#include <fmt/format.h>

#include <filesystem>

using namespace mm;

namespace
{
	constexpr auto eraPluginsDir = "EraPlugins";
	constexpr auto offExtension  = ".off";

	constexpr std::array     pluginDirs = { ".", "BeforeWog", "AfterWog" };
	const std::set<std::string> pluginExts = { ".dll", ".bin", ".era" };

	bool isPlugin(fspath path)
	{
		if (path.extension().string() == offExtension)
			path = path.filename().replace_extension();

		return path.has_extension() && pluginExts.count(path.extension().string());
	}
}

void era2_plugin_helper::load(PluginList& current, const Era2PLuginListPhysicalStructure& structure,
							  const ModList& mods)
{
	current.available.clear();
	current.state.clear();

	for (const auto& modId : mods.active)
	{
		auto it = structure.data.find(modId);
		if (it == structure.data.end())
			continue;

		for (const auto& item : it->second)
		{
			auto [id, enabled] = [&]
			{
				wxString id      = item;
				bool     enabled = true;

				if (id.ends_with(wxString(offExtension)))
				{
					enabled = false;
					id      = id.RemoveLast(std::char_traits<const char>::length(offExtension));
				}

				return std::make_pair(id, enabled);
			}();

			current.available.emplace(id);
			current.state.try_emplace(id, modId, enabled ? PluginState::enabled : PluginState::disabled);
		}
	}

	for (const auto& item : current.overridden)
		current.available.emplace(item.first);
}

mm::PluginList era2_plugin_helper::load(const Era2PLuginListPhysicalStructure& structure, const ModList& mods)
{
	PluginList result;
	load(result, structure, mods);

	return result;
}

Era2PLuginListPhysicalStructure era2_plugin_helper::loadPhysicalStructure(const fspath& base)
{
	Era2PLuginListPhysicalStructure result;

	using di = std::filesystem::directory_iterator;
	for (auto it = di(base), end = di(); it != end; ++it)
	{
		if (!it->is_directory())
			continue;

		const auto modId = it->path().filename();
		if (modId == mm::constant::mm_managed_mod)
			continue;

		auto& place = result.data[wxString::FromUTF8(modId.string())];
		for (const auto& dir : pluginDirs)
		{
			const auto subPath = it->path() / eraPluginsDir / dir;
			if (!std::filesystem::is_directory(subPath))
				continue;

			for (auto subIt = di(subPath), subEnd = di(); subIt != end; ++subIt)
			{
				auto pluginPath = subIt->path();

				if (!std::filesystem::is_regular_file(pluginPath))
					continue;

				if (!isPlugin(pluginPath))
					continue;

				place.emplace(fmt::format("{}/{}", dir, pluginPath.filename().string()));
			}
		}
	}

	return result;
}
