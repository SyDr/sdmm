// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "select_mod_pairs_dialog.hpp"

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

SelectModPairsDialog::SelectModPairsDialog(wxWindow& parent, Application& application,
	IModDataProvider& dataProvider, std::vector<std::pair<wxString, wxString>> values)
	: wxDialog(&parent, wxID_ANY, "Select mod pairs to potentially remove from result"_lng, wxDefaultPosition,
		  wxSize(800, 444), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
	, _iconStorage(application.iconStorage())
	, _modDataProvider(dataProvider)
	, _values(std::move(values))
{
	SetIcon(_iconStorage.get(embedded_icon::main_icon));

	createControls();
	buildLayout();
	fillData();
	bindEvents();
}

std::set<std::pair<wxString, wxString>> SelectModPairsDialog::getSelected() const
{
	std::set<std::pair<wxString, wxString>> result;

	for (size_t i = 0; i < _values.size(); ++i)
	{
		wxVariant value;
		_list->GetValue(value, i, 0);

		if (value.GetBool())
		{
			result.emplace(_values[i]);
			result.emplace(_values[i].second, _values[i].first);
		}
	}

	return result;
}

void SelectModPairsDialog::createControls()
{
	_continue = new wxButton(this, wxID_OK, "Continue"_lng);
	_cancel   = new wxButton(this, wxID_CANCEL, "Cancel"_lng);

	createListControl();
}

void SelectModPairsDialog::createListControl()
{
	_list = new wxDataViewListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxDV_HORIZ_RULES | wxDV_VERT_RULES | wxDV_ROW_LINES);

	_list->AppendToggleColumn(L"", wxDATAVIEW_CELL_ACTIVATABLE, wxCOL_WIDTH_AUTOSIZE);
	_list
		->AppendIconTextColumn("Mod"_lng + L" 1", wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_LEFT,
			wxDATAVIEW_COL_SORTABLE)
		->SetSortOrder(true);
	_list->AppendIconTextColumn(
		"Mod"_lng + L" 2", wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_LEFT, wxDATAVIEW_COL_SORTABLE);
}

void SelectModPairsDialog::bindEvents()
{
	_continue->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { EndModal(wxID_OK); });
	_cancel->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { EndModal(wxID_CANCEL); });
}

void SelectModPairsDialog::buildLayout()
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

void SelectModPairsDialog::fillData()
{
	_list->DeleteAllItems();

	for (size_t i = 0; i < _values.size(); ++i)
	{
		const auto& [mod1, mod2] = _values[i];

		wxVector<wxVariant> data;
		data.push_back(wxVariant(false));
		for (const auto& item : { mod1, mod2 })
		{
			wxIcon     icon;
			const auto mod = _modDataProvider.modData(item);

			if (!mod.icon_filename.empty())
				icon = _iconStorage.get((mod.data_path / mod.icon_filename).string());
			else
				icon = _iconStorage.get(embedded_icon::folder);

			data.push_back(wxVariant(wxDataViewIconText(mod.caption, icon)));
		}

		_list->AppendItem(data);
	}
}
