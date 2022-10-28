// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "mod_list_view.h"

#include "application.h"
#include "base/mod_conflict_resolver.hpp"
#include "domain/imod_data_provider.hpp"
#include "domain/ipreset_manager.hpp"
#include "domain/mod_data.hpp"
#include "interface/domain/ilocal_config.h"
#include "interface/iapp_config.h"
#include "interface/ilaunch_helper.h"
#include "interface/imod_manager.hpp"
#include "interface/imod_platform.hpp"
#include "interface/service/iicon_storage.h"
#include "manage_preset_list_view.hpp"
#include "mod_list_model.h"
#include "select_exe.h"
#include "types/embedded_icon.h"
#include "utility/sdlexcept.h"
#include "utility/shell_util.h"
#include "wx/priority_data_renderer.h"

#include <fmt/format.h>
#include <wx/app.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/collpane.h>
#include <wx/dataview.h>
#include <wx/msgdlg.h>
#include <wx/notifmsg.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>

#include <algorithm>

using namespace mm;

ModListView::ModListView(wxWindow* parent, IModPlatform& managedPlatform, IIconStorage& iconStorage,
						 wxString const& managedPath)
	: _managedPlatform(managedPlatform)
	, _modManager(*managedPlatform.modManager())
	, _listModel(new ModListModel(*managedPlatform.modDataProvider(), iconStorage,
								  managedPlatform.localConfig()->showHiddenMods()))
	, _iconStorage(iconStorage)
{
	MM_EXPECTS(parent, mm::no_parent_window_error);
	Create(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);

	createControls(managedPath);
	_listModel->setModList(_modManager.mods());
	buildLayout();
	bindEvents();
	updateControlsState();
}

void ModListView::buildLayout()
{
	auto listGroupSizer = new wxBoxSizer(wxVERTICAL);
	listGroupSizer->Add(_list, wxSizerFlags(1).Expand().Border(wxALL, 4));
	listGroupSizer->Add(_checkboxShowHidden, wxSizerFlags(0).Border(wxALL, 4));

	auto buttonSizer = new wxBoxSizer(wxVERTICAL);
	buttonSizer->Add(_moveUp, wxSizerFlags(0).Expand().Border(wxALL, 4));
	buttonSizer->Add(_moveDown, wxSizerFlags(0).Expand().Border(wxALL, 4));
	buttonSizer->Add(_changeState, wxSizerFlags(0).Expand().Border(wxALL, 4));
	buttonSizer->AddStretchSpacer(1);
	buttonSizer->Add(_sort, wxSizerFlags(0).Expand().Border(wxALL, 4));

	auto leftGroupSizer = new wxStaticBoxSizer(_group, wxHORIZONTAL);
	leftGroupSizer->Add(listGroupSizer, wxSizerFlags(1).Expand());
	leftGroupSizer->Add(buttonSizer, wxSizerFlags(0).Expand());

	auto contentSizer = new wxBoxSizer(wxHORIZONTAL);
	contentSizer->Add(leftGroupSizer, wxSizerFlags(168).Expand());
	contentSizer->Add(_modDescription, wxSizerFlags(100).Expand().Border(wxALL, 4));

	this->SetSizer(contentSizer);
}

