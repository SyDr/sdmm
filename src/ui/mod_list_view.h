// SD Mod Manager

// Copyright (c) 2020-2024 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <memory>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <wx/dataview.h>
#include <wx/menu.h>

#include "type/mod_list_model_structs.hpp"
#include "utility/wx_widgets_ptr.hpp"

class wxCheckBox;
class wxDataViewCtrl;
class wxDataViewEvent;
class wxInfoBarGeneric;
class wxStaticBox;
class wxStaticText;
class wxWebView;
class wxComboCtrl;
class wxSearchCtrl;
class mmCheckListBoxComboPopup;
class wxWebViewEvent;
class wxHtmlWindow;

namespace mm
{
	struct IModPlatform;
	struct IModManager;
	struct IIconStorage;

	class ModListModel;
	struct ImageGalleryView;

	class ModListView : public wxPanel
	{
	public:
		explicit ModListView(wxWindow* parent, IModPlatform& managedPlatform, IIconStorage& iconStorage,
			wxStatusBar* statusBar);

	private:
		void createControls(const wxString& managedPath);

		void createListControl();
		void createListColumns();

		void buildLayout();
		void bindEvents();

		void OnListItemContextMenu(const wxDataViewItem& item);
		void OnMenuItemSelected(const wxCommandEvent& event);
		void OnWebViewNavigating(wxWebViewEvent& event);

		void onSwitchSelectedModStateRequested();
		void onResetSelectedModStateRequested();

		void expandChildren();
		bool followSelection();
		void updateControlsState();
		void updateCategoryFilterContent();
		void onSortModsRequested(const std::string& enablingMod, const std::string& disablingMod);
		void onRemoveModRequested();
		void openGalleryRequested();

		void updateGalleryState(bool show);

	private:
		IModPlatform& _managedPlatform;
		IModManager&  _modManager;
		IIconStorage& _iconStorage;

		std::string _selectedMod;
		std::string _selectedModCached;

	private:
		wxObjectDataPtr<ModListModel>               _listModel;
		std::set<ModListDsplayedData::GroupItemsBy> _collapsedCategories;
		std::set<std::string>                       _hiddenCategories;

		wxWidgetsPtr<wxStaticBox>              _group          = nullptr;
		wxWidgetsPtr<wxSearchCtrl>             _filterText     = nullptr;
		wxWidgetsPtr<wxComboCtrl>              _filterCategory = nullptr;
		wxWidgetsPtr<mmCheckListBoxComboPopup> _filterPopup    = nullptr;
		std::vector<std::string>               _categories;
		wxWidgetsPtr<wxDataViewCtrl>           _list = nullptr;

		wxWidgetsPtr<wxButton> _configure = nullptr;

		wxWidgetsPtr<wxButton> _moveUp      = nullptr;
		wxWidgetsPtr<wxButton> _moveDown    = nullptr;
		wxWidgetsPtr<wxButton> _changeState = nullptr;

		wxWidgetsPtr<wxButton> _sort = nullptr;

		wxWidgetsPtr<wxStaticBox>  _modDescriptionGroup      = nullptr;
		wxWidgetsPtr<wxWebView>    _modDescriptionWebView    = nullptr;
		wxWidgetsPtr<wxHtmlWindow> _modDescriptionHtmlWindow = nullptr;
		wxWidgetsPtr<wxTextCtrl>   _modDescriptionTextCtrl   = nullptr;

		struct  // Menu
		{
			wxMenu                   menu;
			wxWidgetsPtr<wxMenuItem> openHomepage   = nullptr;
			wxWidgetsPtr<wxMenuItem> openDir        = nullptr;
			wxWidgetsPtr<wxMenuItem> archive        = nullptr;
			wxWidgetsPtr<wxMenuItem> deleteOrRemove = nullptr;
		} _menu;

		wxWidgetsPtr<wxButton>         _showGallery  = nullptr;
		wxWidgetsPtr<wxButton>         _openGallery  = nullptr;
		wxWidgetsPtr<wxStaticBox>      _gallery      = nullptr;
		wxWidgetsPtr<ImageGalleryView> _galleryView  = nullptr;
		bool                           _galleryShown = true;

		wxWidgetsPtr<wxInfoBarGeneric> _infoBar = nullptr;
		wxTimer                        _infoBarTimer;

		wxStatusBar* _statusBar = nullptr;
	};
}
