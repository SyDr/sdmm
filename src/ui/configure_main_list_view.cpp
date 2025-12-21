// SD Mod Manager

// Copyright (c) 2024-2025 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "configure_main_list_view.h"

#include "application.h"
#include "interface/ii18n_service.hpp"
#include "interface/iicon_storage.hpp"
#include "mod_manager_app.h"
#include "type/icon.hpp"
#include "type/filesystem.hpp"

#include <boost/algorithm/string.hpp>
#include <wx/app.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/dir.h>
#include <wx/imaglist.h>
#include <wx/listctrl.h>
#include <wx/rearrangectrl.h>
#include <wx/sizer.h>
#include <magic_enum.hpp>

using namespace mm;

ConfigureMainListView::ConfigureMainListView(wxWindow* parent, IIconStorage& iconStorage,
	const std::vector<int>& columns, ModListModelManagedMode initialManagedMode,
	ModListModelArchivedMode initialArchivedMode)
	: wxDialog(parent, wxID_ANY, "dialog/settings/configure_main_view/caption"_lng, wxDefaultPosition, wxSize(500, 600))
	, _listModel(new ModListModel(*this, iconStorage, ModListModelManagedMode::as_flat_list,
		  ModListModelArchivedMode::as_single_group, Icon::Size::x16))
	, _initialManagedMode(initialManagedMode)
	, _initialArchivedMode(initialArchivedMode)
{
	createControls();
	buildLayout();
	bindEvents();

	for (const auto& item : columns)
		_mods.data.emplace_back(
			std::string(magic_enum::enum_name(static_cast<ModListModelColumn>(std::abs(item)))),
			item > 0 ? ModList::ModState::enabled : ModList::ModState::disabled);

	_listModel->modList(_mods);

	std::unordered_set<std::string> items;
	for (const auto& item : _mods.data)
		if (item.state == ModList::ModState::enabled)
			items.emplace(item.id);

	_listModel->setChecked(items);
}

std::vector<int> ConfigureMainListView::getColumns() const
{
	std::vector<int> result;

	const auto& checked = _listModel->getChecked();
	for (const auto& item : _mods.data)
	{
		const auto mlmc = magic_enum::enum_cast<ModListModelColumn>(item.id);
		if (!mlmc.has_value()) // hmm???
			continue;

		int i = static_cast<int>(mlmc.value());
		if (!checked.contains(item.id))
			i = -i;

		result.emplace_back(i);
	}

	return result;
}

ModListModelManagedMode ConfigureMainListView::getManagedMode() const
{
	return static_cast<ModListModelManagedMode>(_managedChoice->GetSelection());
}

ModListModelArchivedMode ConfigureMainListView::getArchivedMode() const
{
	return static_cast<ModListModelArchivedMode>(_archivedChoice->GetSelection());
}

void ConfigureMainListView::createControls()
{
	_managedStatic = new wxStaticText(this, wxID_ANY, "dialog/settings/configure_main_view/managed_mods"_lng);
	createManagedControl();

	_archivedStatic = new wxStaticText(this, wxID_ANY, "dialog/settings/configure_main_view/archived_mods"_lng);
	createArchivedControl();

	createListControl();

	_save   = new wxButton(this, wxID_OK, "dialog/button/save"_lng);
	_cancel = new wxButton(this, wxID_CANCEL, "dialog/cancel"_lng);
}

void ConfigureMainListView::createListControl()
{
	_list = new wxDataViewCtrl(
		this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_ROW_LINES | wxDV_VERT_RULES);
	_list->EnableDragSource(wxDF_UNICODETEXT);
	_list->EnableDropTarget(wxDF_UNICODETEXT);
	_list->AssociateModel(_listModel.get());

	createListColumns();

	_list->Bind(wxEVT_DATAVIEW_ITEM_BEGIN_DRAG, [=](wxDataViewEvent& event) {
		auto moveFrom = _listModel->findIdByItem(event.GetItem());
		if (moveFrom.empty())
		{
			event.Veto();
			return;
		}

		event.SetDataObject(new wxTextDataObject(wxString::FromUTF8(moveFrom)));
		event.SetDragFlags(wxDrag_DefaultMove);
	});

	_list->Bind(wxEVT_DATAVIEW_ITEM_DROP_POSSIBLE, [=](wxDataViewEvent& event) {
		if (!event.GetItem().IsOk())
		{
			event.Veto();
			return;
		}

		auto moveTo = _listModel->findIdByItem(event.GetItem());
		if (moveTo.empty())
		{
			event.Veto();
			return;
		}
	});

	_list->Bind(wxEVT_DATAVIEW_ITEM_DROP, [=](wxDataViewEvent& event) {
		wxTextDataObject from;
		from.SetData(wxDF_UNICODETEXT, event.GetDataSize(), event.GetDataBuffer());

		const auto moveFrom = from.GetText().utf8_string();
		_mods.move(moveFrom, _listModel->findIdByItem(event.GetItem()));

		_listModel->modList(_mods);
	});
}

