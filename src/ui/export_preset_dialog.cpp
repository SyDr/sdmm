// SD Mod Manager

// Copyright (c) 2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "export_preset_dialog.hpp"

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

ExportPresetDialog::ExportPresetDialog(
	wxWindow* parent, IModPlatform& platform, IIconStorage& iconStorage, const std::string& preset)
	: wxDialog(parent, wxID_ANY, "Export"_lng, wxDefaultPosition, { 600, 800 })
	, _platform(platform)
	, _selected(preset)
	, _iconStorage(iconStorage)
{
	MM_EXPECTS(parent, mm::unexpected_error);

	createControls();

	_exportName->SetValue(wxString::FromUTF8(_selected));

	updateLayout();
	bindEvents();
	updatePreview();
}

void ExportPresetDialog::createControls()
{
	_previewGroup = new wxStaticBox(this, wxID_ANY, "Preview"_lng);

	_exportData = new wxTextCtrl(_previewGroup, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
		wxTE_MULTILINE | wxTE_READONLY | wxTE_BESTWRAP);

	_optionsBox     = new wxStaticBox(this, wxID_ANY, "Export options"_lng);
	_saveExecutable = new wxCheckBox(_optionsBox, wxID_ANY, "Save selected executable"_lng);
	_saveExecutable->SetValue(true);
	_exportNameLabel = new wxStaticText(_optionsBox, wxID_ANY, "Profile name:"_lng);
	_exportName      = new wxTextCtrl(_optionsBox, wxID_ANY);

	_copyToClipboard = new wxButton(this, wxID_ANY, "Copy"_lng);
	_copyToClipboard->SetBitmap(_iconStorage.get(IconPredefined::copy, IconPredefinedSize::x16));
	_saveToFile = new wxButton(this, wxID_ANY, "Save to file"_lng);
	_saveToFile->SetBitmap(_iconStorage.get(IconPredefined::save_to_file, IconPredefinedSize::x16));

	_infoBar = new wxInfoBar(this);
	_ok      = new wxButton(this, wxID_ANY, "OK"_lng);

	_infoBarTimer.SetOwner(this);
}

void ExportPresetDialog::updateLayout()
{
	auto previewGroup = new wxStaticBoxSizer(_previewGroup, wxVERTICAL);
	previewGroup->Add(_exportData, wxSizerFlags(1).Expand().Border(wxALL, 5));

	auto exportName = new wxBoxSizer(wxHORIZONTAL);
	exportName->Add(_exportNameLabel, wxSizerFlags(0).Expand().Border(wxALL, 5));
	exportName->Add(_exportName, wxSizerFlags(1).Expand().Border(wxALL, 5));

	auto optionGroup = new wxStaticBoxSizer(_optionsBox, wxVERTICAL);
	optionGroup->Add(_saveExecutable, wxSizerFlags(0).Expand().Border(wxALL, 5));
	optionGroup->Add(exportName, wxSizerFlags(0).Expand().Border(wxALL, 5));

	auto bottomControls = new wxBoxSizer(wxHORIZONTAL);
	bottomControls->Add(_copyToClipboard, wxSizerFlags(0).Expand().Border(wxALL, 5));
	bottomControls->Add(_saveToFile, wxSizerFlags(0).Expand().Border(wxALL, 5));
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

void ExportPresetDialog::bindEvents()
{
	_saveExecutable->Bind(wxEVT_CHECKBOX, [=](wxCommandEvent&) { updatePreview(); });
	_exportName->Bind(wxEVT_TEXT, [=](wxCommandEvent&) { updatePreview(); });

	_copyToClipboard->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { onCopyToClipboardRequested(); });
	_saveToFile->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { onSaveToFileRequested(); });

	_ok->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { EndModal(wxID_OK); });

	Bind(wxEVT_TIMER, [=](wxTimerEvent&) { _infoBar->Dismiss(); });
}

void ExportPresetDialog::updatePreview()
{
	EX_TRY;

	PresetData preset;

	if (!_selected.empty())
	{
		preset = _platform.getPresetManager()->loadPreset(_selected);
	}
	else
	{
		preset.mods       = _platform.modManager()->mods().enabled();
		preset.executable = _platform.launchHelper()->getExecutable();
	}

	if (!_saveExecutable->IsChecked())
		preset.executable.clear();

	auto data = _platform.getPresetManager()->savePreset(preset);

	if (!_exportName->IsEmpty())
		data["name"] = _exportName->GetValue().utf8_string();

	_exportData->SetValue(wxString::FromUTF8(data.dump(2)));

	EX_UNEXPECTED;
}

void ExportPresetDialog::onCopyToClipboardRequested()
{
	EX_TRY;

	if (wxTheClipboard->Open())
	{
		wxTheClipboard->SetData(new wxTextDataObject(_exportData->GetValue()));
		wxTheClipboard->Close();

		_infoBar->ShowMessage("Copied"_lng);
		_infoBarTimer.StartOnce(5000);
	}
	else
	{
		_infoBar->ShowMessage("Cannot open clipboard"_lng, wxICON_EXCLAMATION);
		_infoBarTimer.StartOnce(5000);
	}

	EX_UNEXPECTED;
}

void ExportPresetDialog::onSaveToFileRequested()
{
	EX_TRY;

	wxString filename = _exportName->GetValue();
	if (!filename.empty())
		filename += L".json";

	wxFileDialog saveFileDialog(
		this, {}, {}, filename, "json files (*.json)|*json"_lng, wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	if (saveFileDialog.ShowModal() != wxID_OK)
		return;

	fs::path targetPath = saveFileDialog.GetPath().utf8_string();
	if (!targetPath.has_extension())
		targetPath.replace_extension(".json");

	overwriteFile(targetPath, _exportData->GetValue().utf8_string());

	_infoBar->ShowMessage("Saved"_lng);
	_infoBarTimer.StartOnce(5000);

	EX_UNEXPECTED;
}
