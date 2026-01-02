// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "type/filesystem.hpp"
#include "utility/wx_widgets_ptr.hpp"

#include <memory>
#include <vector>

#include <wx/dialog.h>
#include <wx/timer.h>

class wxListView;
class wxImageList;
class wxListBox;
class wxButton;
class wxDataViewCtrl;
class wxStaticBox;
class wxDataViewListCtrl;
class wxInfoBarGeneric;

namespace mm
{
	struct IModPlatform;
	class ModListModel;
	class PluginListModel;
	struct IIconStorage;

	class ManagePresetListView : public wxPanel
	{
	public:
		explicit ManagePresetListView(wxWindow* parent, IModPlatform& platform, IIconStorage& iconStorage);

	private:
		void createControls();
		void createListColumns();
		void updateLayout();
		void bindEvents();
		void updatePreview();

		void refreshListContent();

		void onLoadPresetRequested();
		void onSavePresetRequested(std::string baseName);
		void onExportPresetRequested(std::string baseName);
		void onImportPresetRequested();
		void onRenamePreset();
		void onCopyPreset();
		void onDeletePreset();
		void onSelectionChanged();

		std::string getSelection() const;

	private:
		IModPlatform& _platform;
		IIconStorage& _iconStorage;
		std::string   _selected;

		wxWidgetsPtr<wxStaticBox> _presets = nullptr;

		wxWidgetsPtr<wxDataViewListCtrl> _list = nullptr;
		std::vector<std::string>         _profiles;

		wxWidgetsPtr<wxButton> _load = nullptr;
		wxWidgetsPtr<wxButton> _save = nullptr;

		wxWidgetsPtr<wxButton> _export = nullptr;
		wxWidgetsPtr<wxButton> _import = nullptr;

		wxWidgetsPtr<wxButton> _rename = nullptr;
		wxWidgetsPtr<wxButton> _copy   = nullptr;

		wxWidgetsPtr<wxButton> _remove = nullptr;

		wxWidgetsPtr<wxStaticBox>     _preview = nullptr;
		wxObjectDataPtr<ModListModel> _listModel;
		wxWidgetsPtr<wxDataViewCtrl>  _mods = nullptr;

		wxWidgetsPtr<wxInfoBarGeneric> _infoBar = nullptr;
		wxTimer                        _infoBarTimer;
	};
}
