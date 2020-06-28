// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "choose_conflict_resolve_mode_view.hpp"

#include <wx/commandlinkbutton.h>

#include "application.h"

using namespace mm;

ChooseConflictResolveModeView::ChooseConflictResolveModeView(wxWindow *parent)
	: wxDialog(parent, wxID_ANY, "Choose conflict resolve mode"_lng, wxDefaultPosition, wxSize(600, 200))
{
	auto automatic = new wxCommandLinkButton(this, wxID_ANY, "Automatic"_lng, "Automatically resolve mod conflicts when you enable or disable any mod"_lng);
	auto manual = new wxCommandLinkButton(this, wxID_ANY, "Manual"_lng, "Don't resolve conflicts automatically"_lng);

	automatic->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) {
		_conflictResolveMode = ConflictResolveMode::automatic;
		this->EndModal(wxID_OK);
	});

	manual->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) {
		_conflictResolveMode = ConflictResolveMode::manual;
		this->EndModal(wxID_OK);
	});

	auto mainSizer = new wxBoxSizer(wxVERTICAL);
	mainSizer->Add(automatic, wxSizerFlags(0).Expand().Border(wxALL, 4));
	mainSizer->Add(manual, wxSizerFlags(0).Expand().Border(wxALL, 4));

	SetSizer(mainSizer);
	Layout();
}

ConflictResolveMode ChooseConflictResolveModeView::conflictResolveMode() const
{
	return _conflictResolveMode;
}
