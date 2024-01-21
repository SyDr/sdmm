// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <set>

#include <sigslot/signal.hpp>

#include "domain/preset_data.hpp"

namespace mm
{
	struct ModList;
	struct PluginList;

	struct IPresetManager
	{
		virtual ~IPresetManager() = default;

		[[nodiscard]] virtual std::set<std::string> list() const = 0;

		[[nodiscard]] virtual PresetData loadPreset(const std::string& name)    = 0;
		[[nodiscard]] virtual PresetData loadPreset(const nlohmann::json& data) = 0;

		[[nodiscard]] virtual nlohmann::json savePreset(const PresetData& preset)  = 0;
		virtual void savePreset(const std::string& name, const PresetData& preset) = 0;

		[[nodiscard]] virtual bool exists(const std::string& name) const                  = 0;
		virtual void               copy(const std::string& from, const std::string& to)   = 0;
		virtual void               rename(const std::string& from, const std::string& to) = 0;
		virtual void               remove(const std::string& name)                        = 0;

		[[nodiscard]] virtual sigslot::signal<>& onListChanged() = 0;
	};
}
