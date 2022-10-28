// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "domain/ipreset_manager.hpp"

#include <vector>

namespace mm
{
	struct Era2Platform;

	struct Era2PresetManager : IPresetManager
	{
		explicit Era2PresetManager(std::filesystem::path rootPath);

		std::pair<ModList, std::unordered_map<wxString, PluginState>>
			 loadPreset(const wxString& name) override;
		void savePreset(const wxString& name, const ModList& list,
						const std::unordered_map<wxString, PluginState>& plugins) override;

		std::set<wxString> list() const override;

		bool exists(const wxString& name) const override;
		void copy(const wxString& from, const wxString& to) override;
		void rename(const wxString& from, const wxString& to) override;
		void remove(const wxString& name) override;

		sigslot::signal<>& onListChanged() override;

	private:
		std::filesystem::path _rootPath;

		sigslot::signal<> _listChanged;
	};
}
