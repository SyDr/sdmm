// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "select_exe.h"

#include <boost/algorithm/string.hpp>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/dir.h>
#include <wx/imaglist.h>
#include <wx/listctrl.h>
#include <wx/sizer.h>

#include "application.h"
#include "interface/iicon_storage.hpp"
#include "type/embedded_icon.h"
#include "type/filesystem.hpp"

using namespace mm;

SelectExe::SelectExe(wxWindow* parent, const fs::path& basePath,
	const wxString& initiallySelectedFile, IIconStorage& iconStorage)
	: wxDialog(parent, wxID_ANY, "Select executable"_lng, wxDefaultPosition, wxSize(200, 324))
	, _basePath(basePath)
	, _selectedFile(initiallySelectedFile)
	, _iconStorage(iconStorage)
{
	this->SetSizeHints(wxSize(200, 324), wxDefaultSize);

	_imageList = std::make_unique<wxImageList>(16, 16, true);

	wxPanel* mainPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);

	wxBoxSizer* bVertical = new wxBoxSizer(wxVERTICAL);
	mainPanel->SetSizer(bVertical);

	_list = new wxListView(
		mainPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_SMALL_ICON | wxLC_SINGLE_SEL);
	_list->SetImageList(_imageList.get(), wxIMAGE_LIST_SMALL);

	wxBusyCursor busy;
	refreshListContent();

	bVertical->Add(_list, 1, wxEXPAND | static_cast<wxStretch>(wxALL), 5);

	wxBoxSizer* bHorizontal = new wxBoxSizer(wxHORIZONTAL);
	bVertical->Add(bHorizontal, 0, wxSTRETCH_NOT, 5);

	wxButton* buttonOk = new wxButton(mainPanel, wxID_ANY, "OK"_lng);
	buttonOk->Enable(!_selectedFile.empty());

	bHorizontal->AddStretchSpacer(1);
	bHorizontal->Add(buttonOk, 1, wxALL, 5);

	_list->Bind(wxEVT_LIST_ITEM_SELECTED, [=](wxListEvent&) {
		_selectedFile = _list->GetItemText(_list->GetFirstSelected());
		buttonOk->Enable();
	});

	_list->Bind(wxEVT_LIST_ITEM_DESELECTED, [=](wxListEvent&) {
		_selectedFile.clear();
		buttonOk->Disable();
	});

	_list->Bind(wxEVT_LIST_ITEM_ACTIVATED, [=](wxListEvent&) { this->EndModal(wxID_OK); });

	buttonOk->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { this->EndModal(wxID_OK); });

	wxBoxSizer* bSizerMain = new wxBoxSizer(wxHORIZONTAL);
	bSizerMain->Add(mainPanel, 1, wxEXPAND | static_cast<wxStretch>(wxALL));
	this->SetSizer(bSizerMain);
	this->Layout();
}

wxString SelectExe::getSelectedFile() const
{
	return _selectedFile;
}

void SelectExe::refreshListContent()
{
	_list->Freeze();
	_list->DeleteAllItems();
	_imageList->RemoveAll();

	using di = fs::directory_iterator;
	for (auto it = di(_basePath), end = di(); it != end; ++it)
	{
		if (!is_regular_file(it->path()) || it->path().extension() != ".exe")
			continue;

		int index = _list->GetItemCount();
		_list->InsertItem(index, it->path().filename().wstring());

		_imageList->Add(_iconStorage.get(it->path().string()));
		_list->SetItemImage(index, index);

		if (it->path().filename().wstring() == _selectedFile)
			_list->Select(index);
	}

	_list->SetColumnWidth(0, wxLIST_AUTOSIZE);
	_list->Thaw();
}
