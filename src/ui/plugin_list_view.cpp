// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "plugin_list_view.hpp"

#include "application.h"
#include "base/mod_conflict_resolver.hpp"
#include "domain/imod_data_provider.hpp"
#include "domain/iplugin_manager.hpp"
#include "domain/ipreset_manager.hpp"
#include "domain/mod_data.hpp"
#include "interface/domain/ilocal_config.h"
#include "interface/imod_manager.hpp"
#include "interface/imod_platform.hpp"
#include "interface/iapp_config.h"
#include "interface/service/iicon_storage.h"
#include "manage_preset_list_view.hpp"
#include "plugin_list_model.hpp"
#include "select_exe.h"
#include "types/embedded_icon.h"
#include "utility/sdlexcept.h"
#include "utility/shell_util.h"
#include "wx/priority_data_renderer.h"

#include <fmt/format.h>
#include <wx/app.h>
#include <wx/bmpcbox.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/collpane.h>
#include <wx/dataview.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>

#include <algorithm>

using namespace mm;

PluginListView::PluginListView(wxWindow* parent, IPluginManager& pluginManager,
							   IModDataProvider& modDataProvider, IIconStorage& iconStorage)
	: wxPanel(parent, wxID_ANY)
	, _manager(pluginManager)
	, _listModel(new PluginListModel(modDataProvider, iconStorage))
	, _iconStorage(iconStorage)
{
	createControls();
	_listModel->setList(_manager.plugins());
	buildLayout();
	bindEvents();
	updateControlsState();
}

void PluginListView::buildLayout()
{
	auto leftVertSize = new wxBoxSizer(wxVERTICAL);
	leftVertSize->Add(_list, wxSizerFlags(1).Expand().Border(wxALL, 4));
	leftVertSize->Add(_showAll, wxSizerFlags(0).Border(wxALL, 4));

	auto buttonSizer = new wxBoxSizer(wxVERTICAL);
	buttonSizer->Add(_changeState, wxSizerFlags(0).Expand().Border(wxALL, 4));

	auto leftGroupSizer = new wxStaticBoxSizer(_group, wxHORIZONTAL);
	leftGroupSizer->Add(leftVertSize, wxSizerFlags(1).Expand());
	leftGroupSizer->Add(buttonSizer, wxSizerFlags(0).Expand());

	auto mainSizer = new wxBoxSizer(wxVERTICAL);

	mainSizer->Add(leftGroupSizer, wxSizerFlags(1).Expand());

	this->SetSizer(mainSizer);
}

void PluginListView::bindEvents()
{
	_list->Bind(wxEVT_DATAVIEW_COLUMN_SORTED, [=](wxDataViewEvent&) { followSelection(); });

	_list->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, [=](wxDataViewEvent&) {
		_selected = _listModel->findIdByItem(_list->GetSelection());
		updateControlsState();
	});

	_list->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, [=](wxDataViewEvent&) { onSwitchSelectedStateRequested(); });

	_manager.onListChanged().connect([this] {
		_listModel->setList(_manager.plugins());
		followSelection();
		updateControlsState();
	});

	_changeState->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { onSwitchSelectedStateRequested(); });
	_showAll->Bind(wxEVT_CHECKBOX, [=](wxCommandEvent&) { _listModel->showAll(_showAll->IsChecked()); });
}

void PluginListView::createControls()
{
	_group = new wxStaticBox(this, wxID_ANY, "Plugins"_lng);

	createListControl();

	_changeState = new wxButton(_group, wxID_ANY, "Enable"_lng);
	_changeState->SetBitmap(_iconStorage.get(embedded_icon::plus));

	_showAll = new wxCheckBox(_group, wxID_ANY, "Show all"_lng);
}

void PluginListView::createListControl()
{
	_list = new wxDataViewCtrl(_group, wxID_ANY, wxDefaultPosition, wxDefaultSize,
							   wxDV_ROW_LINES | wxDV_VERT_RULES);
	_list->AssociateModel(_listModel.get());

	createListColumns();
}

void PluginListView::createListColumns()
{
	auto r0 = new mmPriorityDataRenderer();
	auto r1 = new wxDataViewTextRenderer();
	auto r2 = new wxDataViewIconTextRenderer();

	r0->SetAlignment(wxALIGN_CENTER_VERTICAL);
	r1->SetAlignment(wxALIGN_CENTER_VERTICAL);

	constexpr auto columnFlags =
		wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_REORDERABLE;

	auto column0 = new wxDataViewColumn("", r0, static_cast<unsigned int>(PluginListModel::Column::state),
										wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER, columnFlags);

	auto column1 =
		new wxDataViewColumn("Plugin"_lng, r1, static_cast<unsigned int>(PluginListModel::Column::caption),
							 wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER, columnFlags);
	auto column2 =
		new wxDataViewColumn("Mod"_lng, r2, static_cast<unsigned int>(PluginListModel::Column::mod),
							 wxCOL_WIDTH_AUTOSIZE, wxALIGN_LEFT, columnFlags);

	_list->AppendColumn(column0);
	_list->AppendColumn(column1);
	_list->AppendColumn(column2);

	column1->SetSortOrder(true);
}

void PluginListView::updateControlsState()
{
	wxLogDebug(__FUNCTION__);

	if (_selected.empty())
	{
		_changeState->Disable();

		return;
	}

	const bool isActive = [&] {
		if (auto state = _manager.plugins().overriddenState(_selected))
			return state == PluginState::enabled;

		if (auto state = _manager.plugins().defaultState(_selected))
			return state->state == PluginState::enabled;

		return false;
	}();

	_changeState->Enable();
	_changeState->SetBitmap(wxNullBitmap);
	_changeState->SetBitmap(_iconStorage.get(isActive ? embedded_icon::minus : embedded_icon::plus));
	_changeState->SetLabelText(isActive ? "Disable"_lng : "Enable"_lng);
}

void PluginListView::followSelection()
{
	wxLogDebug(__FUNCTION__);

	const auto itemToSelect = _listModel->findItemById(_selected);

	if (itemToSelect.IsOk())
	{
		_list->EnsureVisible(itemToSelect);
		_list->Select(itemToSelect);
	}
	else
	{
		_selected = wxEmptyString;
	}
}

void PluginListView::onSwitchSelectedStateRequested()
{
	try_handle_exceptions(this, [&] {
		_manager.switchState(_selected);
		_listModel->ItemChanged(_listModel->findItemById(_selected));
	});
}
