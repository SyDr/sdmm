// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <memory>
#include <unordered_map>
#include <unordered_set>

#include <wx/dataview.h>
#include <wx/menu.h>

#include "utility/wx_widgets_ptr.hpp"
#include "domain/plugin_list.hpp"

class wxDataViewCtrl;
class wxStaticBox;
class wxStaticText;
class wxBitmapComboBox;

namespace mm
{
	struct IPluginManager;
	struct IIconStorage;
	struct IModDataProvider;

	class PluginListModel;

	class PluginListView : public wxPanel
	{
	public:
		explicit PluginListView(wxWindow* parent, IPluginManager& pluginManager,
								IModDataProvider& modDataProvider, IIconStorage& iconStorage);

	private:
		void createControls();

		void createListControl();
		void createListColumns();

		void buildLayout();
		void bindEvents();

		void onSwitchSelectedStateRequested();

		void followSelection();
		void updateControlsState();

	private:
		IPluginManager& _manager;
		IIconStorage&   _iconStorage;

		PluginSource _selected;

	private:
		wxObjectDataPtr<PluginListModel> _listModel;

		wxWidgetsPtr<wxStaticBox>    _group       = nullptr;
		wxWidgetsPtr<wxDataViewCtrl> _list        = nullptr;
		wxWidgetsPtr<wxButton>       _changeState = nullptr;
		wxWidgetsPtr<wxCheckBox>     _showAll     = nullptr;
	};
}
