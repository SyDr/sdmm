// SD Mod Manager

// Copyright (c) 2026 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "application.h"
#include "mod_package_install_dialog.hpp"

#include <wx/button.h>
#include <wx/dataview.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/textctrl.h>

using namespace mm;

namespace
{
	wxString diagnosticText(const ModPackageDiagnostic& diagnostic)
	{
		const auto detail = wxString::FromUTF8(diagnostic.detail);
		wxString   message;
		switch (diagnostic.code)
		{
		case ModPackageDiagnosticCode::unsupported_type:
			message = "dialog/package_installer/diagnostic/unsupported_type"_lng;
			break;
		case ModPackageDiagnosticCode::missing_file:
			message = "dialog/package_installer/diagnostic/missing_file"_lng;
			break;
		case ModPackageDiagnosticCode::cannot_read:
			message = "dialog/package_installer/diagnostic/cannot_read"_lng;
			break;
		case ModPackageDiagnosticCode::corrupt_archive:
			message = "dialog/package_installer/diagnostic/corrupt_archive"_lng;
			break;
		case ModPackageDiagnosticCode::encrypted_content:
			message = wxString::Format("dialog/package_installer/diagnostic/encrypted_content"_lng, detail);
			break;
		case ModPackageDiagnosticCode::unsafe_path:
			message = wxString::Format("dialog/package_installer/diagnostic/unsafe_path"_lng, detail);
			break;
		case ModPackageDiagnosticCode::outside_mods:
			message = wxString::Format("dialog/package_installer/diagnostic/outside_mods"_lng, detail);
			break;
		case ModPackageDiagnosticCode::direct_mods_file:
			message = wxString::Format("dialog/package_installer/diagnostic/direct_mods_file"_lng, detail);
			break;
		case ModPackageDiagnosticCode::link_entry:
			message = wxString::Format("dialog/package_installer/diagnostic/link_entry"_lng, detail);
			break;
		case ModPackageDiagnosticCode::invalid_windows_name:
			message =
				wxString::Format("dialog/package_installer/diagnostic/invalid_windows_name"_lng, detail);
			break;
		case ModPackageDiagnosticCode::no_mods:
			message = "dialog/package_installer/diagnostic/no_mods"_lng;
			break;
		case ModPackageDiagnosticCode::duplicate_target:
			message = wxString::Format("dialog/package_installer/diagnostic/duplicate_target"_lng, detail);
			break;
		case ModPackageDiagnosticCode::target_collision:
			message = wxString::Format("dialog/package_installer/diagnostic/target_collision"_lng, detail);
			break;
		case ModPackageDiagnosticCode::extraction_failed:
			message = "dialog/package_installer/diagnostic/extraction_failed"_lng;
			break;
		case ModPackageDiagnosticCode::invalid_extracted_content:
			message =
				wxString::Format("dialog/package_installer/diagnostic/invalid_extracted_content"_lng, detail);
			break;
		}

		return wxString::Format("dialog/package_installer/diagnostic/package"_lng,
			wxString::FromUTF8(boost::nowide::narrow(diagnostic.package_path.filename().wstring())), message);
	}
}

