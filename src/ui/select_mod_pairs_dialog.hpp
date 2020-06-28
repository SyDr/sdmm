// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <memory>
#include <vector>

#include <wx/dialog.h>

#include "domain/mod_list.hpp"
#include "utility/wx_widgets_ptr.hpp"

class wxDataViewListCtrl;

namespace mm
{
	class Application;
	class IIconStorage;
	struct IModDataProvider;
	struct IModPlatform;

	class SelectModPairsDialog : public wxDialog
	{
	public:
		SelectModPairsDialog(wxWindow& parent, Application& application,
							 IModDataProvider&                          dataProvider,
						 std::vector<std::pair<wxString, wxString>> values);

		std::set<std::pair<wxString, wxString>> getSelected() const;

	private:
		void createControls();
		void createListControl();
		void fillData();
		void bindEvents();
		void buildLayout();

	private:
		IModDataProvider& _modDataProvider;
		IIconStorage& _iconStorage;

		std::vector<std::pair<wxString, wxString>> _values;

		wxWidgetsPtr<wxDataViewListCtrl> _list = nullptr;

		wxWidgetsPtr<wxButton> _continue = nullptr;
		wxWidgetsPtr<wxButton> _cancel   = nullptr;
	};
}
