// SD Mod Manager

// Copyright (c) 2020-2025 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "choose_conflict_resolve_mode_view.hpp"

#include <wx/commandlinkbutton.h>

#include "application.h"

using namespace mm;

ChooseConflictResolveModeView::ChooseConflictResolveModeView(wxWindow* parent)
	: wxDialog(parent, wxID_ANY, "dialog/settings/conflicts/caption"_lng, wxDefaultPosition, wxSize(600, 200))
{
	auto automatic =
		new wxCommandLinkButton(this, wxID_ANY, "dialog/settings/conflicts/automatic"_lng, "dialog/settings/conflicts/automatic_note"_lng);
	auto manual =
		new wxCommandLinkButton(this, wxID_ANY, "dialog/settings/conflicts/manual"_lng, "dialog/settings/conflicts/manual_note"_lng);

	automatic->Bind(wxEVT_BUTTON,
					[=](wxCommandEvent&)
					{
						_conflictResolveMode = ConflictResolveMode::automatic;
						this->EndModal(wxID_OK);
					});

	manual->Bind(wxEVT_BUTTON,
				 [=](wxCommandEvent&)
				 {
					 _conflictResolveMode = ConflictResolveMode::manual;
					 this->EndModal(wxID_OK);
				 });

	auto mainSizer = new wxBoxSizer(wxVERTICAL);
	mainSizer->Add(automatic, wxSizerFlags(0).Expand().Border(wxALL, 4));
	mainSizer->Add(manual, wxSizerFlags(0).Expand().Border(wxALL, 4));

	SetSizer(mainSizer);
	Layout();
	SetSize(-1, -1, -1, -1, wxSIZE_AUTO);
}

ConflictResolveMode ChooseConflictResolveModeView::conflictResolveMode() const
{
	return _conflictResolveMode;
}
