// SD Mod Manager

// Copyright (c) 2026 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "plugin_list_view.hpp"

#include "application.h"
#include "interface/iicon_storage.hpp"
#include "interface/imod_data_provider.hpp"
#include "interface/imod_manager.hpp"
#include "interface/iplugin_manager.hpp"
#include "plugin_list_model.hpp"
#include "utility/sdlexcept.h"
#include "wx/priority_data_renderer.h"

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

PluginListView::PluginListView(wxWindow* parent, IPluginManager& pluginManager, IModManager& modManager,
	IModDataProvider& modDataProvider, IIconStorage& iconStorage)
	: wxPanel(parent, wxID_ANY)
	, _manager(pluginManager)
	, _modManager(modManager)
	, _listModel(new PluginListModel(modDataProvider, iconStorage))
	, _iconStorage(iconStorage)
{
	createControls();
	_listModel->modList(_modManager.mods());
	_listModel->setList(_manager.plugins());
	buildLayout();
	bindEvents();
	updateControlsState();
}

void PluginListView::buildLayout()
{
	auto leftVertSize = new wxBoxSizer(wxVERTICAL);
	leftVertSize->Add(_list, wxSizerFlags(1).Expand().Border(wxALL, 4));
	leftVertSize->Add(_showHidden, wxSizerFlags(0).Border(wxALL, 4));
	leftVertSize->Add(_showFromAllMods, wxSizerFlags(0).Border(wxALL, 4));

	auto buttonSizer = new wxBoxSizer(wxVERTICAL);
	buttonSizer->Add(_enable, wxSizerFlags(0).Expand().Border(wxALL, 4));
	buttonSizer->Add(_disable, wxSizerFlags(0).Expand().Border(wxALL, 4));
	buttonSizer->Add(_reset, wxSizerFlags(0).Expand().Border(wxALL, 4));

	auto leftGroupSizer = new wxStaticBoxSizer(_group, wxHORIZONTAL);
	leftGroupSizer->Add(buttonSizer, wxSizerFlags(0).Expand());
	leftGroupSizer->Add(leftVertSize, wxSizerFlags(1).Expand());

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

	// _list->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, [=](wxDataViewEvent&) { onSwitchSelectedStateRequested(); });

	_manager.onListChanged().connect([this] {
		_listModel->setList(_manager.plugins());
		followSelection();
		updateControlsState();
	});

	_modManager.onListChanged().connect([this] {
		_listModel->modList(_modManager.mods());
		followSelection();
		updateControlsState();
	});

	_enable->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { onEnableRequested(); });
	_disable->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { onDisableRequested(); });
	_reset->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { onResetRequested(); });
	_showHidden->Bind(wxEVT_CHECKBOX, [=](wxCommandEvent&) { _listModel->showHidden(_showHidden->IsChecked()); });
	_showFromAllMods->Bind(
		wxEVT_CHECKBOX, [=](wxCommandEvent&) { _listModel->showFromAllMods(_showFromAllMods->IsChecked()); });
}

void PluginListView::createControls()
{
	_group = new wxStaticBox(this, wxID_ANY, "column/plugins"_lng);

	createListControl();

	_enable  = new wxButton(_group, wxID_ANY, "dialog/button/enable"_lng);
	_disable = new wxButton(_group, wxID_ANY, "dialog/button/disable"_lng);
	_reset   = new wxButton(_group, wxID_ANY, "dialog/button/reset"_lng);

	_enable->SetBitmap(_iconStorage.get(Icon::Stock::checkmark, {}, *wxGREEN));
	_disable->SetBitmap(_iconStorage.get(Icon::Stock::cross, {}, *wxLIGHT_GREY));
	_reset->SetBitmap(_iconStorage.get(Icon::Stock::box));

	_showHidden      = new wxCheckBox(_group, wxID_ANY, "dialog/label/show_hidden"_lng);
	_showFromAllMods = new wxCheckBox(_group, wxID_ANY, "dialog/label/show_from_all_mods"_lng);
}

void PluginListView::createListControl()
{
	_list = new wxDataViewCtrl(
		_group, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_ROW_LINES | wxDV_VERT_RULES);
	_list->AssociateModel(_listModel.get());

	createListColumns();
}

void PluginListView::createListColumns()
{
	auto r0 = new mmPriorityDataRenderer(FromDIP(32));
	auto r1 = new wxDataViewTextRenderer();
	auto r2 = new wxDataViewIconTextRenderer();

	r0->SetAlignment(wxALIGN_CENTER_VERTICAL);
	r1->SetAlignment(wxALIGN_CENTER_VERTICAL);

	constexpr auto columnFlags =
		wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_REORDERABLE;

	auto column0 = new wxDataViewColumn(L"", r0, static_cast<unsigned int>(PluginListModel::Column::state),
		wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER, columnFlags);

	auto column1 =
		new wxDataViewColumn("column/plugin"_lng, r1, static_cast<unsigned int>(PluginListModel::Column::caption),
			wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER, columnFlags);
	auto column2 =
		new wxDataViewColumn("column/name"_lng, r2, static_cast<unsigned int>(PluginListModel::Column::mod),
			wxCOL_WIDTH_AUTOSIZE, wxALIGN_LEFT, columnFlags);

	_list->AppendColumn(column0);
	_list->AppendColumn(column1);
	_list->AppendColumn(column2);

	column1->SetSortOrder(true);
}

void PluginListView::updateControlsState()
{
	// wxLogDebug(__FUNCTION__);

	if (!_selected) // FIXME: fix this shit
	{
		_enable->Disable();
		_disable->Disable();
		_reset->Disable();

		return;
	}

	_enable->Enable();
	_disable->Enable();

 	if (_manager.plugins().managed.contains(_selected->id))
 		_reset->Enable();
}

void PluginListView::followSelection()
{
	// wxLogDebug(__FUNCTION__);
	if (!_selected)
		return;

	const auto itemToSelect = _listModel->findItemById(*_selected);

	if (itemToSelect.IsOk())
	{
		_list->EnsureVisible(itemToSelect);
		_list->Select(itemToSelect);
	}
	else
	{
		_selected = {};
	}
}

void PluginListView::onEnableRequested()
{
	EX_TRY;

	if (!_selected)
		return;

	_manager.enable(_selected->id);

	EX_UNEXPECTED;
}

void PluginListView::onDisableRequested()
{
	EX_TRY;

	if (!_selected)
		return;

	_manager.disable(_selected->id);

	EX_UNEXPECTED;
}

void PluginListView::onResetRequested()
{
	EX_TRY;

	if (!_selected)
		return;

	_manager.reset(_selected->id);

	EX_UNEXPECTED;
}
;