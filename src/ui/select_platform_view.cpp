// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "select_platform_view.h"

#include <boost/algorithm/string.hpp>
#include <wx/dir.h>
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/imaglist.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/statbox.h>

#include "application.h"
#include "interface/service/iplatform_service.h"
#include "interface/domain/iplatform_descriptor.h"

using namespace mm;

SelectPlatformView::SelectPlatformView(wxWindow *parent)
	: wxDialog(parent, wxID_ANY, "select_platform/caption"_lng, wxDefaultPosition, wxSize(400, 444))
{
	SetIcon(wxICON(sample));

	prepareData();
	createControls();
	fillData();
	bindEvents();
	buildLayout();
}

std::string SelectPlatformView::selectedPlatform() const
{
	return _selectedPlatform;
}

void SelectPlatformView::createControls()
{
	_mainGroup = new wxStaticBox(this, wxID_ANY, "select_platform/select"_lng);
	_listView = new wxListView(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_LIST | wxLC_ALIGN_LEFT | wxLC_SINGLE_SEL);
}

void SelectPlatformView::buildLayout()
{
	auto mainSizer = new wxStaticBoxSizer(_mainGroup, wxVERTICAL);

	mainSizer->Add(_listView, wxSizerFlags(1).Expand());

	this->SetSizer(mainSizer);
}

void SelectPlatformView::prepareData()
{
	/*for (const auto& platform : mm::utility::platform_service_instance()->availablePlatforms())
		_platformList.emplace_back(platform->getId(), platform->getPlatformName());*/
	// TODO: rewrite
}

void SelectPlatformView::selectionMade(const std::string& id)
{
	_selectedPlatform = id;
	EndDialog(wxID_OK);
}

void SelectPlatformView::bindEvents()
{
	_listView->Bind(wxEVT_LIST_ITEM_ACTIVATED, [=](const wxListEvent& item) {
		const auto index = item.GetIndex();

		if (index >= 0 && static_cast<size_t>(index) < _platformList.size())
			selectionMade(_platformList[index].id);
	});
}

void SelectPlatformView::fillData()
{
	for (size_t i = 0; i < _platformList.size(); ++i)
		_listView->InsertItem(i, _platformList[i].name);
}
