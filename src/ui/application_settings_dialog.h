// SD Mod Manager

// Copyright (c) 2025 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "domain/mod_data.hpp"
#include "domain/mod_list.hpp"
#include "interface/imod_data_provider.hpp"
#include "mod_list_model.h"
#include "utility/wx_widgets_ptr.hpp"

#include <vector>

#include <wx/dialog.h>

namespace mm
{
	struct Application;

	struct ApplicationSettingsDialog : public wxDialog
	{
		ApplicationSettingsDialog(wxWindow* parent, Application& app);

	private:
		void createControls();

		void bindEvents();
		void buildLayout();

	private:
		Application& _app;

		wxWidgetsPtr<wxStaticBox> _globalGroup = nullptr;

		wxWidgetsPtr<wxStaticText> _updateStatic = nullptr;
		wxWidgetsPtr<wxChoice>     _updateChoice = nullptr;

		wxWidgetsPtr<wxStaticText> _interfaceSizeStatic = nullptr;
		wxWidgetsPtr<wxChoice>     _interfaceSizeChoice = nullptr;

		// wxWidgetsPtr<wxStaticBox>   _conflictResolveModeDialog = nullptr;
		// wxWidgetsPtr<wxRadioButton> _rupdateStatic             = nullptr;
		// wxWidgetsPtr<wxRadioButton> _rupdateChoice             = nullptr;
		//
		wxWidgetsPtr<wxButton> _save   = nullptr;
		wxWidgetsPtr<wxButton> _cancel = nullptr;
	};
}
