// SD Mod Manager

// Copyright (c) 2025 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "application_settings_dialog.h"

#include "application.h"
#include "interface/iapp_config.hpp"
#include "interface/ii18n_service.hpp"
#include "interface/iicon_storage.hpp"
#include "mod_manager_app.h"
#include "type/icon.hpp"
#include "type/filesystem.hpp"
#include "type/interface_size.hpp"
#include "type/mod_description_used_control.hpp"
#include "type/update_check_mode.hpp"

#include <boost/algorithm/string.hpp>
#include <wx/app.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/dir.h>
#include <wx/imaglist.h>
#include <wx/listctrl.h>
#include <wx/rearrangectrl.h>
#include <wx/sizer.h>

using namespace mm;

ApplicationSettingsDialog::ApplicationSettingsDialog(wxWindow* parent, Application& app)
	: wxDialog(parent, wxID_ANY, "Change program settings"_lng, wxDefaultPosition, wxDefaultSize)
	, _app(app)
{
	createControls();
	buildLayout();
	bindEvents();
}

/* ModListModelManagedMode ApplicationSettingsDialog::getManagedMode() const
{
	return static_cast<ModListModelManagedMode>(_managedChoice->GetSelection());
}*/

void ApplicationSettingsDialog::createControls()
{
	_globalGroup = new wxStaticBox(this, wxID_ANY, "Program options"_lng);

	_updateStatic = new wxStaticText(this, wxID_ANY, "Check for program updates:"_lng);
	wxArrayString items;

	for (const auto& item : UpdateCheckModeValues)
		items.push_back(wxString::FromUTF8(wxGetApp().translationString("update_mode/" + to_string(item))));

	_updateChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, items);
	_updateChoice->SetSelection(static_cast<int>(_app.appConfig().updateCheckMode()));

	_interfaceSizeStatic = new wxStaticText(this, wxID_ANY, "Interface size (mod list + some buttons):"_lng);
	items.clear();

	for (const auto& item : InterfaceSizeValues)
		items.Add(wxString::FromUTF8(
			wxGetApp().translationString("interface_size/" + std::string(magic_enum::enum_name(item)))));

	_interfaceSizeChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, items);
	_interfaceSizeChoice->SetSelection(static_cast<int>(_app.appConfig().interfaceSize()));

	_modDescriptionControlStatic = new wxStaticText(this, wxID_ANY, "Mod description:"_lng);
	items.clear();

	for (const auto& item : ModDescriptionUsedControlValues)
		items.Add(wxString::FromUTF8(wxGetApp().translationString(
			"mod_description_control/" + std::string(magic_enum::enum_name(item)))));

	_modDescriptionControlChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, items);
	_modDescriptionControlChoice->SetSelection(
		static_cast<int>(_app.appConfig().modDescriptionUsedControl()));

	_save   = new wxButton(this, wxID_OK, "Save"_lng);
	_cancel = new wxButton(this, wxID_CANCEL, "Cancel"_lng);
}

void ApplicationSettingsDialog::bindEvents()
{
	_save->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) {
		bool restartRequired = false;  // TODO: make something better;
		_app.appConfig().updateCheckMode(static_cast<UpdateCheckMode>(_updateChoice->GetSelection()));
		restartRequired = _app.appConfig().interfaceSize(
							  static_cast<InterfaceSize>(_interfaceSizeChoice->GetSelection())) ||
						  restartRequired;

		restartRequired = _app.appConfig().modDescriptionUsedControl(static_cast<ModDescriptionUsedControl>(
							  _modDescriptionControlChoice->GetSelection())) ||
						  restartRequired;

		EndModal(restartRequired ? wxID_APPLY : wxID_OK);
	});
	_cancel->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { EndModal(wxID_CANCEL); });
}

void ApplicationSettingsDialog::buildLayout()
{
	auto comboSizer = new wxFlexGridSizer(2);
	comboSizer->Add(_updateStatic, wxSizerFlags(1).Border(wxALL, 5).Align(wxALIGN_CENTER_VERTICAL));
	comboSizer->Add(_updateChoice, wxSizerFlags(1).Expand().Border(wxALL, 5).Align(wxALIGN_CENTER_VERTICAL));

	comboSizer->Add(_interfaceSizeStatic, wxSizerFlags(1).Border(wxALL, 5).Align(wxALIGN_CENTER_VERTICAL));
	comboSizer->Add(
		_interfaceSizeChoice, wxSizerFlags(1).Expand().Border(wxALL, 5).Align(wxALIGN_CENTER_VERTICAL));

	comboSizer->Add(
		_modDescriptionControlStatic, wxSizerFlags(1).Border(wxALL, 5).Align(wxALIGN_CENTER_VERTICAL));
	comboSizer->Add(_modDescriptionControlChoice,
		wxSizerFlags(1).Expand().Border(wxALL, 5).Align(wxALIGN_CENTER_VERTICAL));

	auto topSizer = new wxStaticBoxSizer(_globalGroup, wxVERTICAL);
	topSizer->Add(comboSizer, wxSizerFlags(1).Expand().Border(wxALL, 5));

	auto buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	buttonSizer->AddStretchSpacer();
	buttonSizer->Add(_save, wxSizerFlags(0).Expand().Border(wxALL, 5));
	buttonSizer->Add(_cancel, wxSizerFlags(0).Expand().Border(wxALL, 5));

	auto mainSizer = new wxBoxSizer(wxVERTICAL);
	mainSizer->Add(topSizer, wxSizerFlags(1).Expand().Border(wxALL, 5));
	mainSizer->Add(buttonSizer, wxSizerFlags(0).Expand());

	this->SetSizer(mainSizer);

	Fit();
}
