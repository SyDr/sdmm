// SD Mod Manager

// Copyright (c) 2023-2024 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "import_preset_dialog.hpp"

#include "application.h"
#include "domain/mod_list.hpp"
#include "error_view.h"
#include "interface/iicon_storage.hpp"
#include "interface/ilaunch_helper.hpp"
#include "interface/ilocal_config.hpp"
#include "interface/imod_manager.hpp"
#include "interface/imod_platform.hpp"
#include "interface/ipreset_manager.hpp"
#include "mod_list_model.h"
#include "type/embedded_icon.h"
#include "utility/fs_util.h"
#include "utility/sdlexcept.h"
#include "wx/priority_data_renderer.h"

#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/indexed.hpp>
#include <wx/app.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/clipbrd.h>
#include <wx/dir.h>
#include <wx/generic/textdlgg.h>
#include <wx/infobar.h>
#include <wx/listbox.h>
#include <wx/listctrl.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>

#include <format>

using namespace mm;

ImportPresetDialog::ImportPresetDialog(wxWindow* parent, IModPlatform& platform, IIconStorage& iconStorage)
	: wxDialog(parent, wxID_ANY, "Import"_lng, wxDefaultPosition, { 600, 800 })
	, _platform(platform)
	, _iconStorage(iconStorage)
{
	MM_EXPECTS(parent, mm::unexpected_error);

	createControls();

	updateLayout();
	bindEvents();

	CallAfter(&ImportPresetDialog::onPasteFromClipboardRequested);
}

void ImportPresetDialog::createControls()
{
	_previewGroup = new wxStaticBox(this, wxID_ANY, "Preview"_lng);

	_importData = new wxTextCtrl(_previewGroup, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
		wxTE_MULTILINE | wxTE_BESTWRAP);

	_optionsBox      = new wxStaticBox(this, wxID_ANY, "Import options"_lng);
	_importNameLabel = new wxStaticText(_optionsBox, wxID_ANY, "Import with name:"_lng);
	_importName      = new wxTextCtrl(_optionsBox, wxID_ANY);
	_clearName       = new wxBitmapButton(_optionsBox, wxID_ANY, _iconStorage.get(embedded_icon::clear));
	_loadNow         = new wxCheckBox(_optionsBox, wxID_ANY, "Apply immediately"_lng);
	_loadNow->SetValue(true);

	_infoBar = new wxInfoBar(this);

	_fromClipboard = new wxButton(this, wxID_ANY, "Paste"_lng);
	_fromClipboard->SetBitmap(_iconStorage.get(embedded_icon::copy));
	_fromFile = new wxButton(this, wxID_ANY, "Load from file"_lng);
	_fromFile->SetBitmap(_iconStorage.get(embedded_icon::save_to_file));
	_ok = new wxButton(this, wxID_ANY, L"");

	_infoBarTimer.SetOwner(this);
}

void ImportPresetDialog::updateLayout()
{
	auto previewGroup = new wxStaticBoxSizer(_previewGroup, wxVERTICAL);
	previewGroup->Add(_importData, wxSizerFlags(1).Expand().Border(wxALL, 5));

	auto importName = new wxBoxSizer(wxHORIZONTAL);
	importName->Add(_importNameLabel, wxSizerFlags(0).Border(wxALL, 5).CentreVertical());
	importName->Add(_importName, wxSizerFlags(1).Expand().Border(wxALL, 5));
	importName->Add(_clearName, wxSizerFlags(0).Expand().Border(wxUP | wxDOWN | wxRIGHT, 5));

	auto optionGroup = new wxStaticBoxSizer(_optionsBox, wxVERTICAL);
	optionGroup->Add(importName, wxSizerFlags(0).Expand().Border(wxALL, 5));
	optionGroup->AddStretchSpacer();
	optionGroup->Add(_loadNow, wxSizerFlags(0).Expand().Border(wxALL, 5));

	auto bottomControls = new wxBoxSizer(wxHORIZONTAL);
	bottomControls->Add(_fromClipboard, wxSizerFlags(0).Expand().Border(wxALL, 5));
	bottomControls->Add(_fromFile, wxSizerFlags(0).Expand().Border(wxALL, 5));
	bottomControls->AddStretchSpacer();
	bottomControls->Add(_ok, wxSizerFlags(0).Expand().Border(wxALL, 5));

	auto mainSizer = new wxBoxSizer(wxVERTICAL);
	mainSizer->Add(previewGroup, wxSizerFlags(1).Expand().Border(wxALL, 5));
	mainSizer->Add(optionGroup, wxSizerFlags(0).Expand().Border(wxALL, 5));
	mainSizer->Add(_infoBar, wxSizerFlags(0).Expand().Border(wxALL, 5));
	mainSizer->Add(bottomControls, wxSizerFlags(0).Expand().Border(wxALL, 5));

	SetSizer(mainSizer);
	Layout();
}

