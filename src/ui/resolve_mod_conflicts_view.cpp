// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "resolve_mod_conflicts_view.hpp"

#include "interface/iapp_config.hpp"
#include "application.h"
#include "utility/sdlexcept.h"
#include "interface/iicon_storage.h"
#include "type/embedded_icon.h"
#include "wx/data_view_multiple_icons_renderer.h"
#include "mod_list_model.h"
#include "wx/priority_data_renderer.h"
#include "interface/imod_platform.hpp"
#include "interface/imod_manager.hpp"
#include "interface/imod_data_provider.hpp"
#include "domain/mod_data.hpp"

#include <boost/algorithm/string.hpp>
#include <wx/dir.h>
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/imaglist.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/dirctrl.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/dataview.h>

#include <algorithm>

using namespace mm;

ResolveModConflictsView::ResolveModConflictsView(wxWindow *parent, IAppConfig& config, IIconStorage& iconStorage, IModPlatform& managedPlatform)
	: wxDialog(parent, wxID_ANY, "Resolve mod conflicts"_lng, wxDefaultPosition, wxSize(800, 444), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
	, _appConfig(config)
	, _iconStorage(iconStorage)
	, _managedPlatform(managedPlatform)
	, _listModel(new ModListModel(*managedPlatform.modDataProvider(), iconStorage, false))
{
	SetIcon(_iconStorage.get(embedded_icon::main_icon));

	_sortedMods = _managedPlatform.modManager()->mods();
	_listModel->showInactive(false);
	_listModel->setModList(_sortedMods);

	createControls();
	buildLayout();
	bindEvents();
}

void ResolveModConflictsView::createControls()
{
	_log = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
		wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2 | wxTE_AUTO_URL | wxTE_BESTWRAP);

	_start = new wxButton(this, wxID_ANY, "Start"_lng);
	_apply = new wxButton(this, wxID_ANY, "Apply"_lng);

	createListControl();
}

void ResolveModConflictsView::createListControl()
{
	_list = new wxDataViewCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxDV_ROW_LINES | wxDV_VERT_RULES);
	_list->AssociateModel(_listModel.get());

	createListColumns();
}

void ResolveModConflictsView::createListColumns()
{
	auto r0 = new mmPriorityDataRenderer();
	auto r1 = new wxDataViewIconTextRenderer();

	r0->SetAlignment(wxALIGN_CENTER_VERTICAL);
	r1->SetAlignment(wxALIGN_CENTER_VERTICAL);

	auto column0 = new wxDataViewColumn(" ", r0, static_cast<unsigned int>(ModListModel::Column::priority), wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER);
	auto column1 = new wxDataViewColumn("Mod"_lng, r1, static_cast<unsigned int>(ModListModel::Column::caption), wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER);

	_list->AppendColumn(column0);
	_list->AppendColumn(column1);

	column0->SetSortOrder(true);
}

void ResolveModConflictsView::bindEvents()
{
	_start->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) {
		doResolveConflicts();
	});

	_apply->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) {
		_managedPlatform.apply(&_sortedMods, nullptr);
		EndModal(wxID_OK);
	});
}

void ResolveModConflictsView::buildLayout()
{
	auto topSizer = new wxBoxSizer(wxHORIZONTAL);
	topSizer->Add(_list, wxSizerFlags(1).Expand().Border(wxALL, 4));
	topSizer->Add(_log, wxSizerFlags(1).Expand().Border(wxALL, 4));

	auto buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	buttonSizer->AddStretchSpacer();
	buttonSizer->Add(_start, wxSizerFlags(0).Expand().Border(wxALL, 4));
	buttonSizer->Add(_apply, wxSizerFlags(0).Expand().Border(wxALL, 4));

	auto mainSizer = new wxBoxSizer(wxVERTICAL);
	mainSizer->Add(topSizer, wxSizerFlags(1).Expand());
	mainSizer->Add(buttonSizer, wxSizerFlags(0).Expand());

	this->SetSizer(mainSizer);
}

void ResolveModConflictsView::doResolveConflicts()
{
	EX_TRY;

	wxBusyCursor bc;

	_log->Clear();
	_log->AppendText("Starting...\n"_lng);

	auto modDataProvider = _managedPlatform.modDataProvider();
	auto currentState = _sortedMods;

	auto currentlyActive = currentState.active;

	std::set<wxString> activatedInSession;
	std::set<wxString> disabledInSession;

	for (size_t i = 0; i < currentlyActive.size();)
	{
		const auto currentId = currentlyActive[i];

		auto modData = modDataProvider->modData(currentId);
		for (const auto& id : modData->requires_)
		{
			activatedInSession.insert(id);

			if (std::find(currentlyActive.cbegin(), currentlyActive.cend(), id) == currentlyActive.cend())
			{
				currentlyActive.emplace_back(id);
				_log->AppendText(wxString::Format("%s enabled by %s\n"_lng, id, currentId));
			}
		}

		for (const auto& id : modData->incompatible)
		{
			disabledInSession.insert(id);

			if (auto it = std::find(currentlyActive.cbegin(), currentlyActive.cend(), id); it != currentlyActive.cend())
			{
				currentlyActive.erase(it);
				_log->AppendText(wxString::Format("%s disabled by %s\n"_lng, id, currentId));
			}
		}

		i++;
	}

	std::vector<wxString> sortedActive;

	for (size_t i = 0; !currentlyActive.empty();)
	{
		const auto& candidate = currentlyActive[i];
		_log->AppendText(wxString::Format("Position check: %s... "_lng, candidate));

		bool ok = true;
		for (size_t j = 0; ok && j < currentlyActive.size(); ++j)
		{
			if (i == j)
				continue;

			if (modDataProvider->modData(currentlyActive[j])->load_after.count(candidate))
			{
				ok = false;
				_log->AppendText(wxString::Format("fail, must be below %s\n"_lng, currentlyActive[j]));
			}
		}

		if (ok)
		{
			_log->AppendText("ok\n"_lng);
			sortedActive.emplace_back(candidate);
			currentlyActive.erase(currentlyActive.cbegin() + i);
			i = 0;
		}
		else
		{
			++i;
			if (i == currentlyActive.size())
			{
				_log->AppendText(wxString::Format("All failed, %s added\n"_lng, currentlyActive.front()));
				sortedActive.emplace_back(currentlyActive.front());
				currentlyActive.erase(currentlyActive.cbegin());
				i = 0;
			}
		}
	}

	_log->AppendText("Done\n"_lng);

	currentState.active = sortedActive;
	for (const auto& item : sortedActive)
		currentState.hidden.erase(item);

	std::swap(currentState, _sortedMods);
	_listModel->setModList(_sortedMods);

	EX_UNEXPECTED;
}
