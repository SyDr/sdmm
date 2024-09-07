// SD Mod Manager

// Copyright (c) 2024 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "domain/mod_data.hpp"
#include "domain/mod_list.hpp"
#include "interface/imod_data_provider.hpp"
#include "mod_list_model.h"
#include "utility/wx_widgets_ptr.hpp"

#include <vector>

#include <wx/dialog.h>

class wxDataViewCtrl;

namespace mm
{
	class ConfigureMainListView : public wxDialog, public IModDataProvider
	{
	public:
		ConfigureMainListView(wxWindow* parent, IIconStorage& iconStorage, const std::vector<int>& columns,
			ModListModelManagedMode initialManagedMode, ModListModelArchivedMode initialArchivedMode);

		std::vector<int> getColumns() const;

		ModListModelManagedMode  getManagedMode() const;
		ModListModelArchivedMode getArchivedMode() const;

	private:
		void createControls();
		void createListControl();
		void createManagedControl();
		void createArchivedControl();
		void createListColumns();

		void bindEvents();
		void buildLayout();

		const ModData& modData(const std::string& id) override;

	private:
		std::map<std::string, ModData> _data;

		const ModListModelManagedMode  _initialManagedMode;
		const ModListModelArchivedMode _initialArchivedMode;

		wxWidgetsPtr<wxStaticText> _managedStatic = nullptr;
		wxWidgetsPtr<wxChoice>     _managedChoice = nullptr;

		wxWidgetsPtr<wxStaticText> _archivedStatic = nullptr;
		wxWidgetsPtr<wxChoice>     _archivedChoice = nullptr;

		wxObjectDataPtr<ModListModel> _listModel;
		ModList                       _mods;

		wxWidgetsPtr<wxDataViewCtrl> _list = nullptr;

		wxWidgetsPtr<wxButton> _save   = nullptr;
		wxWidgetsPtr<wxButton> _cancel = nullptr;
	};
}
