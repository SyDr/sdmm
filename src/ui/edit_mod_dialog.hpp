// SD Mod Manager

// Copyright (c) 2025 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "domain/mod_list.hpp"
#include "era2/era2_directory_structure.hpp"
#include "mod_list_model.h"
#include "utility/wx_widgets_ptr.hpp"

#include <wx/dialog.h>

#include <memory>
#include <vector>

#include <wx/statbox.h>

class wxDataViewListCtrl;
class wxDataViewCtrl;
class wxCheckBox;
class wxStaticText;
class wxHtmlWindow;

namespace mm
{
	struct IIconStorage;
	struct IModDataProvider;
	struct IModPlatform;
	class ModListModel;

	class EditModDialog : public wxDialog
	{
	public:
		EditModDialog(wxWindow* parent, IModPlatform& managedPlatform, const std::string& name);

	private:
		void createControls();
		void bindEvents();
		void buildLayout();
		void loadData();
		void reloadDataRequested();
		void saveRequested();

		void OnDataEditTextChanged();

	private:
		IModPlatform& _managedPlatform;
		fs::path      _basePath;
		std::string   _modName;

		wxWidgetsPtr<wxStaticBox> _modOptionsGroup = nullptr;
		wxWidgetsPtr<wxTextCtrl>  _modDataEdit     = nullptr;
		wxTimer                   _editTimer;

		wxWidgetsPtr<wxStaticBox>  _docsGroup     = nullptr;
		wxWidgetsPtr<wxHtmlWindow> _docHtmlWindow = nullptr;

		wxWidgetsPtr<wxButton>     _openFolder = nullptr;
		wxWidgetsPtr<wxButton>     _reload     = nullptr;
		wxWidgetsPtr<wxButton>     _save       = nullptr;
		wxWidgetsPtr<wxStaticText> _status     = nullptr;

		wxWidgetsPtr<wxButton> _close = nullptr;
	};
}
