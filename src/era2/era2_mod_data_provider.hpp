// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "domain/imod_data_provider.hpp"

#include "wx/string.h"

#include <filesystem>
#include <map>
#include <set>

namespace mm
{
	class Application;

	struct Era2ModDataProvider : IModDataProvider
	{
		Era2ModDataProvider(std::filesystem::path basePath, wxString preferredLng);

		non_owning_ptr<ModData const> modData(wxString const& id) override;

	private:
		void loadDefaults();

	private:
		std::filesystem::path const _basePath;
		wxString const _preferredLng;

		std::map<wxString, ModData> _data;

		std::map<wxString, std::set<wxString>> _defaultIncompatible;
		std::map<wxString, std::set<wxString>> _defaultRequires;
		std::map<wxString, std::set<wxString>> _defaultLoadAfter;
	};
}
