// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "show_file_list_dialog.hpp"

#include "application.h"
#include "interface/iapp_config.hpp"
#include "interface/iicon_storage.hpp"
#include "type/embedded_icon.h"
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

ShowFileListDialog::ShowFileListDialog(wxWindow* parent, IIconStorage& iconStorage,
									   IModDataProvider& dataProvider, Era2DirectoryStructure data)
	: wxDialog(parent, wxID_ANY, "Mod file list"_lng, wxDefaultPosition, wxSize(800, 444),
			   wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
	, _data(std::move(data))
	, _listModel(new ModListModel(dataProvider, iconStorage))
{
	createControls();
	buildLayout();
	fillData();
	bindEvents();

	ModList list;

	for (const auto& item : _data.modList)
		list.data.emplace_back(item, ModList::ModState::active);

	_listModel->setModList(list);
}

void ShowFileListDialog::createControls()
{
	_list = new wxDataViewCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
							   wxDV_ROW_LINES | wxDV_VERT_RULES);
	_list->AssociateModel(_listModel.get());

	createListColumns();

	_fileList = new wxDataViewListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
									   wxDV_HORIZ_RULES | wxDV_VERT_RULES | wxDV_ROW_LINES);
	_fileList->AppendTextColumn(
		"Index"_lng, wxDATAVIEW_CELL_INERT, 30);

	_fileList->AppendTextColumn(
		"Path"_lng, wxDATAVIEW_CELL_INERT,
		std::max(200l, 400l - static_cast<long>(40l * _data.modList.size())));

	for (size_t i = 0; i < _data.modList.size(); ++i)
		_fileList->AppendTextColumn(std::to_wstring(i), wxDATAVIEW_CELL_INERT, 40);
}

void ShowFileListDialog::createListColumns()
{
	auto r0 = new mmPriorityDataRenderer();
	auto r1 = new wxDataViewIconTextRenderer();

	r0->SetAlignment(wxALIGN_CENTER_VERTICAL);
	r1->SetAlignment(wxALIGN_CENTER_VERTICAL);

	auto column0 =
		new wxDataViewColumn(L" ", r0, static_cast<unsigned int>(ModListModel::Column::priority),
							 wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER);
	auto column1= new wxDataViewColumn("Mod"_lng, r1,
										static_cast<unsigned int>(ModListModel::Column::name),
										wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER);

	_list->AppendColumn(column0);
	_list->AppendColumn(column1);

	column0->SetSortOrder(true);
}


void ShowFileListDialog::bindEvents() {}

void ShowFileListDialog::buildLayout()
{
	auto mainSizer = new wxBoxSizer(wxHORIZONTAL);
	mainSizer->Add(_list, wxSizerFlags(2).Expand().Border(wxALL, 4));
	mainSizer->Add(_fileList, wxSizerFlags(3).Expand().Border(wxALL, 4));

	this->SetSizer(mainSizer);
}

void ShowFileListDialog::fillData()
{
	_fileList->DeleteAllItems();

	size_t index = 0;
	for (size_t i = 0; i < _data.fileList.size(); ++i)
	{
		const auto& path    = _data.fileList[i];
		const auto& modList = _data.entries[i];

		wxVector<wxVariant> data;
		data.push_back(wxVariant(wxString(std::to_wstring(++index))));
		data.push_back(wxVariant(wxString::FromUTF8(path.string())));
		for (const auto& mod : modList)
		{
			wxString item;

			if (mod.raw)
				item.Append('F');

			if (mod.lod)
				item.Append('A');

			data.push_back(wxVariant(item));
		}

		_fileList->AppendItem(data);
	}
}
