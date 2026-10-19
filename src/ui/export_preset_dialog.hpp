// SD Mod Manager

// Copyright (c) 2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "type/filesystem.hpp"
#include "utility/wx_widgets_ptr.hpp"

#include <memory>
#include <vector>

#include <wx/dialog.h>

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

	class ExportPresetDialog : public wxDialog
	{
	public:
		explicit ExportPresetDialog(
			wxWindow* parent, IModPlatform& platform, IIconStorage& iconStorage, const std::string& preset);

	private:
		void createControls();
		void updateLayout();
		void bindEvents();
		void updatePreview();

		void onCopyToClipboardRequested();
		void onSaveToFileRequested();

	private:
		IModPlatform& _platform;
		IIconStorage& _iconStorage;
		std::string   _selected;

		wxWidgetsPtr<wxStaticBox> _previewGroup = nullptr;
		wxWidgetsPtr<wxTextCtrl>  _exportData   = nullptr;

		wxWidgetsPtr<wxStaticBox>  _optionsBox      = nullptr;
		wxWidgetsPtr<wxCheckBox>   _saveExecutable  = nullptr;
		wxWidgetsPtr<wxCheckBox>   _savePlugins     = nullptr;
		wxWidgetsPtr<wxStaticText> _exportNameLabel = nullptr;
		wxWidgetsPtr<wxTextCtrl>   _exportName      = nullptr;

		wxWidgetsPtr<wxButton> _copyToClipboard = nullptr;
		wxWidgetsPtr<wxButton> _saveToFile      = nullptr;

		wxWidgetsPtr<wxButton> _ok = nullptr;

		wxWidgetsPtr<wxInfoBarGeneric> _infoBar = nullptr;
		wxTimer                        _infoBarTimer;
	};
}
