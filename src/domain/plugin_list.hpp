// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <wx/string.h>

#include <optional>
#include <set>
#include <compare>
#include <unordered_map>
#include <vector>

namespace mm
{
	enum class PluginState
	{
		disabled,
		enabled,
	};

	inline PluginState switchPluginState(PluginState state)
	{
		return state == PluginState::enabled ? PluginState::disabled : PluginState::enabled;
	}

	struct ModPluginState
	{
		wxString    mod;
		PluginState state = PluginState::disabled;

		ModPluginState(wxString mod, PluginState state)
			: mod(std::move(mod))
			, state(state) {};

		std::weak_ordering operator<=>(const ModPluginState& other) const = default;
	};

	struct PluginList
	{
		std::set<wxString> available;

		std::unordered_map<wxString, ModPluginState> state;
		std::unordered_map<wxString, PluginState>    overridden;

		void overrideState(const wxString& item, PluginState newState);
		void reset(const wxString& item);

		std::optional<ModPluginState> defaultState(const wxString& item) const;
		std::optional<PluginState>    overriddenState(const wxString& item) const;

		std::weak_ordering operator<=>(const PluginList& other) const = default;
	};
}
