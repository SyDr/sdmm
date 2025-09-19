// SD Mod Manager

// Copyright (c) 2025 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "edit_mod_dialog.hpp"

#include "application.h"
#include "domain/mod_data.hpp"
#include "icon_helper.hpp"
#include "interface/iapp_config.hpp"
#include "interface/iicon_storage.hpp"
#include "interface/imod_data_provider.hpp"
#include "interface/imod_platform.hpp"
#include "mod_list_model.h"
#include "mod_manager_app.h"
#include "system_info.hpp"
#include "type/icon.hpp"
#include "utility/fs_util.h"
#include "utility/sdlexcept.h"
#include "wx/data_view_multiple_icons_renderer.h"
#include "wx/priority_data_renderer.h"

#include <boost/algorithm/string.hpp>
#include <cmark.h>
#include <shlobj_core.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/dataview.h>
#include <wx/dir.h>
#include <wx/dirctrl.h>
#include <wx/html/htmlwin.h>
#include <wx/imaglist.h>
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>

#include <algorithm>

using namespace mm;

EditModDialog::EditModDialog(wxWindow* parent, IModPlatform& managedPlatform, const std::string& name)
	: wxDialog(parent, wxID_ANY, "Edit mod"_lng + L" " + wxString::FromUTF8(name), wxDefaultPosition,
		  wxSize(1280, 720), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
	, _managedPlatform(managedPlatform)
	, _basePath(managedPlatform.managedPath() / "Mods")
	, _modName(name)
{
	createControls();
	buildLayout();
	bindEvents();
	loadData();
}

void EditModDialog::createControls()
{
	_modOptionsGroup = new wxStaticBox(this, wxID_ANY, "Mod"_lng);
	_modDataEdit     = new wxTextCtrl(
        _modOptionsGroup, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);

	_editTimer.SetOwner(this);

	_docsGroup = new wxStaticBox(this, wxID_ANY, "Docs"_lng);

	_docHtmlWindow = new wxHtmlWindow(_docsGroup);
	_docHtmlWindow->SetHTMLBackgroundColour(_docsGroup->GetBackgroundColour());

	_openFolder = new wxButton(this, wxID_ANY, "Open directory"_lng);
	_reload     = new wxButton(this, wxID_ANY, "Reload from disk"_lng);
	_save       = new wxButton(this, wxID_ANY, "Save"_lng);
	_status     = new wxStaticText(this, wxID_ANY, "OK"_lng);

	_close = new wxButton(this, wxID_ANY, "Close"_lng);
}

void EditModDialog::bindEvents()
{
	_modDataEdit->Bind(wxEVT_TEXT, [this](wxCommandEvent&) {
		if (_editTimer.IsRunning())
			_editTimer.Stop();

		_editTimer.StartOnce(2000);
	});

	Bind(wxEVT_TIMER, [this](wxTimerEvent&) { OnDataEditTextChanged(); });

	_openFolder->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) {
		wxLaunchDefaultApplication(wxString::FromUTF8((_basePath / _modName).string()));
	});

	_reload->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { reloadDataRequested(); });

	_save->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { saveRequested(); });

	_close->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { EndDialog(wxOK); });
}

void EditModDialog::reloadDataRequested()
{
	auto str = readFile(_basePath / _modName / SystemInfo::ModInfoFilename);

	_modDataEdit->SetValue(wxString::FromUTF8(str));
}

void EditModDialog::saveRequested()
{
	const auto data = _modDataEdit->GetValue().ToStdString(wxConvUTF8);

	overwriteFile(_basePath / _modName / SystemInfo::ModInfoFilename, data);
	_managedPlatform.reload(true);
}

void EditModDialog::OnDataEditTextChanged()
{
	const auto text = _modDataEdit->GetValue().ToStdString(wxConvUTF8);

	try
	{
		[[maybe_unused]] const auto parsed = nlohmann::json::parse(text);
		_status->SetLabelText("OK"_lng);
	}
	catch (const nlohmann::json::parse_error& pe)
	{
		_status->SetLabelText(wxString::FromUTF8(pe.what()));
	}
}

void EditModDialog::buildLayout()
{
	auto sizer2 = new wxStaticBoxSizer(_modOptionsGroup, wxVERTICAL);
	sizer2->Add(_modDataEdit, wxSizerFlags(1).Expand().Border(wxALL, 4));

	auto sizer3 = new wxStaticBoxSizer(_docsGroup, wxVERTICAL);
	sizer3->Add(_docHtmlWindow, wxSizerFlags(1).Expand().Border(wxALL, 4));

	auto sizer4 = new wxBoxSizer(wxHORIZONTAL);
	sizer4->Add(_openFolder, wxSizerFlags(0).Expand().Border(wxALL, 4));
	sizer4->Add(_reload, wxSizerFlags(0).Expand().Border(wxALL, 4));
	sizer4->Add(_save, wxSizerFlags(0).Expand().Border(wxALL, 4));
	sizer4->AddStretchSpacer();
	sizer4->Add(_close, wxSizerFlags(0).Expand().Border(wxALL, 4));

	auto sizer5 = new wxBoxSizer(wxHORIZONTAL);
	sizer5->Add(sizer2, wxSizerFlags(1).Expand().Border(wxALL, 4));
	sizer5->Add(sizer3, wxSizerFlags(1).Expand().Border(wxALL, 4));

	auto mainSizer = new wxBoxSizer(wxVERTICAL);
	mainSizer->Add(sizer5, wxSizerFlags(1).Expand().Border(wxALL, 4));
	mainSizer->Add(_status, wxSizerFlags(0).Expand().Border(wxALL, 4));
	mainSizer->Add(sizer4, wxSizerFlags(0).Expand().Border(wxALL, 4));

	SetSizer(mainSizer);
}

void EditModDialog::loadData()
{
	auto str = readFile(wxGetApp().appConfig().programPath() / SystemInfo::DocDir / SystemInfo::ModInfoDoc);

	auto cnvt = std::unique_ptr<char, decltype(&std::free)>(
		cmark_markdown_to_html(str.c_str(), str.size(), CMARK_OPT_DEFAULT), &std::free);

	str = cnvt.get();

	_docHtmlWindow->SetPage(wxString::FromUTF8(str));
	_docHtmlWindow->Scroll(0, 0);
	_docHtmlWindow->SetHTMLBackgroundColour(_docsGroup->GetBackgroundColour());

	reloadDataRequested();
}
