// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "manage_preset_list_view.hpp"

#include "application.h"
#include "interface/iplugin_manager.hpp"
#include "interface/ipreset_manager.hpp"
#include "domain/mod_list.hpp"
#include "error_view.h"
#include "interface/ilocal_config.h"
#include "interface/imod_manager.hpp"
#include "interface/imod_platform.hpp"
#include "interface/iicon_storage.h"
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
#include <wx/dir.h>
#include <wx/generic/textdlgg.h>
#include <wx/listbox.h>
#include <wx/listctrl.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>

using namespace mm;

ManagePresetListView::ManagePresetListView(wxWindow* parent, IModPlatform& platform,
										   IIconStorage& iconStorage)
	: wxPanel(parent, wxID_ANY)
	, _platform(platform)
	, _selected(platform.localConfig()->getAcitvePreset())
	, _listModel(new ModListModel(*platform.modDataProvider(), iconStorage, true))
	, _pluginListModel(new PluginListModel(*platform.modDataProvider(), iconStorage))
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
		data.push_back(wxVariant(wxDataViewIconText(
			preset, _iconStorage.get(preset == _selected ? embedded_icon::tick : embedded_icon::blank))));
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

	_new  = new wxButton(_presets, wxID_ANY, "Save as"_lng);
	_load = new wxButton(_presets, wxID_ANY, "Load"_lng);
	// _rename = new wxButton(_presets, wxID_ANY, "Rename"_lng);
	// _copy   = new wxButton(_presets, wxID_ANY, "Copy"_lng);
	_remove = new wxButton(_presets, wxID_ANY, "Delete"_lng);

	_preview = new wxStaticBox(this, wxID_ANY, "Preview"_lng);

	_mods = new wxDataViewCtrl(_preview, wxID_ANY, wxDefaultPosition, wxDefaultSize,
							   wxDV_ROW_LINES | wxDV_VERT_RULES | wxDV_NO_HEADER);
	_mods->AssociateModel(_listModel.get());

	_plugins = new wxDataViewCtrl(_preview, wxID_ANY, wxDefaultPosition, wxDefaultSize,
								  wxDV_ROW_LINES | wxDV_VERT_RULES | wxDV_NO_HEADER);
	_plugins->AssociateModel(_pluginListModel.get());

	createListColumns();
	createPluginsListColumns();
}

void ManagePresetListView::createListColumns()
{
	auto r0 = new mmPriorityDataRenderer();
	auto r1 = new wxDataViewIconTextRenderer();

	r0->SetAlignment(wxALIGN_CENTER_VERTICAL);
	r1->SetAlignment(wxALIGN_CENTER_VERTICAL);

	auto column0 = new wxDataViewColumn(" ", r0, static_cast<unsigned int>(ModListModel::Column::priority),
										wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER);
	auto column1 =
		new wxDataViewColumn("Mod"_lng, r1, static_cast<unsigned int>(ModListModel::Column::caption),
							 wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER);

	_mods->AppendColumn(column0);
	_mods->AppendColumn(column1);

	column0->SetSortOrder(true);
}

void ManagePresetListView::createPluginsListColumns()
{
	auto r0 = new mmPriorityDataRenderer();
	auto r1 = new wxDataViewTextRenderer();

	r0->SetAlignment(wxALIGN_CENTER_VERTICAL);
	r1->SetAlignment(wxALIGN_CENTER_VERTICAL);

	constexpr auto columnFlags =
		wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_REORDERABLE;

	auto column0 = new wxDataViewColumn("", r0, static_cast<unsigned int>(PluginListModel::Column::state),
										wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER, columnFlags);

	auto column1 =
		new wxDataViewColumn("Plugin"_lng, r1, static_cast<unsigned int>(PluginListModel::Column::caption),
							 wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER, columnFlags);

	_plugins->AppendColumn(column0);
	_plugins->AppendColumn(column1);

	column1->SetSortOrder(true);
}

void ManagePresetListView::updateLayout()
{
	auto manageControls = new wxBoxSizer(wxVERTICAL);
	manageControls->Add(_new, wxSizerFlags(1).Expand().Border(wxALL, 5));
	manageControls->Add(_load, wxSizerFlags(1).Expand().Border(wxALL, 5));
	manageControls->AddSpacer(16);
	// manageControls->Add(_rename, wxSizerFlags(1).Expand().Border(wxALL, 5));
	// manageControls->Add(_copy, wxSizerFlags(1).Expand().Border(wxALL, 5));
	manageControls->Add(_remove, wxSizerFlags(1).Expand().Border(wxALL, 5));

	auto midControls = new wxStaticBoxSizer(_presets, wxHORIZONTAL);
	midControls->Add(_list, wxSizerFlags(1).Expand().Border(wxALL, 5));
	midControls->Add(manageControls, wxSizerFlags(0));

	auto previewGroup = new wxStaticBoxSizer(_preview, wxVERTICAL);
	previewGroup->Add(_mods, wxSizerFlags(3).Expand().Border(wxALL, 5));
	previewGroup->Add(_plugins, wxSizerFlags(1).Expand().Border(wxALL, 5));

	auto mainSizer = new wxBoxSizer(wxHORIZONTAL);
	mainSizer->Add(midControls, wxSizerFlags(2).Expand().Border(wxALL, 5));
	mainSizer->Add(previewGroup, wxSizerFlags(3).Expand().Border(wxALL, 5));

	SetSizer(mainSizer);
	Layout();
}