void ModListView::bindEvents()
{
	_checkboxShowHidden->Bind(wxEVT_CHECKBOX, &ModListView::OnEventCheckboxShowHidden, this);

	_list->Bind(wxEVT_DATAVIEW_COLUMN_SORTED, [=](wxDataViewEvent&) { followSelection(); });

	_list->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED,
				[=](wxDataViewEvent&)
				{
					const auto item = _list->GetSelection();
					_selectedMod    = item.IsOk() ? _listModel->findMod(item)->id : wxString();
					updateControlsState();
				});

	_list->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED,
				[=](wxDataViewEvent&) { onSwitchSelectedModStateRequested(); });

	_list->Bind(wxEVT_DATAVIEW_ITEM_BEGIN_DRAG,
				[=](wxDataViewEvent& event)
				{
					event.Veto();  // default
					const auto item = event.GetItem();

					if (!item.IsOk())
						return;

					if (auto sortingColumn = _list->GetSortingColumn(); !sortingColumn)
						return;
					else if (sortingColumn->GetModelColumn() !=
							 static_cast<unsigned int>(ModListModel::Column::priority))
						return;

					auto id = _listModel->findIdByItem(item);

					if (!_modManager.activePosition(id).has_value())
						return;

					event.Allow();
					event.SetDataObject(new wxTextDataObject(id));
				});

	_list->Bind(wxEVT_DATAVIEW_ITEM_DROP_POSSIBLE,
				[=](wxDataViewEvent& event)
				{
					event.Veto();  // default
					const auto item(event.GetItem());

					if (!item.IsOk())
						return;

					if (!_modManager.activePosition(_listModel->findIdByItem(item)).has_value())
						return;

					event.Allow();
				});

	_list->Bind(wxEVT_DATAVIEW_ITEM_DROP,
				[=](wxDataViewEvent& event)
				{
					if (auto sortingColumn = _list->GetSortingColumn(); !sortingColumn)
						return;
					else if (sortingColumn->GetModelColumn() !=
							 static_cast<unsigned int>(ModListModel::Column::priority))
						return;

					const auto item(event.GetItem());
					if (!item.IsOk())
						return;

					wxTextDataObject from;
					from.SetData(wxDF_UNICODETEXT, event.GetDataSize(), event.GetDataBuffer());

					wxString text = from.GetText();
					if (text.Last() == L'\0')
						text = text.Left(text.Len() - 1);

					_modManager.move(text, _listModel->findIdByItem(item));
				});

	_list->Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU,
				[=](wxDataViewEvent& event)
				{
					const auto item(event.GetItem());
					if (!item.IsOk())
					{
						event.Veto();
						return;
					}

					OnListItemContextMenu(item);
				});

	Bind(wxEVT_MENU, &ModListView::OnMenuItemSelected, this);

	_modManager.onListChanged().connect(
		[this]
		{
			_listModel->setModList(_modManager.mods());
			followSelection();
			updateControlsState();
		});

	_moveUp->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { _modManager.moveUp(_selectedMod); });
	_moveDown->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { _modManager.moveDown(_selectedMod); });
	_changeState->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { onSwitchSelectedModStateRequested(); });
	_sort->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { onSortModsRequested(); });
}

void ModListView::createControls(wxString const& managedPath)
{
	_group = new wxStaticBox(this, wxID_ANY, wxString::Format("Mod list (%s)"_lng, managedPath));

	createListControl();

	_checkboxShowHidden = new wxCheckBox(_group, wxID_ANY, "Show hidden"_lng);
	_checkboxShowHidden->SetValue(_managedPlatform.localConfig()->showHiddenMods());

	_modDescription =
		new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
					   wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2 | wxTE_AUTO_URL | wxTE_BESTWRAP);

	_moveUp = new wxButton(_group, wxID_ANY, "Move Up"_lng);
	_moveUp->SetBitmap(_iconStorage.get(embedded_icon::up));

	_moveDown = new wxButton(_group, wxID_ANY, "Move Down"_lng);
	_moveDown->SetBitmap(_iconStorage.get(embedded_icon::down));

	_changeState = new wxButton(_group, wxID_ANY, "Enable"_lng);
	_changeState->SetBitmap(_iconStorage.get(embedded_icon::plus));

	_sort = new wxButton(_group, wxID_ANY, "Sort"_lng);
	_sort->SetBitmap(_iconStorage.get(embedded_icon::sort));

	_menu.showOrHide     = _menu.menu.Append(wxID_ANY, "placeholder");
	_menu.openHomepage   = _menu.menu.Append(wxID_ANY, "Go to homepage"_lng);
	_menu.openDir        = _menu.menu.Append(wxID_ANY, "Open directory"_lng);
	_menu.deleteOrRemove = _menu.menu.Append(wxID_ANY, "placeholder");
}

void ModListView::createListControl()
{
	_list = new wxDataViewCtrl(_group, wxID_ANY, wxDefaultPosition, wxDefaultSize,
							   wxDV_ROW_LINES | wxDV_VERT_RULES);
	_list->EnableDragSource(wxDF_UNICODETEXT);
	_list->EnableDropTarget(wxDF_UNICODETEXT);
	_list->AssociateModel(_listModel.get());

	createListColumns();
}

