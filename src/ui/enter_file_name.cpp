// SD Mod Manager

// Copyright (c) 2025 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "stdafx.h"

#include "enter_file_name.hpp"

#include <wx/generic/textdlgg.h>

wxString mm::enterFileName(wxWindow* parent, wxString message, wxString caption, wxString name)
{
	wxTextEntryDialog ted(parent, message, caption, name);
	ted.SetTextValidator(wxTextValidatorStyle::wxFILTER_EMPTY);
	ted.GetTextValidator()->SetCharExcludes(L"\\/:*?\"<>|");

	if (ted.ShowModal() != wxID_OK)
		return {};

	return ted.GetValue();
}
