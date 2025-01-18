// SD Mod Manager

// Copyright (c) 2020-2024 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "manage_preset_list_view.hpp"

#include "application.h"
#include "domain/mod_list.hpp"
#include "error_view.h"
#include "export_preset_dialog.hpp"
#include "import_preset_dialog.hpp"
#include "interface/iicon_storage.hpp"
#include "interface/ilaunch_helper.hpp"
#include "interface/ilocal_config.hpp"
#include "interface/imod_manager.hpp"
#include "interface/imod_platform.hpp"
#include "interface/ipreset_manager.hpp"
#include "mod_list_model.h"
#include "type/embedded_icon.h"
#include "utility/sdlexcept.h"
#include "wx/priority_data_renderer.h"

#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/indexed.hpp>
#include <wx/app.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/dir.h>
#include <wx/generic/textdlgg.h>
#include <wx/infobar.h>
#include <wx/listbox.h>
#include <wx/listctrl.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>

#include <format>

using namespace mm;

namespace
{
	wxString showEnterNameDialog(wxWindow* parent, wxString message, wxString caption, wxString name)
	{
		wxTextEntryDialog ted(parent, message, caption, name);
		ted.SetTextValidator(wxTextValidatorStyle::wxFILTER_EMPTY);
		ted.GetTextValidator()->SetCharExcludes(L"\\/:*?\"<>|");

		if (ted.ShowModal() != wxID_OK)
			return {};

		return ted.GetValue();
	}
}

ManagePresetListView::ManagePresetListView(
	wxWindow* parent, IModPlatform& platform, IIconStorage& iconStorage)
	: wxPanel(parent, wxID_ANY)
	, _platform(platform)
	, _selected(platform.localConfig()->getAcitvePreset())
	, _listModel(new ModListModel(*platform.modDataProvider(), iconStorage))
	, _iconStorage(iconStorage)
{
	MM_EXPECTS(parent, mm::unexpected_error);

	createControls();

	updateLayout();
	bindEvents();
	refreshListContent();
}

void ManagePresetListView::refreshListContent()
{
	_list->DeleteAllItems();
	_profiles.clear();

	const auto list = _platform.getPresetManager()->list();
	for (const auto& preset : list)
	{
		wxVector<wxVariant> data;
		data.push_back(wxVariant(wxDataViewIconText(wxString::FromUTF8(preset),
			_iconStorage.get(preset == _selected ? embedded_icon::tick : embedded_icon::blank))));
		_list->AppendItem(data);

		_profiles.emplace_back(preset);

		if (preset == _selected)
			_list->SelectRow(_list->GetItemCount() - 1);
	}

	onSelectionChanged();
}

void ManagePresetListView::createControls()
{
	_presets = new wxStaticBox(this, wxID_ANY, "Profiles"_lng);

	_list = new wxDataViewListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxDV_HORIZ_RULES | wxDV_VERT_RULES | wxDV_ROW_LINES);
	_list->AppendIconTextColumn("Profile"_lng, wxDATAVIEW_CELL_INERT);

	_load = new wxButton(_presets, wxID_ANY, "Load"_lng);
	_save = new wxButton(_presets, wxID_ANY, "Save"_lng);

	_export = new wxButton(_presets, wxID_ANY, "Export"_lng);
	_import = new wxButton(_presets, wxID_ANY, "Import"_lng);

	_rename = new wxButton(_presets, wxID_ANY, "Rename"_lng);
	_copy   = new wxButton(_presets, wxID_ANY, "Copy"_lng);

	_remove = new wxButton(_presets, wxID_ANY, "Delete"_lng);

	_preview = new wxStaticBox(this, wxID_ANY, "Preview"_lng);

	_mods = new wxDataViewCtrl(_preview, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxDV_ROW_LINES | wxDV_VERT_RULES | wxDV_NO_HEADER);
	_mods->AssociateModel(_listModel.get());

	_infoBar = new wxInfoBar(this);
	_infoBarTimer.SetOwner(this);

	createListColumns();
}