void ModListView::createListColumns()
{
	auto r0 = new mmPriorityDataRenderer();
	auto r1 = new wxDataViewIconTextRenderer();
	auto r2 = new wxDataViewTextRenderer();
	auto r3 = new wxDataViewTextRenderer();
	auto r4 = new wxDataViewTextRenderer();

	r0->SetAlignment(wxALIGN_CENTER_VERTICAL);
	r1->SetAlignment(wxALIGN_CENTER_VERTICAL);
	r2->SetAlignment(wxALIGN_CENTER_VERTICAL);
	r3->SetAlignment(wxALIGN_CENTER_VERTICAL);
	r4->SetAlignment(wxALIGN_CENTER_VERTICAL);

	r4->EnableEllipsize(wxELLIPSIZE_END);

	constexpr auto columnFlags =
		wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_REORDERABLE;

	auto column0 = new wxDataViewColumn(" ", r0, static_cast<unsigned int>(ModListModel::Column::priority),
										wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER, columnFlags);
	auto column1 =
		new wxDataViewColumn("Mod"_lng, r1, static_cast<unsigned int>(ModListModel::Column::caption),
							 wxCOL_WIDTH_AUTOSIZE, wxALIGN_LEFT, columnFlags);
	auto column2 =
		new wxDataViewColumn("Category"_lng, r2, static_cast<unsigned int>(ModListModel::Column::category),
							 wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER, columnFlags);
	auto column3 =
		new wxDataViewColumn("Version"_lng, r3, static_cast<unsigned int>(ModListModel::Column::version),
							 wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER, columnFlags);
	auto column4 =
		new wxDataViewColumn("Author"_lng, r4, static_cast<unsigned int>(ModListModel::Column::author),
							 wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER, columnFlags);

	_list->AppendColumn(column0);
	_list->AppendColumn(column1);
	_list->AppendColumn(column2);
	_list->AppendColumn(column3);
	_list->AppendColumn(column4);

	column0->SetSortOrder(true);
}

void ModListView::updateControlsState()
{
	wxLogDebug(__FUNCTION__);

	if (_selectedMod.empty())
	{
		_moveUp->Disable();
		_moveDown->Disable();
		_changeState->Disable();
		_modDescription->SetValue(wxEmptyString);

		return;
	}

	const auto mod = _managedPlatform.modDataProvider()->modData(_selectedMod);

	_changeState->Enable();
	_changeState->SetBitmap(wxNullBitmap);
	_changeState->SetBitmap(_iconStorage.get(
		_modManager.activePosition(mod->id).has_value() ? embedded_icon::minus : embedded_icon::plus));
	_changeState->SetLabelText(_modManager.activePosition(mod->id).has_value() ? "Disable"_lng
																			   : "Enable"_lng);

	_moveUp->Enable(_modManager.canMoveUp(mod->id));
	_moveDown->Enable(_modManager.canMoveDown(mod->id));

	auto description = "No description available"_lng;

	if (mod->virtual_mod)
	{
		description = "This mod is virtual, there is no corresponding directory on disk"_lng;
	}
	else if (auto file = std::ifstream(mod->data_path / mod->full_description); file)
	{
		std::stringstream stream;
		stream << file.rdbuf();
		file.close();

		auto string = wxString::FromUTF8(stream.str());
		if (string.empty())
			string = stream.str();

		if (!string.empty())
			description = std::move(string);
	}
	else if (!mod->short_description.empty())
	{
		description = mod->short_description;
	}

	_modDescription->SetValue(description);

	Layout();
}

void ModListView::followSelection()
{
	wxLogDebug(__FUNCTION__);
	const auto itemToSelect = _listModel->findItemById(_selectedMod);

	if (itemToSelect.IsOk())
	{
		_list->EnsureVisible(itemToSelect);
		_list->Select(itemToSelect);
	}
	else
	{
		_selectedMod.clear();
	}
}