void ImportPresetDialog::bindEvents()
{
	_importData->Bind(wxEVT_TEXT, [=](wxCommandEvent&) {
		updatePreview();
		updateOkButton();
	});

	_importName->Bind(wxEVT_TEXT, [=](wxCommandEvent&) { updateOkButton(); });

	_clearName->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) {
		_importName->Clear();
		updateOkButton();
	});

	_fromClipboard->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { onPasteFromClipboardRequested(); });
	_fromFile->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { onImportFromFileRequested(); });

	_loadNow->Bind(wxEVT_CHECKBOX, [=](wxCommandEvent&) { updateOkButton(); });

	_ok->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { doImportAndClose(); });

	Bind(wxEVT_TIMER, [=](wxTimerEvent&) { _infoBar->Dismiss(); });
}

void ImportPresetDialog::updatePreview()
{
	EX_TRY;

	auto parsed = nlohmann::json::parse(_importData->GetValue().utf8_string(), nullptr, false);

	if (parsed.is_discarded())
		parsed = nlohmann::json::object();

	_importName->SetValue(
		parsed["name"].is_string() ? wxString::FromUTF8(parsed["name"].get<std::string>()) : wxString());

	EX_UNEXPECTED;
}

void ImportPresetDialog::updateOkButton()
{
	EX_TRY;

	_ok->Enable();

	if (!_loadNow->IsChecked() && _importName->IsEmpty())
	{
		_ok->SetLabel("Select option"_lng);
		_ok->Disable();
	}
	else if (_loadNow->IsChecked() && !_importName->IsEmpty())
		_ok->SetLabel("Save and apply"_lng);
	else if (!_importName->IsEmpty())
		_ok->SetLabel("Save only"_lng);
	else  // if (_loadNow->IsChecked())
		_ok->SetLabel("Apply only"_lng);

	Layout();

	EX_UNEXPECTED;
}

void ImportPresetDialog::doImportAndClose()
{
	auto parsed     = nlohmann::json::parse(_importData->GetValue().utf8_string(), nullptr, false);
	auto presetData = _platform.getPresetManager()->loadPreset(parsed);

	if (_loadNow->IsChecked())
		_platform.apply(&presetData.mods);

	if (!_importName->IsEmpty())
	{
		_platform.getPresetManager()->savePreset(_importName->GetValue().ToStdString(wxConvUTF8),
			{ presetData.mods, presetData.executable });
		_platform.launchHelper()->setExecutable(presetData.executable);
		if (_loadNow->IsChecked())
			_platform.localConfig()->setActivePreset(_importName->GetValue().ToStdString(wxConvUTF8));
	}

	EndModal(wxID_OK);
}

void ImportPresetDialog::onPasteFromClipboardRequested()
{
	EX_TRY;

	if (wxTheClipboard->Open())
	{
		if (wxTheClipboard->IsSupported(wxDF_TEXT))
		{
			wxTextDataObject data;
			wxTheClipboard->GetData(data);

			_importData->SetValue(data.GetText());
			updatePreview();
			updateOkButton();
		}

		_infoBar->ShowMessage("Copied from clipboard"_lng);
		_infoBarTimer.StartOnce(5000);
	}
	else
	{
		_infoBar->ShowMessage("Cannot open clipboard"_lng, wxICON_EXCLAMATION);
		_infoBarTimer.StartOnce(5000);
	}

	EX_UNEXPECTED;
}

void ImportPresetDialog::onImportFromFileRequested()
{
	EX_TRY;

	wxFileDialog fileDialog(
		this, {}, {}, {}, "json files (*.json)|*json"_lng, wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	if (fileDialog.ShowModal() != wxID_OK)
		return;

	fs::path targetPath = fileDialog.GetPath().utf8_string();

	_importData->SetValue(wxString::FromUTF8(readFile(targetPath)));

	_infoBar->ShowMessage("Loaded from file"_lng);
	_infoBarTimer.StartOnce(5000);

	EX_UNEXPECTED;
}
