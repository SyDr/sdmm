// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "select_mods_dialog.hpp"

#include "application.h"
#include "domain/mod_data.hpp"
#include "interface/iapp_config.hpp"
#include "interface/iicon_storage.hpp"
#include "interface/imod_data_provider.hpp"
#include "interface/imod_manager.hpp"
#include "interface/imod_platform.hpp"
#include "mod_list_model.h"
#include "type/embedded_icon.h"
#include "utility/sdlexcept.h"
#include "wx/data_view_multiple_icons_renderer.h"
#include "wx/priority_data_renderer.h"

#include <boost/algorithm/string.hpp>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/dataview.h>
#include <wx/dir.h>
#include <wx/dirctrl.h>
#include <wx/imaglist.h>
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>

#include <algorithm>

using namespace mm;

SelectModsDialog::SelectModsDialog(
	wxWindow& parent, IIconStorage& iconStorage, IModDataProvider& dataProvider, ModList list)
	: wxDialog(&parent, wxID_ANY, "Select mods"_lng, wxDefaultPosition, wxSize(800, 444),
		  wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
	, _iconStorage(iconStorage)
	, _listModel(new ModListModel(dataProvider, iconStorage, true))
	, _mods(std::move(list))
{
	createControls();
	buildLayout();
	bindEvents();

	_listModel->setModList(_mods);
	_listModel->setChecked({ _mods.active.cbegin(), _mods.active.cend() });
}

std::unordered_set<std::string> const& SelectModsDialog::getSelected() const
{
	return _listModel->getChecked();
}

void SelectModsDialog::createControls()
{
	_continue = new wxButton(this, wxID_OK, "Continue"_lng);
	_cancel   = new wxButton(this, wxID_CANCEL, "Cancel"_lng);

	createListControl();
}

void SelectModsDialog::createListControl()
{
	_list = new wxDataViewCtrl(
		this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_ROW_LINES | wxDV_VERT_RULES);
	_list->AssociateModel(_listModel.get());

	createListColumns();
}

void SelectModsDialog::createListColumns()
{
	auto r0 =
		new wxDataViewToggleRenderer(wxDataViewToggleRenderer::GetDefaultType(), wxDATAVIEW_CELL_ACTIVATABLE);
	auto r1 = new mmPriorityDataRenderer();
	auto r2 = new wxDataViewIconTextRenderer();

	r1->SetAlignment(wxALIGN_CENTER_VERTICAL);
	r2->SetAlignment(wxALIGN_CENTER_VERTICAL);

	auto column0 = new wxDataViewColumn(L" ", r0, static_cast<unsigned int>(ModListModel::Column::checkbox),
		wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER);
	auto column1 = new wxDataViewColumn(L" ", r1, static_cast<unsigned int>(ModListModel::Column::priority),
		wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER);
	auto column2 = new wxDataViewColumn("Mod"_lng, r2,
		static_cast<unsigned int>(ModListModel::Column::caption), wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER);

	_list->AppendColumn(column0);
	_list->AppendColumn(column1);
	_list->AppendColumn(column2);

	column1->SetSortOrder(true);
}

void SelectModsDialog::bindEvents()
{
	_continue->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { EndModal(wxID_OK); });
	_cancel->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { EndModal(wxID_CANCEL); });
}

void SelectModsDialog::buildLayout()
{
	auto topSizer = new wxBoxSizer(wxHORIZONTAL);
	topSizer->Add(_list, wxSizerFlags(1).Expand().Border(wxALL, 4));

	auto buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	buttonSizer->AddStretchSpacer();
	buttonSizer->Add(_continue, wxSizerFlags(0).Expand().Border(wxALL, 4));
	buttonSizer->Add(_cancel, wxSizerFlags(0).Expand().Border(wxALL, 4));

	auto mainSizer = new wxBoxSizer(wxVERTICAL);
	mainSizer->Add(topSizer, wxSizerFlags(1).Expand());
	mainSizer->Add(buttonSizer, wxSizerFlags(0).Expand());

	this->SetSizer(mainSizer);
}
