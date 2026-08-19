// SD Mod Manager

// Copyright (c) 2026 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "domain/mod_list.hpp"
#include "type/filesystem.hpp"
#include "utility/wx_widgets_ptr.hpp"

#include <wx/dialog.h>

#include <mutex>
#include <vector>

class wxDataViewListCtrl;
class wxDataViewCtrl;
class wxCheckBox;
class wxDirPickerCtrl;

namespace mm
{
	struct IIconStorage;
	struct IModDataProvider;
	class ModListModel;

	class CreateModPackageDialog : public wxDialog
	{
	public:
		CreateModPackageDialog(wxWindow* parent, IIconStorage& iconStorage, IModDataProvider& dataProvider,
			ModList list, const fs::path& basePath, const fs::path& tempPath, const std::string& selectedMod);

		struct PacketInfo
		{
			fs::path                source;  // relative to game dir
			fs::path                target;  // full path to target
			std::optional<fs::path> icon;
		};

	private:
		void createControls();
		void createSelectModsList();
		void bindEvents();
		void buildLayout();
		void createPackageRequested();
		void selectOutputDirectoryRequested();
		void updateLog();

		void createPackageImpl(std::stop_token token, std::vector<PacketInfo> ordered, fs::path basePath,
			bool packIndividually, bool packInBackground);

	private:
		IIconStorage&     _iconStorage;
		IModDataProvider& _dataProvider;
		fs::path          _basePath;
		fs::path          _tempPath;

		wxWidgetsPtr<wxStaticBox>     _selectOptionsGroup = nullptr;
		wxObjectDataPtr<ModListModel> _selectModsModel;
		wxWidgetsPtr<wxDataViewCtrl>  _selectModsList = nullptr;
		ModList                       _mods;
		std::string                   _initiallySelected;

		wxWidgetsPtr<wxStaticText> _targetStatic          = nullptr;
		wxWidgetsPtr<wxTextCtrl>   _targetDirectory       = nullptr;
		wxWidgetsPtr<wxButton>     _pickTargetDirectory   = nullptr;
		wxWidgetsPtr<wxCheckBox>   _packIntoSingleArchive = nullptr;
		wxWidgetsPtr<wxCheckBox>   _packInBackground      = nullptr;

		// wxWidgetsPtr<wxStaticText> _sevenZipSwitchesStatic = nullptr;
		// wxWidgetsPtr<wxTextCtrl>   _sevenZipSwitchesEdit   = nullptr;

		wxWidgetsPtr<wxButton> _continue = nullptr;

		wxWidgetsPtr<wxStaticBox> _logGroup = nullptr;
		wxWidgetsPtr<wxTextCtrl>  _logCtrl  = nullptr;

		wxWidgetsPtr<wxButton> _close = nullptr;

		std::jthread _thread;

		std::mutex               _mutex;
		std::vector<std::string> _pendingLogEntries;

		wxTimer _progressTimer;
	};
}
