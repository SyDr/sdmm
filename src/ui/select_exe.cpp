// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "select_exe.h"

#include <boost/algorithm/string.hpp>
#include <wx/dir.h>
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/imaglist.h>
#include <wx/checkbox.h>
#include <wx/button.h>

#include "interface/iicon_storage.h"
#include "application.h"
#include "type/embedded_icon.h"

using namespace mm;

SelectExe::SelectExe(wxWindow *parent, const std::filesystem::path& basePath, const wxString& initiallySelectedFile,
	IIconStorage& iconStorage)
	: wxDialog(parent, wxID_ANY, "Select executable"_lng, wxDefaultPosition, wxSize(200, 324))
	, _basePath(basePath)
	, _selectedFile(initiallySelectedFile)
	, _iconStorage(iconStorage)
{
	SetIcon(_iconStorage.get(embedded_icon::main_icon));
	this->SetSizeHints(wxSize(200, 324), wxDefaultSize);

	_imageList = std::make_unique<wxImageList>(16, 16, true);

	wxPanel* mainPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);

	wxBoxSizer* bVertical = new wxBoxSizer(wxVERTICAL);
	mainPanel->SetSizer(bVertical);

	_list = new wxListView(mainPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_SMALL_ICON | wxLC_SINGLE_SEL);
	_list->SetImageList(_imageList.get(), wxIMAGE_LIST_SMALL);

	wxBusyCursor busy;
	refreshListContent();

	bVertical->Add(_list, 1, wxEXPAND | static_cast<wxStretch>(wxALL), 5);

	wxBoxSizer* bHorizontal = new wxBoxSizer(wxHORIZONTAL);
	bVertical->Add(bHorizontal, 0, wxSTRETCH_NOT, 5);

	wxButton* buttonOk = new wxButton(mainPanel, wxID_ANY, "Ok"_lng);
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

	_list->Bind(wxEVT_LIST_ITEM_ACTIVATED, [=](wxListEvent&) {
		this->EndModal(wxID_OK);
	});

	buttonOk->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) {
		this->EndModal(wxID_OK);
	});

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
	const auto files = getFileList();
	_list->Freeze();
	_list->DeleteAllItems();
	_imageList->RemoveAll();

	for (const auto& item : files)
	{
		int index = _list->GetItemCount();
		_list->InsertItem(index, item);

		_imageList->Add(_iconStorage.get(_basePath.string() + L"/" + item));
		_list->SetItemImage(index, index);

		if (item == _selectedFile)
			_list->Select(index);
	}

	_list->SetColumnWidth(0, wxLIST_AUTOSIZE);
	_list->Thaw();
}

std::vector<wxString> SelectExe::getFileList()
{
	std::vector<wxString> result;

	wxDir dir(_basePath.string());
	if (!dir.IsOpened())
		return result;

	wxString tmp;
	if (dir.GetFirst(&tmp, "*.exe", wxDIR_FILES | wxDIR_HIDDEN))
	{
		result.push_back(tmp);
		while (dir.GetNext(&tmp))
			result.push_back(tmp);
	}

	dir.Close();

	return result;
}
