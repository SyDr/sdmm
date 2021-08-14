// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "domain/mod_data.hpp"

#include <wx/string.h>

#include <filesystem>

namespace mm::era2_mod_loader
{
	ModData updateAvailability(std::filesystem::path const& loadFrom, wxString const& preferredLng,
				 std::set<wxString> const& defaultIncompatible, std::set<wxString> const& defaultRequires,
				 std::set<wxString> const& defaultLoadAfter);
}
