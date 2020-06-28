// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <memory>
#include <vector>

#include <wx/dialog.h>

#include "domain/mod_list.hpp"
#include "utility/wx_widgets_ptr.hpp"

#include "mod_list_model.h"

class wxDataViewCtrl;

namespace mm
{
	class Application;
	class IIconStorage;
	struct IModPlatform;

	class SelectModsDialog : public wxDialog
	{
	public:
		SelectModsDialog(wxWindow& parent, Application& application, IModDataProvider& dataProvider,
						 ModList list);

		std::unordered_set<wxString> const& getSelected() const;

	private:
		void createControls();
		void createListControl();
		void createListColumns();

		void bindEvents();
		void buildLayout();

	private:
		IIconStorage& _iconStorage;

		wxObjectDataPtr<ModListModel> _listModel;
		ModList                       _mods;

		wxWidgetsPtr<wxDataViewCtrl> _list = nullptr;

		wxWidgetsPtr<wxButton> _continue = nullptr;
		wxWidgetsPtr<wxButton> _cancel   = nullptr;
	};
}
