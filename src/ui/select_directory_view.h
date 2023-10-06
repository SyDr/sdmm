// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <memory>
#include <vector>

#include <wx/dialog.h>

#include "utility/wx_widgets_ptr.hpp"

class wxListBox;
class wxListView;
class wxGenericDirCtrl;
class wxStaticText;
class wxStaticBox;
class wxTextCtrl;
class wxButton;
class wxDataViewListCtrl;

namespace mm
{
	struct IAppConfig;
	struct IIconStorage;

	class SelectDirectoryDialog : public wxDialog
	{
	public:
		SelectDirectoryDialog(wxWindow* parent, IAppConfig& config, IIconStorage& iconStorage);

		wxString getSelectedPath() const;

	private:
		void createControls();
		void bindEvents();
		void buildLayout();
		void fillData();
		void OnMenuItemSelected(const wxCommandEvent& event);
		void OnListItemContextMenu();
		void removeFromListRequested();
		void starUnstarRequested();

	private:
		IAppConfig&   _appConfig;
		IIconStorage& _iconStorage;

		wxWidgetsPtr<wxGenericDirCtrl>   _explorerList   = nullptr;
		wxWidgetsPtr<wxDataViewListCtrl> _recentDirsList = nullptr;

		wxWidgetsPtr<wxStaticText> _selectedLabel    = nullptr;
		wxWidgetsPtr<wxTextCtrl>   _selectedPathEdit = nullptr;
		wxWidgetsPtr<wxButton>     _accept           = nullptr;

		struct  // Menu
		{
			wxMenu                   menu;
			wxWidgetsPtr<wxMenuItem> starUnstar     = nullptr;
			wxWidgetsPtr<wxMenuItem> removeFromList = nullptr;
		} _menu;
	};
}
