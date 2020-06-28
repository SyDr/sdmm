// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "utility/wx_widgets_ptr.hpp"

#include <filesystem>
#include <memory>
#include <vector>

#include <wigwag/token_pool.hpp>
#include <wx/dialog.h>

class wxListView;
class wxImageList;
class wxListBox;
class wxButton;
class wxDataViewCtrl;
class wxStaticBox;
class wxDataViewListCtrl;

namespace mm
{
	struct IModPlatform;
	class ModListModel;
	class IIconStorage;

	class ManagePresetListView : public wxDialog
	{
	public:
		explicit ManagePresetListView(wxWindow* parent, IModPlatform& platform, IIconStorage& iconStorage);
		void onSavePresetRequested();

	private:
		void createControls();
		void createListColumns();
		void updateLayout();
		void bindEvents();
		void updateModList();

		void refreshListContent();

		void onLoadPresetRequested();
		void onRenamePreset();
		void onCopyPreset();
		void onDeletePreset();
		void onSelectionChanged();

		wxString getSelection() const;
		void     onFilesystemError(std::filesystem::filesystem_error const& e);

	private:
		const char* _program_name = nullptr;

		IModPlatform& _platform;
		IIconStorage& _iconStorage;
		wxString      _selected;

		wigwag::token_pool _connections;

		wxWidgetsPtr<wxStaticBox> _presets = nullptr;

		wxWidgetsPtr<wxDataViewListCtrl> _list = nullptr;
		std::vector<wxString>            _profiles;

		wxWidgetsPtr<wxButton> _new       = nullptr;
		wxWidgetsPtr<wxButton> _load      = nullptr;
		//wxWidgetsPtr<wxButton> _rename    = nullptr;
		//wxWidgetsPtr<wxButton> _copy      = nullptr;
		wxWidgetsPtr<wxButton> _remove    = nullptr;

		wxWidgetsPtr<wxStaticBox>     _preview = nullptr;
		wxObjectDataPtr<ModListModel> _listModel;
		wxWidgetsPtr<wxDataViewCtrl>  _mods = nullptr;
	};
}
