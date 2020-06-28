// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <memory>
#include <vector>

#include <wx/dialog.h>

#include "era2/era2_directory_structure.hpp"
#include "utility/wx_widgets_ptr.hpp"

class wxDataViewListCtrl;
class wxDataViewCtrl;

namespace mm
{
	class Application;
	class IIconStorage;
	struct IModDataProvider;
	class ModListModel;

	class ShowFileListDialog : public wxDialog
	{
	public:
		ShowFileListDialog(wxWindow* parent, Application& application,
						   IModDataProvider& dataProvider, Era2DirectoryStructure data);

	private:
		void createControls();
		void createListColumns();
		void bindEvents();
		void buildLayout();
		void fillData();

	private:
		Era2DirectoryStructure _data;

		wxObjectDataPtr<ModListModel> _listModel;
		wxWidgetsPtr<wxDataViewCtrl>  _mods = nullptr;

		wxWidgetsPtr<wxDataViewListCtrl> _fileList = nullptr;
	};
}