void ManagePresetListView::createListColumns()
{
	auto r0 = new mmPriorityDataRenderer(FromDIP(32));
	auto r1 = new wxDataViewIconTextRenderer();

	r0->SetAlignment(wxALIGN_CENTER_VERTICAL);
	r1->SetAlignment(wxALIGN_CENTER_VERTICAL);

	auto column0 = new wxDataViewColumn(L" ", r0, static_cast<unsigned int>(ModListModelColumn::priority),
		wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER);
	auto column1 = new wxDataViewColumn("Mod"_lng, r1, static_cast<unsigned int>(ModListModelColumn::name),
		wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER);

	_mods->AppendColumn(column0);
	_mods->AppendColumn(column1);

	column0->SetSortOrder(true);
}

void ManagePresetListView::updateLayout()
{
	auto manageControls = new wxBoxSizer(wxVERTICAL);
	manageControls->Add(_load, wxSizerFlags(1).Expand().Border(wxALL, 5));
	manageControls->Add(_save, wxSizerFlags(1).Expand().Border(wxALL, 5));
	manageControls->AddSpacer(16);
	manageControls->Add(_export, wxSizerFlags(1).Expand().Border(wxALL, 5));
	manageControls->Add(_import, wxSizerFlags(1).Expand().Border(wxALL, 5));
	manageControls->AddSpacer(16);
	manageControls->Add(_rename, wxSizerFlags(1).Expand().Border(wxALL, 5));
	manageControls->Add(_copy, wxSizerFlags(1).Expand().Border(wxALL, 5));
	manageControls->AddSpacer(16);
	manageControls->Add(_remove, wxSizerFlags(1).Expand().Border(wxALL, 5));

	auto midControls = new wxStaticBoxSizer(_presets, wxHORIZONTAL);
	midControls->Add(_list, wxSizerFlags(1).Expand().Border(wxALL, 5));
	midControls->Add(manageControls, wxSizerFlags(0));

	auto previewGroup = new wxStaticBoxSizer(_preview, wxVERTICAL);
	previewGroup->Add(_mods, wxSizerFlags(3).Expand().Border(wxALL, 5));

	auto mainSizer = new wxBoxSizer(wxHORIZONTAL);
	mainSizer->Add(midControls, wxSizerFlags(2).Expand().Border(wxALL, 5));
	mainSizer->Add(previewGroup, wxSizerFlags(3).Expand().Border(wxALL, 5));

	auto vertSizer = new wxBoxSizer(wxVERTICAL);
	vertSizer->Add(mainSizer, wxSizerFlags(1).Expand());
	vertSizer->Add(_infoBar, wxSizerFlags(0).Expand());

	SetSizer(vertSizer);
	Layout();
}

void ManagePresetListView::bindEvents()
{
	_platform.getPresetManager()->onListChanged().connect([=] { refreshListContent(); });

	_list->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, [=](wxDataViewEvent&) { onLoadPresetRequested(); });
	_list->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, [=](wxDataViewEvent&) { onSelectionChanged(); });

	_load->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { onLoadPresetRequested(); });
	_save->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { onSavePresetRequested(getSelection()); });

	_export->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { onExportPresetRequested(getSelection()); });
	_import->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { onImportPresetRequested(); });

	_rename->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { onRenamePreset(); });
	_copy->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { onCopyPreset(); });

	_remove->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { onDeletePreset(); });

	Bind(wxEVT_TIMER, [=](wxTimerEvent&) { _infoBar->Dismiss(); });
}

void ManagePresetListView::onSavePresetRequested(std::string baseName)
{
	EX_TRY;

	if (baseName.empty())
		baseName =
			showEnterNameDialog(this, "Enter profile name"_lng, "Create"_lng, wxString::FromUTF8(baseName))
				.ToStdString(wxConvUTF8);

	if (baseName.empty())
		return;

	if (_platform.getPresetManager()->exists(baseName))
	{
		int const answer = wxMessageBox(
			wxString::Format("'%s' already exists, overwrite?"_lng, wxString::FromUTF8(baseName)),
			wxTheApp->GetAppName(), wxYES_NO | wxNO_DEFAULT);

		if (answer != wxYES)
			return;
	}

	_platform.getPresetManager()->savePreset(
		baseName, { _platform.modManager()->mods().enabled(), _platform.launchHelper()->getExecutable() });
	_platform.localConfig()->setActivePreset(baseName);
	_selected = baseName;

	refreshListContent();

	EX_ON_EXCEPTION(fs::filesystem_error, onFilesystemError);
	EX_UNEXPECTED;
}

void ManagePresetListView::onExportPresetRequested(std::string baseName)
{
	EX_TRY;

	ExportPresetDialog epf(this, _platform, _iconStorage, baseName);
	epf.ShowModal();

	EX_UNEXPECTED;
}

