// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <memory>
#include <unordered_map>
#include <unordered_set>

#include <wx/dataview.h>
#include <wx/menu.h>
#include <wigwag/token_pool.hpp>

#include "utility/wx_widgets_ptr.hpp"

class wxDataViewCtrl;
class wxDataViewEvent;
class wxStaticBox;
class wxCheckBox;
class wxStaticText;

namespace mm
{
	class IRemoteMod;
	struct IModPlatform;
	struct IModManager;
	class IPlatformDescriptor;
	class IIconStorage;

	class ModListModel;

	class ModListView : public wxPanel
	{
	public:
		explicit ModListView(wxWindow* parent, IModPlatform& managedPlatform, IIconStorage& iconStorage);

	private:
		void createControls();

		void createListControl();
		void createListColumns();

		void buildLayout();
		void bindEvents();

		void OnListItemContextMenu(const wxDataViewItem& item);
		void OnMenuItemSelected(const wxCommandEvent& event);
		void OnEventCheckboxShowHidden(const wxCommandEvent& event);

		void onSwitchSelectedModStateRequested();

		void followSelection();
		void updateControlsState();
		void selectExeToLaunch();
		void updateExecutableIcon();
		void onSortModsRequested();
		void onRemoveModRequested();

		void onLaunchGameRequested();

	private:
		IModPlatform& _managedPlatform;
		IModManager& _modManager;
		IIconStorage& _iconStorage;

		wigwag::token_pool _connections;

		wxString _selectedMod;

	private:
		wxObjectDataPtr<ModListModel> _listModel;

		wxWidgetsPtr<wxStaticBox> _group = nullptr;
		wxWidgetsPtr<wxDataViewCtrl> _list = nullptr;
		wxWidgetsPtr<wxCheckBox> _checkboxShowHidden = nullptr;

		wxWidgetsPtr<wxButton> _moveUp = nullptr;
		wxWidgetsPtr<wxButton> _moveDown = nullptr;
		wxWidgetsPtr<wxButton> _changeState = nullptr;
		wxWidgetsPtr<wxButton> _sort = nullptr;

		wxWidgetsPtr<wxButton> _launchButton = nullptr;
		wxWidgetsPtr<wxButton> _launchManageButton = nullptr;

		wxWidgetsPtr<wxTextCtrl> _modDescription = nullptr;

		struct // Menu
		{
			wxMenu menu;
			wxWidgetsPtr<wxMenuItem> showOrHide = nullptr;
			wxWidgetsPtr<wxMenuItem> openHomepage = nullptr;
			wxWidgetsPtr<wxMenuItem> openDir = nullptr;
			wxWidgetsPtr<wxMenuItem> deleteOrRemove = nullptr;
		} _menu;
	};
}
