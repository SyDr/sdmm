// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "select_directory_view.h"

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

#include "application.h"
#include "interface/iapp_config.hpp"
#include "interface/iicon_storage.hpp"
#include "type/icon.hpp"
#include "utility/sdlexcept.h"

using namespace mm;

SelectDirectoryDialog::SelectDirectoryDialog(wxWindow* parent, IAppConfig& config, IIconStorage& iconStorage)
	: wxDialog(
		  parent, wxID_ANY, "dialog/select_game_directory/caption"_lng, wxDefaultPosition, wxSize(1000, 666))
	, _appConfig(config)
	, _iconStorage(iconStorage)
{
	createControls();
	buildLayout();
	fillData();
	bindEvents();
}

wxString SelectDirectoryDialog::getSelectedPath() const
{
	return _selectedPathEdit->GetValue();
}

void SelectDirectoryDialog::createControls()
{
	auto selectedPath = _appConfig.getDataPath();
	auto dirToSelect  = selectedPath.empty() ? wxString::FromUTF8(wxDirDialogDefaultFolderStr) : wxString(selectedPath.wstring());

	_explorerList = new wxGenericDirCtrl(this, wxID_ANY, dirToSelect, wxDefaultPosition, wxDefaultSize,
		wxDIRCTRL_DIR_ONLY | wxDIRCTRL_3D_INTERNAL);

	_recentDirsList = new wxDataViewListCtrl(
		this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_HORIZ_RULES | wxDV_ROW_LINES | wxDV_NO_HEADER);
	_recentDirsList->AppendIconTextColumn("column/path"_lng);

	_selectedLabel    = new wxStaticText(this, wxID_ANY, "dialog/label/selected"_lng);
	_selectedPathEdit =
		new wxTextCtrl(this, wxID_ANY, dirToSelect, wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	_accept = new wxButton(this, wxID_ANY, "dialog/button/accept"_lng);

	_menu.starUnstar     = _menu.menu.Append(wxID_ANY, L"placeholder");
	_menu.removeFromList = _menu.menu.Append(wxID_ANY, "dialog/button/remove_from_list"_lng);
}

void SelectDirectoryDialog::bindEvents()
{
	_explorerList->Bind(wxEVT_DIRCTRL_SELECTIONCHANGED, [=](wxCommandEvent&) {
		const wxString selected = _explorerList->GetPath();
		_selectedPathEdit->SetValue(selected);

		auto pathList = _appConfig.getKnownDataPathList();
		for (size_t i = 0; i < pathList.size(); ++i)
		{
			if (wxString::FromUTF8(pathList[i].string()) == selected)
			{
				_recentDirsList->SelectRow(i);
				return;
			}
		}

		_recentDirsList->UnselectAll();
	});

	_recentDirsList->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, [=](wxCommandEvent&) {
		auto row = _recentDirsList->GetSelectedRow();
		if (row < 0)
			return;

		auto path = wxString::FromUTF8(_appConfig.getKnownDataPathList().at(row).string());
		_selectedPathEdit->SetValue(path);
		_explorerList->SetPath(path);
	});

	_recentDirsList->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, [=](wxCommandEvent&) { EndModal(wxID_OK); });
	_accept->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { EndDialog(wxID_OK); });

	Bind(wxEVT_MENU, &SelectDirectoryDialog::OnMenuItemSelected, this);

	_recentDirsList->Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, [=](wxDataViewEvent& event) {
		const wxDataViewItem item(event.GetItem());
		if (!item.IsOk())
		{
			event.Veto();
			return;
		}

		OnListItemContextMenu();
	});
}

void SelectDirectoryDialog::buildLayout()
{
	auto leftGroupSizer = new wxStaticBoxSizer(wxVERTICAL, this, "dialog/label/explorer"_lng);
	leftGroupSizer->Add(_explorerList, wxSizerFlags(1).Expand().Border(wxALL, 4));

	auto rightGroupSizer = new wxStaticBoxSizer(wxVERTICAL, this, "dialog/label/recent"_lng);
	rightGroupSizer->Add(_recentDirsList, wxSizerFlags(1).Expand().Border(wxALL, 4));

	auto contentSizer = new wxBoxSizer(wxHORIZONTAL);
	contentSizer->Add(leftGroupSizer, wxSizerFlags(1).Expand().Border(wxALL, 4));
	contentSizer->Add(rightGroupSizer, wxSizerFlags(1).Expand().Border(wxALL, 4));

	auto lastLineSizer = new wxBoxSizer(wxHORIZONTAL);
	lastLineSizer->Add(_selectedLabel, wxSizerFlags(0).CenterVertical().Border(wxALL, 4));
	lastLineSizer->Add(_selectedPathEdit, wxSizerFlags(1).CenterVertical().Border(wxALL, 4));
	lastLineSizer->Add(_accept, wxSizerFlags(0).CenterVertical().Border(wxALL, 4));

	auto mainSizer = new wxBoxSizer(wxVERTICAL);
	mainSizer->Add(contentSizer, wxSizerFlags(1).Expand().Border(wxALL, 4));
	mainSizer->Add(lastLineSizer, wxSizerFlags(0).Expand().Border(wxALL, 4));

	this->SetSizer(mainSizer);
}

void SelectDirectoryDialog::fillData()
{
	_recentDirsList->DeleteAllItems();

	for (const auto& value : _appConfig.getKnownDataPathList())
	{
		auto icon = _iconStorage.get(
			_appConfig.dataPathHasStar(value) ? Icon::Stock::bookmark : Icon::Stock::blank,
			Icon::Size::x16);

		wxVector<wxVariant> data;
		data.push_back(wxVariant(wxDataViewIconText(wxString::FromUTF8(value.string()), icon)));

		_recentDirsList->AppendItem(data);
		if (value == getSelectedPath().ToStdString(wxConvUTF8))
			_recentDirsList->SelectRow(_recentDirsList->GetItemCount() - 1);
	}
}

void SelectDirectoryDialog::OnMenuItemSelected(const wxCommandEvent& event)
{
	const auto itemId = event.GetId();

	if (itemId == _menu.removeFromList->GetId())
		removeFromListRequested();
	else if (itemId == _menu.starUnstar->GetId())
		starUnstarRequested();
}

void SelectDirectoryDialog::removeFromListRequested()
{
	auto row = _recentDirsList->GetSelectedRow();
	if (row < 0)
		return;

	EX_TRY;
	auto path = _appConfig.getKnownDataPathList().at(row);

	_appConfig.forgetDataPath(path);
	fillData();

	EX_UNEXPECTED;
}

void SelectDirectoryDialog::starUnstarRequested()
{
	auto row = _recentDirsList->GetSelectedRow();
	if (row < 0)
		return;

	EX_TRY;

	auto path = _appConfig.getKnownDataPathList().at(row);
	_appConfig.starDataPath(path, !_appConfig.dataPathHasStar(path));
	fillData();

	EX_UNEXPECTED;
}

void SelectDirectoryDialog::OnListItemContextMenu()
{
	auto row = _recentDirsList->GetSelectedRow();
	if (row < 0)
		return;

	EX_TRY;

	auto path = _appConfig.getKnownDataPathList().at(row);
	_menu.starUnstar->SetItemLabel(
		!_appConfig.dataPathHasStar(path) ? "dialog/button/add_to_favs"_lng : "dialog/button/remove_from_favs"_lng);

	_recentDirsList->PopupMenu(&_menu.menu);
	EX_UNEXPECTED;
}