void ConfigureMainListView::createManagedControl()
{
	wxArrayString items;
	for (const auto& item : ManagedModeValues)
		items.Add(wxString::FromUTF8(wxGetApp().translationString(
			"dialog/settings/configure_main_view/managed_mods_value/" + std::string(magic_enum::enum_name(item)))));

	_managedChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, items);
	_managedChoice->SetSelection(static_cast<int>(_initialManagedMode));
}

void ConfigureMainListView::createArchivedControl()
{
	wxArrayString items;
	for (const auto& item : ArchivedModeValues)
		items.Add(wxString::FromUTF8(wxGetApp().translationString(
			"dialog/settings/configure_main_view/archived_mods_value/" + std::string(magic_enum::enum_name(item)))));

	_archivedChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, items);
	_archivedChoice->SetSelection(static_cast<int>(_initialArchivedMode));
}

void ConfigureMainListView::createListColumns()
{
	auto r0 =
		new wxDataViewToggleRenderer(wxDataViewToggleRenderer::GetDefaultType(), wxDATAVIEW_CELL_ACTIVATABLE);
	auto r2 = new wxDataViewIconTextRenderer();

	r2->SetAlignment(wxALIGN_CENTER_VERTICAL);

	auto column0 = new wxDataViewColumn(L" ", r0, static_cast<unsigned int>(ModListModelColumn::checkbox),
		wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER);
	auto column2 = new wxDataViewColumn("dialog/settings/configure_main_view/column"_lng, r2, static_cast<unsigned int>(ModListModelColumn::name),
		wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER);

	_list->AppendColumn(column0);
	_list->AppendColumn(column2);
}

void ConfigureMainListView::bindEvents()
{
	_save->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { EndModal(wxID_OK); });
	_cancel->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { EndModal(wxID_CANCEL); });
}

void ConfigureMainListView::buildLayout()
{
	auto comboSizer = new wxFlexGridSizer(2, 2, wxSize(0, 0));
	comboSizer->Add(_managedStatic, wxSizerFlags(1).Border(wxALL, 5).Align(wxALIGN_CENTER_VERTICAL));
	comboSizer->Add(_managedChoice, wxSizerFlags(1).Expand().Border(wxALL, 5));
	comboSizer->Add(_archivedStatic, wxSizerFlags(1).Border(wxALL, 5).Align(wxALIGN_CENTER_VERTICAL));
	comboSizer->Add(_archivedChoice, wxSizerFlags(1).Expand().Border(wxALL, 5));

	auto topSizer = new wxBoxSizer(wxHORIZONTAL);
	topSizer->Add(_list, wxSizerFlags(1).Expand().Border(wxALL, 5));

	auto buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	buttonSizer->AddStretchSpacer();
	buttonSizer->Add(_save, wxSizerFlags(0).Expand().Border(wxALL, 5));
	buttonSizer->Add(_cancel, wxSizerFlags(0).Expand().Border(wxALL, 5));

	auto mainSizer = new wxBoxSizer(wxVERTICAL);
	mainSizer->Add(comboSizer, wxSizerFlags(0).Expand());
	mainSizer->Add(topSizer, wxSizerFlags(1).Expand());
	mainSizer->Add(buttonSizer, wxSizerFlags(0).Expand());

	this->SetSizer(mainSizer);
}

const ModData& ConfigureMainListView::modData(const std::string& id)
{
	auto it = _data.find(id);

	if (it == _data.cend())
	{
		ModData md;
		md.id   = id;
		md.name = wxGetApp().i18nService().column(id);

		std::tie(it, std::ignore) = _data.emplace(id, std::move(md));
	}

	return it->second;
}

const std::string& mm::ConfigureMainListView::description(const std::string&)
{
	return _workaround;  // FIXME: refactor this piece of
}