void ModListView::OnListItemContextMenu(const wxDataViewItem& item)
{
	if (const auto mod = _listModel->findMod(item))
	{
		_menu.showOrHide->SetItemLabel(_modManager.mods().hidden.count(mod->id) ? "Show"_lng : "Hide"_lng);
		_menu.openHomepage->Enable(!mod->homepage_link.empty());
		_menu.openDir->Enable(!mod->virtual_mod);
		_menu.deleteOrRemove->SetItemLabel(mod->virtual_mod ? "Remove from list"_lng : "Delete"_lng);
		_list->PopupMenu(&_menu.menu);
	}
}

void ModListView::OnMenuItemSelected(const wxCommandEvent& event)
{
	const auto itemId = event.GetId();

	const auto mod = _listModel->findMod(_list->GetSelection());

	if (itemId == _menu.showOrHide->GetId())
		_modManager.switchVisibility(_selectedMod);
	else if (itemId == _menu.openHomepage->GetId())
		wxLaunchDefaultBrowser(mod->homepage_link);
	else if (itemId == _menu.openDir->GetId())
		wxLaunchDefaultApplication(mod->data_path.string());
	else if (itemId == _menu.deleteOrRemove->GetId())
		onRemoveModRequested();
}

void ModListView::onSwitchSelectedModStateRequested()
{
	try_handle_exceptions(
		this,
		[&]
		{
			if (!_modManager.activePosition(_selectedMod).has_value())
			{
				auto modData = _managedPlatform.modDataProvider()->modData(_selectedMod);

				std::vector<std::string> incompatible;
				for (const auto& item : _modManager.mods().active)
				{
					auto other = _managedPlatform.modDataProvider()->modData(item);
					if (modData->incompatible.count(item) || other->incompatible.count(_selectedMod))
					{
						incompatible.emplace_back('"' + other->caption.ToStdString(wxConvUTF8) + '"');
					}
				}

				if (!incompatible.empty())
				{
					/* const auto message = fmt::format(
						"Mod \"{0}\" is incompatible with {1}.\r\n"
						"Do you really want to enable this mod?"_lng.ToStdString(wxConvUTF8),
						_selectedMod.ToStdString(wxConvUTF8), boost::algorithm::join(incompatible, ", "));

					const auto answer = wxMessageBox(wxString::FromUTF8(message), wxTheApp->GetAppName(),
													 wxYES_NO | wxNO_DEFAULT | wxICON_WARNING);

					if (answer != wxYES)
						return;*/
				}
			}

			_modManager.switchState(_selectedMod);

			if (_managedPlatform.localConfig()->conflictResolveMode() == ConflictResolveMode::automatic)
			{
				onSortModsRequested();

				static bool messageWasShown = false;
				if (!messageWasShown)
				{
					wxNotificationMessage nm("conflicts/caption"_lng, "conflicts/automatic_warning"_lng, this);
					nm.Show();
					messageWasShown = true;
				}
			}
		});
}

void ModListView::OnEventCheckboxShowHidden(const wxCommandEvent&)
{
	try_handle_exceptions(this,
						  [&]
						  {
							  _listModel->showHidden(_checkboxShowHidden->IsChecked());
							  _managedPlatform.localConfig()->showHiddenMods(
								  _checkboxShowHidden->IsChecked());
						  });
}

void ModListView::onSortModsRequested()
{
	wxBusyCursor bc;

	try_handle_exceptions(
		this,
		[&] {
			_modManager.mods(resolve_mod_conflicts(_modManager.mods(), *_managedPlatform.modDataProvider()));
		});
}

void ModListView::onRemoveModRequested()
{
	try_handle_exceptions(this,
						  [&]
						  {
							  auto mod = _managedPlatform.modDataProvider()->modData(_selectedMod);

							  if (!mod->virtual_mod)
							  {
								  const auto formatMessage =
									  "Are you sure want to delete mod \"%s\"?\n\n"
									  "It will be deleted to recycle bin, if possible."_lng;
								  const auto answer = wxMessageBox(
									  wxString::Format(formatMessage, mod->caption), wxTheApp->GetAppName(),
									  wxYES_NO | wxNO_DEFAULT | wxICON_WARNING);

								  if (answer != wxYES)
									  return;

								  if (!shellRemove(mod->data_path.string()))
									  return;
							  }

							  _modManager.remove(mod->id);
						  });
}
