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
#include "interface/iplugin_manager.hpp"
#include "interface/ipreset_manager.hpp"
#include "mod_list_model.h"
#include "plugin_list_model.hpp"
#include "type/embedded_icon.h"
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

	updateLayout();
	bindEvents();
	updatePreview();
}

void ExportPresetDialog::createControls()
{
	_exportInfo = new wxStaticBox(this, wxID_ANY, "Export"_lng);

	_exportData = new wxTextCtrl(_exportInfo, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
		wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2 | wxTE_AUTO_URL | wxTE_BESTWRAP);

	_saveExecutable = new wxCheckBox(_exportInfo, wxID_ANY, "Save selected executable"_lng);
	_saveExecutable->SetValue(true);
	_savePlugins = new wxCheckBox(_exportInfo, wxID_ANY, "Save selected plugins"_lng);
	_savePlugins->SetValue(true);

	_copyToClipboard = new wxButton(_exportInfo, wxID_ANY, "Copy"_lng);
	_copyToClipboard->SetBitmap(_iconStorage.get(embedded_icon::copy));
	_saveToFile = new wxButton(_exportInfo, wxID_ANY, "Save"_lng);
	_saveToFile->SetBitmap(_iconStorage.get(embedded_icon::save_to_file));
	_saveToFile->Hide();

	_ok = new wxButton(_exportInfo, wxID_ANY, "OK"_lng);

	_infoBar = new wxInfoBar(this);
	_infoBarTimer.SetOwner(this);
}

void ExportPresetDialog::updateLayout()
{
	auto controls = new wxBoxSizer(wxHORIZONTAL);
	controls->Add(_copyToClipboard, wxSizerFlags(0).Expand().Border(wxALL, 5));
	controls->Add(_saveToFile, wxSizerFlags(0).Expand().Border(wxALL, 5));
	controls->AddStretchSpacer();
	controls->Add(_ok, wxSizerFlags(0).Expand().Border(wxALL, 5));

	auto mainSpacer = new wxStaticBoxSizer(_exportInfo, wxVERTICAL);
	mainSpacer->Add(_exportData, wxSizerFlags(1).Expand().Border(wxALL, 5));
	mainSpacer->AddSpacer(16);
	mainSpacer->Add(_saveExecutable, wxSizerFlags(0).Expand().Border(wxALL, 5));
	mainSpacer->Add(_savePlugins, wxSizerFlags(0).Expand().Border(wxALL, 5));
	mainSpacer->AddSpacer(16);
	mainSpacer->Add(_infoBar, wxSizerFlags(0).Expand().Border(wxALL, 5));
	mainSpacer->Add(controls, wxSizerFlags(0).Expand().Border(wxALL, 5));

	SetSizer(mainSpacer);
	Layout();
}

void ExportPresetDialog::bindEvents()
{
	_saveExecutable->Bind(wxEVT_CHECKBOX, [=](wxCommandEvent&) { updatePreview(); });
	_savePlugins->Bind(wxEVT_CHECKBOX, [=](wxCommandEvent&) { updatePreview(); });

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
		preset.mods       = _platform.modManager()->mods();
		preset.plugins    = _platform.pluginManager()->plugins();
		preset.executable = _platform.launchHelper()->getExecutable();
	}

	if (!_saveExecutable->IsChecked())
		preset.executable.clear();

	if (!_savePlugins->IsChecked())
		preset.plugins.managed.clear();

	const auto data = _platform.getPresetManager()->savePreset(preset);

	_exportData->SetValue(wxString::FromUTF8(data.dump(2)));

	Layout();

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

void ExportPresetDialog::onSaveToFileRequested() {}
