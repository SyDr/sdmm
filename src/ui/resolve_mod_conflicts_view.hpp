// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <memory>
#include <vector>

#include <wx/dialog.h>

#include "utility/wx_widgets_ptr.hpp"
#include "domain/mod_list.hpp"

#include "mod_list_model.h"

class wxDataViewCtrl;

namespace mm
{
	struct IAppConfig;
	struct IIconStorage;
	struct IModPlatform;

	class ResolveModConflictsView : public wxDialog
	{
	public:
		ResolveModConflictsView(wxWindow *parent, IAppConfig& config, IIconStorage& iconStorage, IModPlatform& managedPlatform);

	private:
		void createControls();
		void createListControl();
		void createListColumns();

		void bindEvents();
		void buildLayout();

		void doResolveConflicts();

	private:
		IModPlatform& _managedPlatform;
		IAppConfig& _appConfig;
		IIconStorage& _iconStorage;

		wxObjectDataPtr<ModListModel> _listModel;
		ModList _sortedMods;

		wxWidgetsPtr<wxDataViewCtrl> _list = nullptr;
		wxWidgetsPtr<wxTextCtrl> _log = nullptr;

		wxWidgetsPtr<wxButton> _start = nullptr;
		wxWidgetsPtr<wxButton> _apply = nullptr;
	};
}