ModPackageInstallDialog::ModPackageInstallDialog(wxWindow* parent, const ModPackageInspection& inspection)
	: wxDialog(parent, wxID_ANY, "dialog/package_installer/caption"_lng, wxDefaultPosition, wxDefaultSize,
		  wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
	, _inspection(inspection)
{
	_mods = new wxDataViewListCtrl(
		this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_ROW_LINES | wxDV_VERT_RULES | wxDV_SINGLE);
	_mods->AppendToggleColumn(
		"dialog/package_installer/column/selected"_lng, wxDATAVIEW_CELL_ACTIVATABLE, 80, wxALIGN_CENTER);
	_mods->AppendTextColumn("dialog/package_installer/column/mod"_lng, wxDATAVIEW_CELL_INERT, 190);
	_mods->AppendTextColumn("dialog/package_installer/column/directory"_lng, wxDATAVIEW_CELL_INERT, 170);
	_mods->AppendTextColumn("dialog/package_installer/column/package"_lng, wxDATAVIEW_CELL_INERT, 150);
	_mods->AppendTextColumn(
		"dialog/package_installer/column/package_version"_lng, wxDATAVIEW_CELL_INERT, 115);
	_mods->AppendTextColumn(
		"dialog/package_installer/column/installed_version"_lng, wxDATAVIEW_CELL_INERT, 115);
	_mods->AppendTextColumn("dialog/package_installer/column/action"_lng, wxDATAVIEW_CELL_INERT, 100);

	_descriptionGroup = new wxStaticBox(this, wxID_ANY, "dialog/plugin_manager/description"_lng);
	_description      = new wxTextCtrl(_descriptionGroup, wxID_ANY, wxEmptyString, wxDefaultPosition,
		wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_NOHIDESEL);
	_description->SetBackgroundColour(_descriptionGroup->GetBackgroundColour());

	_diagnosticsGroup = new wxStaticBox(this, wxID_ANY, "dialog/package_installer/diagnostics"_lng);
	_diagnostics      = new wxTextCtrl(_diagnosticsGroup, wxID_ANY, wxEmptyString, wxDefaultPosition,
		wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_NOHIDESEL);
	_diagnostics->SetBackgroundColour(_diagnosticsGroup->GetBackgroundColour());

	_install    = new wxButton(this, wxID_OK, "dialog/package_installer/install_selected"_lng);
	auto cancel = new wxButton(this, wxID_CANCEL, "dialog/button/cancel"_lng);
	_install->SetDefault();

	auto descriptionSizer = new wxStaticBoxSizer(_descriptionGroup, wxVERTICAL);
	descriptionSizer->Add(_description, wxSizerFlags(1).Expand().Border(wxALL, 4));

	auto topSizer = new wxBoxSizer(wxHORIZONTAL);
	topSizer->Add(_mods, wxSizerFlags(3).Expand().Border(wxALL, 4));
	topSizer->Add(descriptionSizer, wxSizerFlags(2).Expand().Border(wxALL, 4));

	auto diagnosticsSizer = new wxStaticBoxSizer(_diagnosticsGroup, wxVERTICAL);
	diagnosticsSizer->Add(_diagnostics, wxSizerFlags(1).Expand().Border(wxALL, 4));

	auto buttons = new wxBoxSizer(wxHORIZONTAL);
	buttons->AddStretchSpacer();
	buttons->Add(_install, wxSizerFlags(0).Border(wxALL, 4));
	buttons->Add(cancel, wxSizerFlags(0).Border(wxALL, 4));

	auto layout = new wxBoxSizer(wxVERTICAL);
	layout->Add(topSizer, wxSizerFlags(1).Expand());
	layout->Add(diagnosticsSizer, wxSizerFlags(0).Expand().Border(wxLEFT | wxRIGHT, 4));
	layout->Add(buttons, wxSizerFlags(0).Expand());
	SetSizer(layout);
	SetMinSize(FromDIP(wxSize(980, 560)));
	SetSize(FromDIP(wxSize(1120, 680)));
	CentreOnParent();

	populate();

	_mods->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, [this](wxDataViewEvent&) { updateDescription(); });
	_mods->Bind(wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, [this](wxDataViewEvent& event) {
		const auto row = _mods->ItemToRow(event.GetItem());
		if (row != wxNOT_FOUND && !_inspection.mods[static_cast<std::size_t>(row)].selectable &&
			_mods->GetToggleValue(static_cast<unsigned int>(row), 0))
			_mods->SetToggleValue(false, static_cast<unsigned int>(row), 0);
		updateInstallButton();
	});
}

void ModPackageInstallDialog::populate()
{
	const wxString noVersion = L"\u2014";
	for (const auto& mod : _inspection.mods)
	{
		wxVector<wxVariant> values;
		values.push_back(wxVariant(mod.selectable && mod.selected));
		values.push_back(wxVariant(wxString::FromUTF8(mod.metadata.name)));
		values.push_back(wxVariant(wxString::FromUTF8(mod.directory)));
		values.push_back(
			wxVariant(wxString::FromUTF8(boost::nowide::narrow(mod.package_path.filename().wstring()))));
		values.push_back(
			wxVariant(mod.metadata.version.empty() ? noVersion : wxString::FromUTF8(mod.metadata.version)));
		values.push_back(wxVariant(!mod.installed || mod.installed_version.empty()
									   ? noVersion
									   : wxString::FromUTF8(mod.installed_version)));
		values.push_back(wxVariant(!mod.selectable ? "dialog/package_installer/action/invalid"_lng
								   : mod.installed ? "dialog/package_installer/action/reinstall"_lng
												   : "dialog/package_installer/action/install"_lng));
		_mods->AppendItem(values);
	}

	wxString diagnostics;
	for (const auto& item : _inspection.diagnostics)
	{
		if (!diagnostics.empty())
			diagnostics += L"\n";
		diagnostics += diagnosticText(item);
	}
	_diagnostics->SetValue(diagnostics);
	_diagnosticsGroup->Show(!diagnostics.empty());

	if (!_inspection.mods.empty())
	{
		_mods->SelectRow(0);
		updateDescription();
	}
	updateInstallButton();
	Layout();
}

std::vector<std::size_t> ModPackageInstallDialog::selectedMods() const
{
	std::vector<std::size_t> result;
	for (std::size_t row = 0; row < _inspection.mods.size(); ++row)
		if (_inspection.mods[row].selectable && _mods->GetToggleValue(static_cast<unsigned int>(row), 0))
			result.emplace_back(row);
	return result;
}

void ModPackageInstallDialog::updateDescription()
{
	const auto row = _mods->GetSelectedRow();
	if (row == wxNOT_FOUND || static_cast<std::size_t>(row) >= _inspection.mods.size())
	{
		_description->Clear();
		return;
	}

	_description->SetValue(wxString::FromUTF8(_inspection.mods[static_cast<std::size_t>(row)].description));
}

void ModPackageInstallDialog::updateInstallButton()
{
	_install->Enable(!selectedMods().empty());
}
