// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "string_util.h"

#include <wx/string.h>

wxString mm::utf8_to_wxString(const std::string& input)
{
	wxString result;

	if (!input.empty())
		result = wxString::FromUTF8(input.c_str());

	if (result.empty())
		result = input;

	return result;
}

wxString mm::utf8_to_wxString(const wxString& input)
{
	wxString result;

	if (!input.empty())
		result = wxString::FromUTF8(input.c_str());

	if (result.empty())
		result = input;

	return result;
}

std::string mm::wxString_to_utf8(const wxString& input)
{
	const auto str = input.utf8_str();

	return std::string(str.data(), str.length());
}
