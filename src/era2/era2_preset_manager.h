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

		ModList loadPreset(wxString const& name) override;
		void    savePreset(wxString const& name, ModList const& list) override;

		std::set<wxString> list() const override;

		bool exists(wxString const& name) const override;
		void copy(wxString const& from, wxString const& to) override;
		void rename(wxString const& from, wxString const& to) override;
		void remove(wxString const& name) override;

		wigwag::signal_connector<void()> onListChanged() const override;

	private:
		std::filesystem::path _rootPath;

		wigwag::signal<void()> _listChanged;
	};
}
