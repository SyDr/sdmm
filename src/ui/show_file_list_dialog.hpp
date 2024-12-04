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
class wxCheckBox;

namespace mm
{
	struct IIconStorage;
	struct IModDataProvider;
	class ModListModel;

	class ShowFileListDialog : public wxDialog
	{
	public:
		enum class ShowGameFiles
		{
			none,
			overriden_only,
			all
		};

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
		void fillData(ShowGameFiles gameFiles);
		void updateProgress();

		void doLoadData(
			std::stop_token token, std::vector<std::string> ordered, ShowGameFiles gameFiles);

	private:
		IIconStorage&     _iconStorage;
		IModDataProvider& _dataProvider;
		fs::path          _basePath;

		wxWidgetsPtr<wxStaticBox>     _selectOptionsGroup = nullptr;
		wxObjectDataPtr<ModListModel> _selectModsModel;
		wxWidgetsPtr<wxDataViewCtrl>  _selectModsList = nullptr;
		ModList                       _mods;
		wxWidgetsPtr<wxCheckBox>      _showGameFiles    = nullptr;
		wxWidgetsPtr<wxCheckBox>      _showGameFilesAll = nullptr;

		wxWidgetsPtr<wxButton> _continue = nullptr;

		wxWidgetsPtr<wxStaticBox>        _resultGroup = nullptr;
		wxWidgetsPtr<wxDataViewListCtrl> _fileList    = nullptr;
		wxWidgetsPtr<wxDataViewListCtrl> _detailsList = nullptr;

		wxWidgetsPtr<wxStaticText> _progressStatic = nullptr;
		wxTimer                    _progressTimer;
		wxWidgetsPtr<wxButton>     _close = nullptr;

		std::jthread           _thread;
		std::stop_token        _stopToken;
		std::mutex             _progressMutex;
		std::string            _progress;
		Era2DirectoryStructure _data;
	};
}
