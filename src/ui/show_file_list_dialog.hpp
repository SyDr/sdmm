// SD Mod Manager

// Copyright (c) 2020-2024 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "domain/mod_list.hpp"
#include "era2/era2_directory_structure.hpp"
#include "mod_list_model.h"
#include "utility/wx_widgets_ptr.hpp"

#include <wx/dialog.h>

#include <memory>
#include <vector>

class wxDataViewListCtrl;
class wxDataViewCtrl;

namespace mm
{
	struct IIconStorage;
	struct IModDataProvider;
	class ModListModel;

	class ShowFileListDialog : public wxDialog
	{
	public:
		ShowFileListDialog(wxWindow* parent, IIconStorage& iconStorage, IModDataProvider& dataProvider,
			ModList list, const fs::path& basePath);

	private:
		void createControls();
		void createSelectModsList();
		void createResultList();
		void createDetailsList();
		void bindEvents();
		void buildLayout();
		void loadData();
		void fillData();

	private:
		IIconStorage&     _iconStorage;
		IModDataProvider& _dataProvider;
		fs::path          _basePath;

		wxWidgetsPtr<wxStaticBox>     _selectOptionsGroup = nullptr;
		wxObjectDataPtr<ModListModel> _selectModsModel;
		wxWidgetsPtr<wxDataViewCtrl>  _selectModsList = nullptr;
		ModList                       _mods;

		wxWidgetsPtr<wxButton> _continue = nullptr;

		wxWidgetsPtr<wxStaticBox>        _resultGroup = nullptr;
		wxWidgetsPtr<wxDataViewListCtrl> _fileList    = nullptr;
		wxWidgetsPtr<wxDataViewListCtrl> _detailsList = nullptr;

		Era2DirectoryStructure _data;
	};
}