void ManagePresetListView::onImportPresetRequested()
{
	EX_TRY;

	ImportPresetDialog epf(this, _platform, _iconStorage);
	epf.ShowModal();

	refreshListContent();

	EX_UNEXPECTED;
}

void ManagePresetListView::onLoadPresetRequested()
{
	EX_TRY;

	auto selected = getSelection();
	auto preset   = _platform.getPresetManager()->loadPreset(selected);

	_platform.modManager()->apply(preset.mods);
	_platform.localConfig()->setActivePreset(selected);

	_selected = selected;

	refreshListContent();

	_infoBar->ShowMessage(wxString::Format("Profile \"%s\" loaded."_lng, wxString::FromUTF8(selected)));
	_infoBarTimer.StartOnce(5000);

	EX_ON_EXCEPTION(fs::filesystem_error, onFilesystemError);
	EX_UNEXPECTED;
}

void ManagePresetListView::onRenamePreset()
{
	EX_TRY;

	const auto selected = getSelection();
	const auto newName =
		showEnterNameDialog(this, "Enter profile name"_lng, "Rename"_lng, wxString::FromUTF8(selected))
			.ToStdString(wxConvUTF8);

	if (newName.empty())
		return;

	if (_platform.getPresetManager()->exists(newName))
	{
		_infoBar->ShowMessage(
			wxString::Format("Profile \"%s\" already exists."_lng, wxString::FromUTF8(newName)));
		_infoBarTimer.StartOnce(5000);
		return;
	}

	_platform.getPresetManager()->rename(selected, newName);
	_selected = newName;

	EX_ON_EXCEPTION(fs::filesystem_error, onFilesystemError);
	EX_UNEXPECTED;
}

void ManagePresetListView::onCopyPreset()
{
	EX_TRY;

	const auto selected = getSelection();
	const auto newName =
		showEnterNameDialog(this, "Enter profile name"_lng, "Copy"_lng, wxString::FromUTF8(selected))
			.ToStdString(wxConvUTF8);

	if (newName.empty())
		return;

	if (_platform.getPresetManager()->exists(newName))
	{
		_infoBar->ShowMessage(
			wxString::Format("Profile \"%s\" already exists."_lng, wxString::FromUTF8(newName)));
		_infoBarTimer.StartOnce(5000);
		return;
	}

	_platform.getPresetManager()->copy(selected, newName);

	EX_ON_EXCEPTION(fs::filesystem_error, onFilesystemError);
	EX_UNEXPECTED;
}

void ManagePresetListView::onDeletePreset()
{
	EX_TRY;

	auto      selected = getSelection();
	const int answer =
		wxMessageBox(wxString::Format(wxString("Delete profile '%s'?"_lng), wxString::FromUTF8(selected)),
			wxTheApp->GetAppName(), wxYES_NO | wxNO_DEFAULT);

	if (answer == wxYES)
		_platform.getPresetManager()->remove(selected);

	EX_ON_EXCEPTION(fs::filesystem_error, onFilesystemError);
	EX_UNEXPECTED;
}

void ManagePresetListView::onFilesystemError(const fs::filesystem_error& e)
{
	wxMessageOutputMessageBox().Printf(
		"Error happened during execution of operation. "
		"Details:\n\n %s"_lng,
		wxString::FromUTF8(e.what()));
}

void ManagePresetListView::updatePreview()
{
	wxBusyCursor bc;

	EX_TRY;

	auto selected = getSelection();

	PresetData preset;
	if (!selected.empty())
	{
		preset = _platform.getPresetManager()->loadPreset(selected);
		// mods.available = _platform.modManager()->mods().available;
	}

	_listModel->modList(ModList(preset.mods));

	EX_UNEXPECTED;
}

std::string ManagePresetListView::getSelection() const
{
	if (auto index = _list->GetSelectedRow(); index != wxNOT_FOUND)
		return _profiles.at(index);

	return {};
}

void ManagePresetListView::onSelectionChanged()
{
	auto selected = getSelection();

	updatePreview();

	_save->SetLabel(selected.empty() ? "Save as"_lng : "Save"_lng);

	_load->Enable(!selected.empty());
	_rename->Enable(!selected.empty());
	_copy->Enable(!selected.empty());
	_remove->Enable(!selected.empty());
}
