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
	struct IIconStorage;

	class ImportPresetDialog : public wxDialog
	{
	public:
		explicit ImportPresetDialog(wxWindow* parent, IModPlatform& platform, IIconStorage& iconStorage);

	private:
		void createControls();
		void updateLayout();
		void bindEvents();
		void updatePreview();
		void updateOkButton();
		void doImportAndClose();

		void onPasteFromClipboardRequested();
		void onImportFromFileRequested();

	private:
		IModPlatform& _platform;
		IIconStorage& _iconStorage;

		wxWidgetsPtr<wxStaticBox> _previewGroup = nullptr;
		wxWidgetsPtr<wxTextCtrl>  _importData   = nullptr;

		wxWidgetsPtr<wxStaticBox>  _optionsBox      = nullptr;
		wxWidgetsPtr<wxStaticText> _importNameLabel = nullptr;
		wxWidgetsPtr<wxTextCtrl>   _importName      = nullptr;
		wxWidgetsPtr<wxButton>     _clearName       = nullptr;

		wxWidgetsPtr<wxButton>   _fromClipboard = nullptr;
		wxWidgetsPtr<wxButton>   _fromFile      = nullptr;
		wxWidgetsPtr<wxCheckBox> _loadNow       = nullptr;

		wxWidgetsPtr<wxButton> _ok = nullptr;

		wxWidgetsPtr<wxInfoBarGeneric> _infoBar = nullptr;
		wxTimer                        _infoBarTimer;
	};
}
