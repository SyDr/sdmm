// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <memory>
#include <unordered_map>
#include <unordered_set>

#include <wx/dataview.h>
#include <wx/menu.h>

#include "mod_list_model_mode.hpp"
#include "utility/wx_widgets_ptr.hpp"

class wxCheckBox;
class wxDataViewCtrl;
class wxDataViewEvent;
class wxInfoBarGeneric;
class wxStaticBox;
class wxStaticText;
class wxWebView;

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
			ModListModelMode listMode = ModListModelMode::flat);

	private:
		void createControls(const wxString& managedPath);

		void createListControl();
		void createListColumns();

		void buildLayout();
		void bindEvents();

		void OnListItemContextMenu(const wxDataViewItem& item);
		void OnMenuItemSelected(const wxCommandEvent& event);

		void onSwitchSelectedModStateRequested();
		void onResetSelectedModStateRequested();

		void expandChildren();
		void followSelection();
		void updateControlsState();
		void onSortModsRequested(const std::string& disablingMod);
		void onRemoveModRequested();
		void openGalleryRequested();

		void updateGalleryState(bool show, bool expand);

	private:
		IModPlatform& _managedPlatform;
		IModManager&  _modManager;
		IIconStorage& _iconStorage;

		std::string _selectedMod;

	private:
		wxObjectDataPtr<ModListModel>   _listModel;
		std::unordered_set<std::string> _hiddenCategories;

		wxWidgetsPtr<wxStaticBox>    _group              = nullptr;
		wxWidgetsPtr<wxDataViewCtrl> _list               = nullptr;

		wxWidgetsPtr<wxButton> _moveUp      = nullptr;
		wxWidgetsPtr<wxButton> _moveDown    = nullptr;
		wxWidgetsPtr<wxButton> _changeState = nullptr;
		wxWidgetsPtr<wxButton> _resetState  = nullptr;

		wxWidgetsPtr<wxButton> _sort = nullptr;

		wxWidgetsPtr<wxWebView>  _modDescription      = nullptr;
		wxWidgetsPtr<wxTextCtrl> _modDescriptionPlain = nullptr;

		struct  // Menu
		{
			wxMenu                   menu;
			wxWidgetsPtr<wxMenuItem> openHomepage   = nullptr;
			wxWidgetsPtr<wxMenuItem> openDir        = nullptr;
			wxWidgetsPtr<wxMenuItem> deleteOrRemove = nullptr;
		} _menu;

		wxWidgetsPtr<wxButton>         _showGallery     = nullptr;
		wxWidgetsPtr<wxButton>         _openGallery     = nullptr;
		wxWidgetsPtr<wxButton>         _expandGallery   = nullptr;
		wxWidgetsPtr<ImageGalleryView> _galleryView     = nullptr;
		bool                           _galleryShown    = false;
		bool                           _galleryExpanded = false;

		wxWidgetsPtr<wxInfoBarGeneric> _infoBar = nullptr;
		wxTimer                        _infoBarTimer;
	};
}
