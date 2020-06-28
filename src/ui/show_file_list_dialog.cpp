// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "show_file_list_dialog.hpp"

#include "application.h"
#include "interface/service/iapp_config.h"
#include "interface/service/iicon_storage.h"
#include "types/embedded_icon.h"
#include "utility/sdlexcept.h"
#include "mod_list_model.h"
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

ShowFileListDialog::ShowFileListDialog(wxWindow* parent, Application& application,
									   IModDataProvider& dataProvider, Era2DirectoryStructure data)
	: wxDialog(parent, wxID_ANY, "Mod file list"_lng, wxDefaultPosition, wxSize(800, 444),
			   wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
	, _data(std::move(data))
	, _listModel(new ModListModel(dataProvider, application.iconStorage(), true))
{
	SetIcon(application.iconStorage().get(embedded_icon::main_icon));

	createControls();
	buildLayout();
	fillData();
	bindEvents();

	ModList list;
	list.active = _data.modList;

	_listModel->setModList(list);
}

void ShowFileListDialog::createControls()
{
	_mods = new wxDataViewCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
							   wxDV_ROW_LINES | wxDV_VERT_RULES);
	_mods->AssociateModel(_listModel.get());

	createListColumns();

	_fileList = new wxDataViewListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
									   wxDV_HORIZ_RULES | wxDV_VERT_RULES | wxDV_ROW_LINES);
	_fileList->AppendTextColumn(
		"Path", wxDATAVIEW_CELL_INERT,
		std::max(200l, 400l - static_cast<long>(40l * _data.modList.size())));

	for (size_t i = 0; i < _data.modList.size(); ++i)
		_fileList->AppendTextColumn(std::to_string(i), wxDATAVIEW_CELL_INERT, 40);
}

void ShowFileListDialog::createListColumns()
{
	auto r0 = new mmPriorityDataRenderer();
	auto r1 = new wxDataViewIconTextRenderer();

	r0->SetAlignment(wxALIGN_CENTER_VERTICAL);
	r1->SetAlignment(wxALIGN_CENTER_VERTICAL);

	auto column0 =
		new wxDataViewColumn(" ", r0, static_cast<unsigned int>(ModListModel::Column::priority),
							 wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER);
	auto column1= new wxDataViewColumn("Mod"_lng, r1,
										static_cast<unsigned int>(ModListModel::Column::caption),
										wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER);

	_mods->AppendColumn(column0);
	_mods->AppendColumn(column1);

	column0->SetSortOrder(true);
}


void ShowFileListDialog::bindEvents() {}

void ShowFileListDialog::buildLayout()
{
	auto mainSizer = new wxBoxSizer(wxHORIZONTAL);
	mainSizer->Add(_mods, wxSizerFlags(2).Expand().Border(wxALL, 4));
	mainSizer->Add(_fileList, wxSizerFlags(3).Expand().Border(wxALL, 4));

	this->SetSizer(mainSizer);
}

void ShowFileListDialog::fillData()
{
	_fileList->DeleteAllItems();

	for (size_t i = 0; i < _data.fileList.size(); ++i)
	{
		auto const& path    = _data.fileList[i];
		auto const& modList = _data.entries[i];

		std::vector<wxVariant> data;
		data.emplace_back(wxVariant(wxString(path)));
		for (auto const& mod : modList)
		{
			wxString item;

			if (mod.raw)
				item.Append('F');

			if (mod.lod)
				item.Append('A');

			data.emplace_back(wxVariant(item));
		}

		_fileList->AppendItem(data);
	}
}
