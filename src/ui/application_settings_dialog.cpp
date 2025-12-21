// SD Mod Manager

// Copyright (c) 2025 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "application_settings_dialog.h"

#include "application.h"
#include "interface/iapp_config.hpp"
#include "interface/ii18n_service.hpp"
#include "interface/iicon_storage.hpp"
#include "interface/ilocal_config.hpp"
#include "mod_manager_app.h"
#include "type/conflict_resolve_mode.hpp"
#include "type/filesystem.hpp"
#include "type/icon.hpp"
#include "type/interface_label.hpp"
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

ApplicationSettingsDialog::ApplicationSettingsDialog(
	wxWindow* parent, Application& app, ILocalConfig* localConfig)
	: wxDialog(parent, wxID_ANY, "dialog/settings/caption"_lng, wxDefaultPosition, wxDefaultSize)
	, _app(app)
	, _localConfig(localConfig)
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
	_globalGroup = new wxStaticBox(this, wxID_ANY, "dialog/settings/program_options"_lng);

	_languageStatic = new wxStaticText(this, wxID_ANY, "dialog/main_frame/menu/tools/language"_lng);
	wxArrayString items;

	int lng = 0;
	for (const auto& lngCode : _app.i18nService().available())
	{
		if (_app.appConfig().currentLanguageCode() == lngCode)
			lng = items.size();

		items.push_back(wxString::FromUTF8(_app.i18nService().languageName(lngCode)));
	}

	_languageChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, items);
	_languageChoice->SetSelection(lng);

	_updateStatic = new wxStaticText(this, wxID_ANY, "dialog/settings/update_mode/label"_lng);

	items.clear();
	for (const auto& item : UpdateCheckModeValues)
		items.push_back(wxString::FromUTF8(
			wxGetApp().translationString("dialog/settings/update_mode/" + to_string(item))));

	_updateChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, items);
	_updateChoice->SetSelection(static_cast<int>(_app.appConfig().updateCheckMode()));

	_interfaceSizeStatic = new wxStaticText(this, wxID_ANY, "dialog/settings/interface_size/label"_lng);
	items.clear();

	for (const auto& item : InterfaceSizeValues)
		items.Add(wxString::FromUTF8(wxGetApp().translationString(
			"dialog/settings/interface_size/" + std::string(magic_enum::enum_name(item)))));

	_interfaceSizeChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, items);
	_interfaceSizeChoice->SetSelection(static_cast<int>(_app.appConfig().interfaceSize()));

	_interfaceLabelStatic = new wxStaticText(this, wxID_ANY, "dialog/settings/interface_label/label"_lng);
	items.clear();

	for (const auto& item : InterfaceLabelValues)
		items.Add(wxString::FromUTF8(wxGetApp().translationString(
			"dialog/settings/interface_label/" + std::string(magic_enum::enum_name(item)))));

	_interfaceLabelChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, items);
	_interfaceLabelChoice->SetSelection(static_cast<int>(_app.appConfig().interfaceLabel()));

	_modDescriptionControlStatic =
		new wxStaticText(this, wxID_ANY, "dialog/settings/mod_description_control/label"_lng);
	items.clear();

	for (const auto& item : ModDescriptionUsedControlValues)
		items.Add(wxString::FromUTF8(wxGetApp().translationString(
			"dialog/settings/mod_description_control/" + std::string(magic_enum::enum_name(item)))));

	_modDescriptionControlChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, items);
	_modDescriptionControlChoice->SetSelection(
		static_cast<int>(_app.appConfig().modDescriptionUsedControl()));

	_platformGroup = new wxStaticBox(this, wxID_ANY, "dialog/settings/platform_options"_lng);

	_conflictResolveStatic =
		new wxStaticText(this, wxID_ANY, "dialog/settings/conflict_resolve_mode/label"_lng);
	items.clear();

	for (const auto& item : ConflictResolveModeValues)
		items.Add(wxString::FromUTF8(wxGetApp().translationString(
			"dialog/settings/conflict_resolve_mode/" + std::string(magic_enum::enum_name(item)))));

	_conflictResolveChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, items);
	_conflictResolveChoice->SetSelection(static_cast<int>(_localConfig->conflictResolveMode()));

	_save   = new wxButton(this, wxID_OK, "dialog/button/save"_lng);
	_cancel = new wxButton(this, wxID_CANCEL, "dialog/cancel"_lng);
}