void ManagePresetListView::bindEvents()
{
	_new->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { onSavePresetRequested(getSelection()); });
	_load->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { onLoadPresetRequested(); });
	//_rename->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { onRenamePreset(); });
	//_copy->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { onCopyPreset(); });
	_remove->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { onDeletePreset(); });
	_list->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, [=](wxDataViewEvent&) { onSelectionChanged(); });

	_mods->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, [=](wxDataViewEvent&) { onLoadPresetRequested(); });

	_platform.getPresetManager()->onListChanged().connect([=] { refreshListContent(); });
}

void ManagePresetListView::onSavePresetRequested(wxString baseName)
{
	try_handle_exceptions(
		this,
		[&]
		{
			if (baseName.empty())
			{
				wxTextEntryDialog ted(this, "Enter profile name"_lng, "Create"_lng, baseName);
				ted.SetTextValidator(wxTextValidatorStyle::wxFILTER_EMPTY);
				ted.GetTextValidator()->SetCharExcludes("\\/:*?\"<>|");

				if (ted.ShowModal() != wxID_OK)
					return;

				baseName = ted.GetValue();
			}

			if (_platform.getPresetManager()->exists(baseName))
			{
				int const answer =
					wxMessageBox(wxString::Format(wxString("'%s' already exists, overwrite?"_lng), baseName),
								 wxTheApp->GetAppName(), wxYES_NO | wxNO_DEFAULT);

				if (answer != wxYES)
					return;
			}

			_platform.getPresetManager()->savePreset(baseName, _platform.modManager()->mods(),
													 _platform.pluginManager()->plugins().overridden);
			_platform.localConfig()->setActivePreset(baseName);
			_selected = baseName;

			refreshListContent();
			// EX_ON_EXCEPTION(std::filesystem::filesystem_error, onFilesystemError);
		});
}

void ManagePresetListView::onLoadPresetRequested()
{
	try_handle_exceptions(this,
						  [&]
						  {
							  auto selected           = getSelection();
							  auto [mods, overridden] = _platform.getPresetManager()->loadPreset(selected);

							  mods.available = _platform.modManager()->mods().available;
							  mods.invalid   = _platform.modManager()->mods().invalid;

							  _platform.modManager()->mods(std::move(mods));

							  auto currentPlugins = _platform.pluginManager()->plugins();
							  currentPlugins.replaceOverridenState(overridden);
							  _platform.pluginManager()->plugins(currentPlugins);
							  _platform.localConfig()->setActivePreset(selected);
							  _selected = selected;

							  refreshListContent();
						  });
}

void ManagePresetListView::onRenamePreset()
{
	EX_TRY;

	const wxString    selected = getSelection();
	wxTextEntryDialog ted(this, "Enter profile name"_lng, "Rename"_lng, selected);
	const wxString    newName = ted.ShowModal() == wxID_OK ? ted.GetValue() : wxString();

	if (!_platform.getPresetManager()->exists(newName))
	{
		_platform.getPresetManager()->rename(selected.ToStdString(), newName.ToStdString());
		_selected = newName;
	}

	EX_ON_EXCEPTION(std::filesystem::filesystem_error, onFilesystemError);
	EX_UNEXPECTED;
}

void ManagePresetListView::onCopyPreset()
{
	EX_TRY;

	const wxString    selected = getSelection();
	wxTextEntryDialog ted(this, "Enter profile name"_lng, "Copy"_lng, selected);
	const wxString    newName = ted.ShowModal() == wxID_OK ? ted.GetValue() : wxString();

	if (!newName.empty() && !_platform.getPresetManager()->list().count(newName))
		_platform.getPresetManager()->copy(selected.ToStdString(), newName.ToStdString());

	EX_ON_EXCEPTION(std::filesystem::filesystem_error, onFilesystemError);
	EX_UNEXPECTED;
}

void ManagePresetListView::onDeletePreset()
{
	EX_TRY;

	auto      selected = getSelection();
	const int answer   = wxMessageBox(wxString::Format(wxString("Delete profile '%s'?"_lng), selected),
									  wxTheApp->GetAppName(), wxYES_NO | wxNO_DEFAULT);

	if (answer == wxYES)
		_platform.getPresetManager()->remove(selected);

	EX_ON_EXCEPTION(std::filesystem::filesystem_error, onFilesystemError);
	EX_UNEXPECTED;
}

void ManagePresetListView::onFilesystemError(const std::filesystem::filesystem_error& e)
{
	wxMessageOutputBest().Printf(
		"Error happened during execution of operation."
		"Details:\n\n %s"_lng,
		e.what());
}

void ManagePresetListView::updatePreview()
{
	wxBusyCursor bc;

	try_handle_exceptions(this,
						  [&]
						  {
							  auto selected = getSelection();

							  ModList                                   mods;
							  std::unordered_map<wxString, PluginState> plugins;
							  if (!selected.empty())
							  {
								  std::tie(mods, plugins) =
									  _platform.getPresetManager()->loadPreset(selected);
								  // mods.available = _platform.modManager()->mods().available;
							  }

							  _listModel->setModList(mods);
							  PluginList pluginsData;
							  _platform.pluginManager()->updateBaseState(pluginsData, mods);
							  pluginsData.available.clear();
							  pluginsData.replaceOverridenState(plugins);
							  _pluginListModel->setList(pluginsData);

							  _plugins->Show(!pluginsData.overridden.empty());
							  Layout();
						  });
}

wxString ManagePresetListView::getSelection() const
{
	if (auto index = _list->GetSelectedRow(); index != wxNOT_FOUND)
		return _profiles.at(index);

	return {};
}

void ManagePresetListView::onSelectionChanged()
{
	auto selected = getSelection();

	updatePreview();

	_new->SetLabel(selected.empty() ? "Save as"_lng : "Overwrite"_lng);

	_load->Enable(!selected.empty());
	//_rename->Enable(!selected.empty());
	//_copy->Enable(!selected.empty());
	_remove->Enable(!selected.empty());
}
