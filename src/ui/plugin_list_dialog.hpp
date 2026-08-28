// SD Mod Manager

// Copyright (c) 2026 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "domain/mod_data.hpp"
#include "domain/mod_plugin.hpp"
#include "utility/wx_widgets_ptr.hpp"

#include <wx/panel.h>

#include <functional>
#include <array>
#include <optional>
#include <string>
#include <vector>

class wxButton;
class wxCommandEvent;
class wxKeyEvent;
class wxSearchCtrl;
class wxStaticBox;
class wxTextCtrl;
class wxTreeCtrl;
class wxTreeEvent;
class wxTreeItemId;

namespace mm
{
	struct IIconStorage;
	struct IPluginManager;

	class PluginListView : public wxPanel
	{
	public:
		PluginListView(wxWindow* parent, IPluginManager& manager, IIconStorage& iconStorage,
			int actionColumnWidth, std::array<int, 2> precedingActionSlotHeights,
			std::function<void()> onBack);
		~PluginListView() override;

		[[nodiscard]] bool load(const ModData& mod);
		void               focusSelectedPlugin();

	private:
		void createControls();
		bool createTreeImages();
		void buildLayout();
		void bindEvents();
		void populateTree(std::optional<std::size_t> preferredSelection = std::nullopt,
			bool focusSelection = true);
		void onCharHook(wxKeyEvent& event);
		void onFilterChanged(wxCommandEvent& event);
		void onTreeSelectionChanged(wxTreeEvent& event);
		void onTreeItemActivated(wxTreeEvent& event);
		void onChangeState(wxCommandEvent& event);
		void onBack(wxCommandEvent& event);
		void updateSelection();
		void updateChangeStateButton(const ModPlugin* plugin);
		void updatePluginItem(const wxTreeItemId& item, const ModPlugin& plugin);
		void toggleSelected();

		[[nodiscard]] bool                       pluginPassesFilter(const ModPlugin& plugin) const;
		[[nodiscard]] std::optional<std::size_t> selectedPluginIndex();
		[[nodiscard]] ModPlugin* selectedPlugin();

	private:
		IPluginManager& _manager;
		IIconStorage&   _iconStorage;
		int             _actionColumnWidth;
		std::array<int, 2> _precedingActionSlotHeights;

		std::function<void()>  _onBack;
		std::optional<ModData> _mod;

		std::vector<ModPlugin> _plugins;
		bool                   _treeHasImages = false;
		bool                   _populatingTree = false;
		bool                   _closing        = false;
		std::string            _filter;

		wxWidgetsPtr<wxStaticBox> _pluginsGroup     = nullptr;
		wxWidgetsPtr<wxSearchCtrl> _filterText      = nullptr;
		wxWidgetsPtr<wxTreeCtrl>  _tree             = nullptr;
		wxWidgetsPtr<wxStaticBox> _descriptionGroup = nullptr;
		wxWidgetsPtr<wxTextCtrl>  _description      = nullptr;
		wxWidgetsPtr<wxButton>    _changeState      = nullptr;
		wxWidgetsPtr<wxButton>    _back             = nullptr;
	};
}
