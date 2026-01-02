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
	struct ILocalConfig;

	struct ApplicationSettingsDialog : public wxDialog
	{
		ApplicationSettingsDialog(wxWindow* parent, Application& app, ILocalConfig* localConfig);

	private:
		void createControls();

		void bindEvents();
		void buildLayout();

	private:
		Application&  _app;
		ILocalConfig* _localConfig = nullptr;

		wxWidgetsPtr<wxStaticBox> _globalGroup = nullptr;

		wxWidgetsPtr<wxStaticText> _updateStatic = nullptr;
		wxWidgetsPtr<wxChoice>     _updateChoice = nullptr;

		wxWidgetsPtr<wxStaticText> _languageStatic = nullptr;
		wxWidgetsPtr<wxChoice>     _languageChoice = nullptr;

		wxWidgetsPtr<wxStaticText> _interfaceSizeStatic = nullptr;
		wxWidgetsPtr<wxChoice>     _interfaceSizeChoice = nullptr;

		wxWidgetsPtr<wxStaticText> _interfaceLabelStatic = nullptr;
		wxWidgetsPtr<wxChoice>     _interfaceLabelChoice = nullptr;

		wxWidgetsPtr<wxStaticText> _modDescriptionControlStatic = nullptr;
		wxWidgetsPtr<wxChoice>     _modDescriptionControlChoice = nullptr;

		wxWidgetsPtr<wxStaticBox> _platformGroup = nullptr;

		wxWidgetsPtr<wxStaticText> _conflictResolveStatic = nullptr;
		wxWidgetsPtr<wxChoice>     _conflictResolveChoice = nullptr;

		wxWidgetsPtr<wxStaticText> _warnAboutConflictsStatic = nullptr;
		wxWidgetsPtr<wxChoice>     _warnAboutConflictsChoice = nullptr;

		wxWidgetsPtr<wxButton> _save   = nullptr;
		wxWidgetsPtr<wxButton> _cancel = nullptr;
	};
}
