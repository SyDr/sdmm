// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "interface/imod_data_provider.hpp"

#include "type/filesystem.hpp"
#include "wx/string.h"

#include <map>
#include <set>

namespace mm
{
	struct Application;

	struct Era2ModDataProvider : IModDataProvider
	{
		Era2ModDataProvider(fs::path basePath, wxString preferredLng);

		const ModData& modData(const wxString& id) override;

	private:
		void loadDefaults();

	private:
		const fs::path _basePath;
		const wxString _preferredLng;

		std::map<wxString, ModData> _data;

		std::map<wxString, std::set<wxString>> _defaultIncompatible;
		std::map<wxString, std::set<wxString>> _defaultRequires;
		std::map<wxString, std::set<wxString>> _defaultLoadAfter;
	};
}
