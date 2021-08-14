// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "era2_plugin_helper.hpp"

#include "application.h"
#include "domain/mod_list.hpp"
#include "domain/plugin_list.hpp"
#include "system_info.hpp"
#include "utility/fs_util.h"
#include "utility/json_util.h"

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <filesystem>

using namespace mm;

namespace
{
	bool isPlugin(fspath path)
	{
		if (path.extension().string() == constant::pluginOffExtension)
			path = path.filename().replace_extension();

		return path.has_extension() && constant::pluginExts.count(path.extension().string());
	}
}

void era2_plugin_helper::updateAvailability(PluginList&                            current,
											const Era2PLuginListPhysicalStructure& structure,
											const ModList&                         mods)
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

				if (id.ends_with(wxString(constant::pluginOffExtension)))
				{
					enabled = false;
					id = id.RemoveLast(std::char_traits<const char>::length(constant::pluginOffExtension));
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

mm::PluginList era2_plugin_helper::updateAvailability(const Era2PLuginListPhysicalStructure& structure,
													  const ModList&                         mods)
{
	PluginList result;
	updateAvailability(result, structure, mods);

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
		for (const auto& dir : constant::pluginDirs)
		{
			const auto subPath = it->path() / constant::pluginSubdir / dir;
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

void mm::era2_plugin_helper::loadManagedState(PluginList& target, const fspath& pluginPath)
{
	if (std::ifstream datafile(pluginPath.string()); datafile)
	{
		try
		{
			auto data = nlohmann::json::parse(datafile);
			for (auto& [key, value] : data.items())
			{
				target.overrideState(key, value == "enabled" ? PluginState::enabled : PluginState::disabled);
			}
		}
		catch (...)
		{
			wxLogDebug("Can't parse data file");
		}
	}
}

void era2_plugin_helper::saveManagedState(const fspath& pluginPath, const fspath& modsPath,
										  const PluginList& list)
{
	nlohmann::json data = nlohmann::json::object();
	for (const auto& item : list.overridden)
		data[item.first.ToStdString()] = (item.second == PluginState::enabled ? "enabled" : "disabled");

	overwriteFileContent(pluginPath, data.dump(2));

	auto targetPath = modsPath / constant::mm_managed_mod / constant::pluginSubdir;
	for (const auto& dir : constant::pluginDirs)
	{
		const auto subPath = targetPath / dir;
		std::filesystem::remove_all(subPath);
		std::filesystem::create_directories(subPath);
	}

	for (auto const& [id, state] : list.overridden)
	{
		auto it = list.state.find(id);
		if (it == list.state.end())
			continue;

		const auto mod = it->second.mod;
		const auto copyFrom =
			modsPath / mod.ToStdString() / constant::pluginSubdir /
			(id + (it->second.state == PluginState::disabled ? constant::pluginOffExtension : ""))
				.ToStdString();

		if (state != PluginState::disabled)
		{
			std::filesystem::copy_file(copyFrom, targetPath / id.ToStdString(),
									   std::filesystem::copy_options::overwrite_existing);
		}
		else
		{
			std::ofstream(targetPath / id.ToStdString());
		}
	}
}
