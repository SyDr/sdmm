// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "interface/ipreset_manager.hpp"

#include "type/filesystem.hpp"

namespace mm
{
	struct Era2Platform;

	struct Era2PresetManager : IPresetManager
	{
		explicit Era2PresetManager(fs::path rootPath, fs::path modsPath);

		std::pair<ModList, PluginList> loadPreset(const wxString& name) override;
		void savePreset(const wxString& name, const ModList& list, const PluginList& plugins) override;

		std::set<wxString> list() const override;

		bool exists(const wxString& name) const override;
		void copy(const wxString& from, const wxString& to) override;
		void rename(const wxString& from, const wxString& to) override;
		void remove(const wxString& name) override;

		sigslot::signal<>& onListChanged() override;

	private:
		fs::path _rootPath;
		fs::path _modsPath;

		sigslot::signal<> _listChanged;
	};
}
