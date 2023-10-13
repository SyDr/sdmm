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
	struct Application;
	struct IIconStorage;
	struct IModDataProvider;
	struct IModPlatform;

	class SelectModPairsDialog : public wxDialog
	{
	public:
		SelectModPairsDialog(wxWindow& parent, Application& application, IModDataProvider& dataProvider,
			std::vector<std::pair<std::string, std::string>> values);

		std::set<std::pair<std::string, std::string>> getSelected() const;

	private:
		void createControls();
		void createListControl();
		void fillData();
		void bindEvents();
		void buildLayout();

	private:
		IModDataProvider& _modDataProvider;
		IIconStorage&     _iconStorage;

		std::vector<std::pair<std::string, std::string>> _values;

		wxWidgetsPtr<wxDataViewListCtrl> _list = nullptr;

		wxWidgetsPtr<wxButton> _continue = nullptr;
		wxWidgetsPtr<wxButton> _cancel   = nullptr;
	};
}
