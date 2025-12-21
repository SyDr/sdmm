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
#include "type/icon.hpp"
#include "type/filesystem.hpp"

using namespace mm;

SelectExe::SelectExe(wxWindow* parent, const fs::path& basePath,
	const wxString& initiallySelectedFile, IIconStorage& iconStorage)
	: wxDialog(parent, wxID_ANY, "dialog/select_executable/caption"_lng, wxDefaultPosition, wxSize(300, 450))
	, _basePath(basePath)
	, _selectedFile(initiallySelectedFile)
	, _iconStorage(iconStorage)
{
	this->SetSizeHints(wxSize(300, 450), wxDefaultSize);

	_imageList = std::make_unique<wxImageList>(16, 16, true);

	auto mainPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);

	_list = new wxListView(
		mainPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_SMALL_ICON | wxLC_SINGLE_SEL);
	_list->SetImageList(_imageList.get(), wxIMAGE_LIST_SMALL);

	wxBusyCursor busy;
	refreshListContent();

	auto buttonOk = new wxButton(mainPanel, wxID_ANY, "dialog/button/ok"_lng);
	buttonOk->Enable(!_selectedFile.empty());

	auto bHorizontal = new wxBoxSizer(wxHORIZONTAL);
	bHorizontal->AddStretchSpacer(1);
	bHorizontal->Add(buttonOk, wxSizerFlags(0).Expand().Border(wxALL, 4));

	auto bVertical = new wxBoxSizer(wxVERTICAL);
	bVertical->Add(_list, wxSizerFlags(1).Expand().Border(wxALL, 4));
	bVertical->Add(bHorizontal, wxSizerFlags(0).Expand().Border(wxALL, 4));

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

	mainPanel->SetSizer(bVertical);

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

	wxLogNull noLogging;  // suppress wxWidgets messages about inability to load icon

	using di = fs::directory_iterator;
	for (auto it = di(_basePath), end = di(); it != end; ++it)
	{
		if (!is_regular_file(it->path()) || it->path().extension() != ".exe")
			continue;

		int index = _list->GetItemCount();
		_list->InsertItem(index, it->path().filename().wstring());

		_imageList->Add(_iconStorage.get(it->path().string(), Icon::Size::x16));
		_list->SetItemImage(index, index);

		if (it->path().filename().wstring() == _selectedFile)
			_list->Select(index);
	}

	_list->SetColumnWidth(0, wxLIST_AUTOSIZE);
	_list->Thaw();
}