void ApplicationSettingsDialog::bindEvents()
{
	_save->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) {
		bool restartRequired = false;  // TODO: make something better;

		_app.appConfig().updateCheckMode(static_cast<UpdateCheckMode>(_updateChoice->GetSelection()));

		restartRequired = _app.appConfig().setCurrentLanguageCode(
							  _app.i18nService().available().at(_languageChoice->GetSelection())) ||
						  restartRequired;

		restartRequired = _app.appConfig().interfaceSize(
							  static_cast<InterfaceSize>(_interfaceSizeChoice->GetSelection())) ||
						  restartRequired;

		restartRequired = _app.appConfig().interfaceLabel(
							  static_cast<InterfaceLabel>(_interfaceLabelChoice->GetSelection())) ||
						  restartRequired;

		restartRequired = _app.appConfig().modDescriptionUsedControl(static_cast<ModDescriptionUsedControl>(
							  _modDescriptionControlChoice->GetSelection())) ||
						  restartRequired;

		_localConfig->conflictResolveMode(
			static_cast<ConflictResolveMode>(_conflictResolveChoice->GetSelection()));

		EndModal(restartRequired ? wxID_APPLY : wxID_OK);
	});
	_cancel->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { EndModal(wxID_CANCEL); });
}

void ApplicationSettingsDialog::buildLayout()
{
	auto comboSizer = new wxFlexGridSizer(2);

	comboSizer->Add(_languageStatic, wxSizerFlags(1).Border(wxALL, 5).Align(wxALIGN_CENTER_VERTICAL));
	comboSizer->Add(
		_languageChoice, wxSizerFlags(1).Expand().Border(wxALL, 5).Align(wxALIGN_CENTER_VERTICAL));

	comboSizer->Add(_updateStatic, wxSizerFlags(1).Border(wxALL, 5).Align(wxALIGN_CENTER_VERTICAL));
	comboSizer->Add(_updateChoice, wxSizerFlags(1).Expand().Border(wxALL, 5).Align(wxALIGN_CENTER_VERTICAL));

	comboSizer->Add(_interfaceSizeStatic, wxSizerFlags(1).Border(wxALL, 5).Align(wxALIGN_CENTER_VERTICAL));
	comboSizer->Add(
		_interfaceSizeChoice, wxSizerFlags(1).Expand().Border(wxALL, 5).Align(wxALIGN_CENTER_VERTICAL));

	comboSizer->Add(_interfaceLabelStatic, wxSizerFlags(1).Border(wxALL, 5).Align(wxALIGN_CENTER_VERTICAL));
	comboSizer->Add(
		_interfaceLabelChoice, wxSizerFlags(1).Expand().Border(wxALL, 5).Align(wxALIGN_CENTER_VERTICAL));

	comboSizer->Add(
		_modDescriptionControlStatic, wxSizerFlags(1).Border(wxALL, 5).Align(wxALIGN_CENTER_VERTICAL));
	comboSizer->Add(_modDescriptionControlChoice,
		wxSizerFlags(1).Expand().Border(wxALL, 5).Align(wxALIGN_CENTER_VERTICAL));

	auto topSizer = new wxStaticBoxSizer(_globalGroup, wxVERTICAL);
	topSizer->Add(comboSizer, wxSizerFlags(1).Expand().Border(wxALL, 5));

	auto sizer2 = new wxBoxSizer(wxHORIZONTAL);
	sizer2->Add(_conflictResolveStatic, wxSizerFlags(0).Border(wxALL, 5).Align(wxALIGN_CENTER_VERTICAL));
	sizer2->AddStretchSpacer();
	sizer2->Add(_conflictResolveChoice, wxSizerFlags(0).Border(wxALL, 5).Align(wxALIGN_CENTER_VERTICAL));

	auto sizer3 = new wxStaticBoxSizer(_platformGroup, wxVERTICAL);
	sizer3->Add(sizer2, wxSizerFlags(1).Expand().Border(wxALL, 5));

	auto buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	buttonSizer->AddStretchSpacer();
	buttonSizer->Add(_save, wxSizerFlags(0).Expand().Border(wxALL, 5));
	buttonSizer->Add(_cancel, wxSizerFlags(0).Expand().Border(wxALL, 5));

	auto mainSizer = new wxBoxSizer(wxVERTICAL);
	mainSizer->Add(topSizer, wxSizerFlags(1).Expand().Border(wxALL, 5));
	mainSizer->Add(sizer3, wxSizerFlags(0).Expand().Border(wxALL, 5));
	mainSizer->Add(buttonSizer, wxSizerFlags(0).Expand());

	this->SetSizer(mainSizer);

	Fit();
}
