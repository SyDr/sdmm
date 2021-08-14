// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <wx/string.h>

namespace mm
{
	wxString    utf8_to_wxString(const std::string& input);
	wxString    utf8_to_wxString(const wxString& input);
	std::string wxString_to_utf8(const wxString& input);
}
