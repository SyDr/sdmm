// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "type/conflict_resolve_mode.hpp"

#include <wx/dialog.h>

namespace mm
{
	struct IIconStorage;

	class ChooseConflictResolveModeView : public wxDialog
	{
	public:
		ChooseConflictResolveModeView(wxWindow *parent);

		ConflictResolveMode conflictResolveMode() const;

	private:
		ConflictResolveMode _conflictResolveMode = ConflictResolveMode::undefined;
	};
}
