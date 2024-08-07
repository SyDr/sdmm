// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "mod_list_view.h"

#include "application.h"
#include "domain/mod_conflict_resolver.hpp"
#include "domain/mod_data.hpp"
#include "image_gallery_view.hpp"
#include "interface/iapp_config.hpp"
#include "interface/iicon_storage.hpp"
#include "interface/ilocal_config.hpp"
#include "interface/imod_data_provider.hpp"
#include "interface/imod_manager.hpp"
#include "interface/imod_platform.hpp"
#include "interface/ipreset_manager.hpp"
#include "manage_preset_list_view.hpp"
#include "mod_list_model.h"
#include "select_exe.h"
#include "type/embedded_icon.h"
#include "utility/fs_util.h"
#include "utility/sdlexcept.h"
#include "utility/shell_util.h"
#include "wx/priority_data_renderer.h"

#include <cmark.h>
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
#include <wx/webview.h>

#include <algorithm>

using namespace mm;

ModListView::ModListView(
	wxWindow* parent, IModPlatform& managedPlatform, IIconStorage& iconStorage, bool groupMods)
	: _managedPlatform(managedPlatform)
	, _modManager(*managedPlatform.modManager())
	, _listModel(new ModListModel(*managedPlatform.modDataProvider(), iconStorage,
		  managedPlatform.localConfig()->showHiddenMods(), groupMods))
	, _iconStorage(iconStorage)
{
	MM_EXPECTS(parent, mm::no_parent_window_error);
	Create(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);

	createControls(wxString::FromUTF8(managedPlatform.managedPath().string()));
	_listModel->setModList(_modManager.mods());
	expandChildren();
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

	auto rightBottomSizer = new wxBoxSizer(wxHORIZONTAL);
	rightBottomSizer->AddStretchSpacer();
	rightBottomSizer->Add(_showGallery, wxSizerFlags(0).Border(wxALL, 4));
	rightBottomSizer->Add(_openGallery, wxSizerFlags(0).Border(wxALL, 4));
	rightBottomSizer->Add(_expandGallery, wxSizerFlags(0).Border(wxALL, 4));

	auto rightSizer = new wxBoxSizer(wxVERTICAL);
	rightSizer->Add(_modDescription, wxSizerFlags(1).Expand().Border(wxALL, 4));
	rightSizer->Add(_modDescriptionPlain, wxSizerFlags(1).Expand().Border(wxALL, 4));
	rightSizer->Add(rightBottomSizer, wxSizerFlags(0).Expand());
	rightSizer->Add(_galleryView, wxSizerFlags(0).Expand().Border(wxALL, 4));

	auto contentSizer = new wxBoxSizer(wxHORIZONTAL);
	contentSizer->Add(leftGroupSizer, wxSizerFlags(168).Expand());
	contentSizer->Add(rightSizer, wxSizerFlags(100).Expand().Border(wxALL, 4));

	this->SetSizer(contentSizer);
}

void ModListView::bindEvents()
{
	_checkboxShowHidden->Bind(wxEVT_CHECKBOX, &ModListView::OnEventCheckboxShowHidden, this);

	_list->Bind(wxEVT_DATAVIEW_COLUMN_SORTED, [=](wxDataViewEvent&) { followSelection(); });

	_list->Bind(wxEVT_DATAVIEW_ITEM_COLLAPSING, [=](wxDataViewEvent& event) {
		if (auto item = _listModel->categoryByItem(event.GetItem()); item.has_value())
			_hiddenCategories.emplace(*item);
		else
			event.Veto();
	});

	_list->Bind(wxEVT_DATAVIEW_ITEM_EXPANDING, [=](wxDataViewEvent& event) {
		if (auto item = _listModel->categoryByItem(event.GetItem()); item.has_value())
			_hiddenCategories.erase(*item);
	});

	_list->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, [=](wxDataViewEvent&) {
		const auto item = _list->GetSelection();
		const auto mod  = _listModel->findMod(item);
		_selectedMod    = mod ? mod->id : "";
		updateControlsState();
	});

	_list->Bind(
		wxEVT_DATAVIEW_ITEM_ACTIVATED, [=](wxDataViewEvent&) { onSwitchSelectedModStateRequested(); });

	_list->Bind(wxEVT_DATAVIEW_ITEM_BEGIN_DRAG, [=](wxDataViewEvent& event) {
		auto moveFrom = _listModel->findIdByItem(event.GetItem());
		if (moveFrom.empty())
		{
			event.Veto();
			return;
		}

		event.SetDataObject(new wxTextDataObject(wxString::FromUTF8(moveFrom)));
		event.SetDragFlags(wxDrag_DefaultMove);
	});

	_list->Bind(wxEVT_DATAVIEW_ITEM_DROP_POSSIBLE, [=](wxDataViewEvent& event) {
		if (!event.GetItem().IsOk())
		{
			event.Veto();
			return;
		}

		auto moveTo = _listModel->findIdByItem(event.GetItem());
		if (moveTo.empty())
		{
			event.Veto();
			return;
		}

		if (!_modManager.activePosition(_selectedMod).has_value() &&
			!_modManager.activePosition(moveTo).has_value())
			event.Veto();
	});

	_list->Bind(wxEVT_DATAVIEW_ITEM_DROP, [=](wxDataViewEvent& event) {
		wxTextDataObject from;
		from.SetData(wxDF_UNICODETEXT, event.GetDataSize(), event.GetDataBuffer());

		const auto moveFrom = from.GetText().utf8_string();
		_modManager.move(moveFrom, _listModel->findIdByItem(event.GetItem()));
	});

	_list->Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, [=](wxDataViewEvent& event) {
		if (!event.GetItem().IsOk())
		{
			event.Veto();
			return;
		}

		OnListItemContextMenu(event.GetItem());
	});

	Bind(wxEVT_MENU, &ModListView::OnMenuItemSelected, this);

	_modManager.onListChanged().connect([this] {
		_listModel->setModList(_modManager.mods());

		expandChildren();
		followSelection();
		updateControlsState();
	});

	_moveUp->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { _modManager.moveUp(_selectedMod); });
	_moveDown->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { _modManager.moveDown(_selectedMod); });
	_changeState->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { onSwitchSelectedModStateRequested(); });
	_sort->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { onSortModsRequested({}); });

	_openGallery->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { openGalleryRequested(); });

	_showGallery->Bind(
		wxEVT_BUTTON, [=](wxCommandEvent&) { updateGalleryState(!_galleryShown, _galleryExpanded); });
	_expandGallery->Bind(
		wxEVT_BUTTON, [=](wxCommandEvent&) { updateGalleryState(_galleryShown, !_galleryExpanded); });
}

void ModListView::createControls(const wxString& managedPath)
{
	_group = new wxStaticBox(this, wxID_ANY, wxString::Format("Mod list (%s)"_lng, managedPath));

	createListControl();

	_checkboxShowHidden = new wxCheckBox(_group, wxID_ANY, "Show hidden"_lng);
	_checkboxShowHidden->SetValue(_managedPlatform.localConfig()->showHiddenMods());

	_modDescription = wxWebView::New();
	_modDescription->Create(this, wxID_ANY, wxString(), wxDefaultPosition, wxDefaultSize);
	_modDescription->EnableContextMenu(false);
	_modDescription->EnableHistory(false);
	_modDescription->SetPage(L"", L"");
	_modDescription->Hide();

	_modDescriptionPlain = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
		wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2 | wxTE_AUTO_URL | wxTE_BESTWRAP);

	_moveUp = new wxButton(_group, wxID_ANY, "Move Up"_lng);
	_moveUp->SetBitmap(_iconStorage.get(embedded_icon::up));

	_moveDown = new wxButton(_group, wxID_ANY, "Move Down"_lng);
	_moveDown->SetBitmap(_iconStorage.get(embedded_icon::down));

	_changeState = new wxButton(_group, wxID_ANY, "Enable"_lng);
	_changeState->SetBitmap(_iconStorage.get(embedded_icon::plus));

	_sort = new wxButton(_group, wxID_ANY, "Sort"_lng);
	_sort->SetBitmap(_iconStorage.get(embedded_icon::sort));

	_menu.showOrHide     = _menu.menu.Append(wxID_ANY, L"placeholder");
	_menu.openHomepage   = _menu.menu.Append(wxID_ANY, "Go to homepage"_lng);
	_menu.openDir        = _menu.menu.Append(wxID_ANY, "Open directory"_lng);
	_menu.deleteOrRemove = _menu.menu.Append(wxID_ANY, L"placeholder");

	_showGallery = new wxButton(this, wxID_ANY, "Screenshots"_lng);
	_showGallery->SetBitmap(_iconStorage.get(embedded_icon::double_up));

	wxSize goodSize = _showGallery->GetBestSize();
	goodSize.SetWidth(goodSize.GetHeight());

	_openGallery = new wxButton(this, wxID_ANY, wxEmptyString, wxDefaultPosition, goodSize, wxBU_EXACTFIT);
	_openGallery->SetBitmap(_iconStorage.get(embedded_icon::folder));

	_expandGallery = new wxButton(this, wxID_ANY, wxEmptyString, wxDefaultPosition, goodSize, wxBU_EXACTFIT);
	_expandGallery->SetBitmap(_iconStorage.get(embedded_icon::maximize));

	_galleryView = new ImageGalleryView(this, wxID_ANY);
	_galleryView->Show(_galleryShown);
}

void ModListView::createListControl()
{
	_list = new wxDataViewCtrl(
		_group, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_ROW_LINES | wxDV_VERT_RULES);
	_list->EnableDragSource(wxDF_UNICODETEXT);
	_list->EnableDropTarget(wxDF_UNICODETEXT);
	_list->AssociateModel(_listModel.get());

	createListColumns();
}

void ModListView::createListColumns()
{
	auto rActivity  = new wxDataViewIconTextRenderer();
	auto rLoadOrder = new wxDataViewTextRenderer();

	auto r1 = new wxDataViewIconTextRenderer();
	auto r2 = new wxDataViewTextRenderer();
	auto r3 = new wxDataViewTextRenderer();
	auto r4 = new wxDataViewTextRenderer();

	rActivity->SetAlignment(wxALIGN_CENTER_VERTICAL);
	rLoadOrder->SetAlignment(wxALIGN_CENTER_VERTICAL);

	r1->SetAlignment(wxALIGN_CENTER_VERTICAL);
	r2->SetAlignment(wxALIGN_CENTER_VERTICAL);
	r3->SetAlignment(wxALIGN_CENTER_VERTICAL);
	r4->SetAlignment(wxALIGN_CENTER_VERTICAL);

	r4->EnableEllipsize(wxELLIPSIZE_END);

	constexpr auto columnFlags =
		wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_REORDERABLE;

	auto columnActivity =
		new wxDataViewColumn("Status"_lng, rActivity, static_cast<unsigned int>(ModListModel::Column::status),
			wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER, columnFlags);
	auto columnLoadOrder = new wxDataViewColumn("Load order"_lng, rLoadOrder,
		static_cast<unsigned int>(ModListModel::Column::load_order), wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER,
		columnFlags);

	auto column1 = new wxDataViewColumn("Mod"_lng, r1, static_cast<unsigned int>(ModListModel::Column::name),
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

	_list->AppendColumn(columnActivity);
	_list->AppendColumn(column1);
	_list->AppendColumn(column2);
	_list->AppendColumn(columnLoadOrder);
	_list->AppendColumn(column3);
	_list->AppendColumn(column4);

	columnActivity->SetSortOrder(true);
}

void ModListView::updateControlsState()
{
	// wxLogDebug(__FUNCTION__);

	EX_TRY;

	if (_selectedMod.empty())
	{
		_moveUp->Disable();
		_moveDown->Disable();
		_changeState->Disable();
		_modDescription->SetPage(L"", L"");
		_modDescriptionPlain->SetValue(L"");
		_openGallery->Disable();
		_galleryView->Reset();

		return;
	}

	const auto mod = _managedPlatform.modDataProvider()->modData(_selectedMod);

	_changeState->Enable();
	_changeState->SetBitmap(wxNullBitmap);
	_changeState->SetBitmap(_iconStorage.get(
		_modManager.activePosition(mod.id).has_value() ? embedded_icon::minus : embedded_icon::plus));
	_changeState->SetLabelText(_modManager.activePosition(mod.id).has_value() ? "Disable"_lng : "Enable"_lng);

	_moveUp->Enable(_modManager.canMoveUp(mod.id));
	_moveDown->Enable(_modManager.canMoveDown(mod.id));

	bool useRichDescription = false;
	auto description = "No description available"_lng;

	if (mod.virtual_mod)
	{
		description = "This mod is virtual, there is no corresponding directory on disk"_lng;
	}
	else if (auto desc = readFile(mod.data_path / mod.description); !desc.empty())
	{
		if (mod.description.extension() == ".md")
		{
			auto cnvt = std::unique_ptr<char, decltype(&std::free)>(
				cmark_markdown_to_html(desc.c_str(), desc.size(), 0), &std::free);

			desc = cnvt.get();
			useRichDescription = true;
		}

		auto asString = wxString::FromUTF8(desc);

		if (asString.empty())
			asString = wxString(desc.c_str(), wxConvLocal, desc.size());

		if (!asString.empty())
			std::swap(asString, description);
	}

	if (useRichDescription)
	{
		_modDescription->Show();
		_modDescription->SetPage(description, L"");
		_modDescriptionPlain->Hide();
	}
	else
	{
		_modDescriptionPlain->Show();
		_modDescriptionPlain->SetValue(description);
		_modDescription->Hide();
	}
	_openGallery->Enable(fs::exists(mod.data_path / "Screens"));
	_galleryView->SetPath(mod.data_path / "Screens");

	Layout();

	EX_UNEXPECTED;
}

void ModListView::expandChildren()
{
	wxDataViewItemArray children;
	_listModel->GetChildren(wxDataViewItem(), children);

	for (const auto& item : children)
	{
		auto cat = _listModel->categoryByItem(item);

		if (!cat.has_value() || !_hiddenCategories.contains(*cat))
			_list->ExpandChildren(item);
	}
}

void ModListView::followSelection()
{
	// wxLogDebug(__FUNCTION__);
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
		_menu.openHomepage->Enable(!mod->homepage.empty());
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
		wxLaunchDefaultBrowser(wxString::FromUTF8(mod->homepage));
	else if (itemId == _menu.openDir->GetId())
		wxLaunchDefaultApplication(wxString::FromUTF8(mod->data_path.string()));
	else if (itemId == _menu.deleteOrRemove->GetId())
		onRemoveModRequested();
}

void ModListView::onSwitchSelectedModStateRequested()
{
	EX_TRY;

	if (_selectedMod.empty())
		return;

	if (!_modManager.activePosition(_selectedMod).has_value())
	{
		const auto& modData = _managedPlatform.modDataProvider()->modData(_selectedMod);

		std::vector<std::string> incompatible;
		for (const auto& item : _modManager.mods().active)
		{
			auto other = _managedPlatform.modDataProvider()->modData(item);
			if (modData.incompatible.contains(item) || other.incompatible.contains(_selectedMod))
			{
				incompatible.emplace_back('"' + other.name + '"');
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
		onSortModsRequested(_modManager.mods().isActive(_selectedMod) ? std::string() : _selectedMod);

		static bool messageWasShown = false;
		if (!messageWasShown)
		{
			wxNotificationMessage nm("conflicts/caption"_lng, "conflicts/automatic_warning"_lng, this);
			nm.Show();
			messageWasShown = true;
		}
	}

	EX_UNEXPECTED;
}

void ModListView::OnEventCheckboxShowHidden(const wxCommandEvent&)
{
	EX_TRY;

	_listModel->showHidden(_checkboxShowHidden->IsChecked());
	_managedPlatform.localConfig()->showHiddenMods(_checkboxShowHidden->IsChecked());

	expandChildren();
	followSelection();
	updateControlsState();

	EX_UNEXPECTED;
}

void ModListView::onSortModsRequested(const std::string& disablingMod)
{
	wxBusyCursor bc;

	EX_TRY;

	auto mods = resolve_mod_conflicts(_modManager.mods(), *_managedPlatform.modDataProvider(), disablingMod);
	if (mods != _modManager.mods())
		_managedPlatform.apply(&mods);

	EX_UNEXPECTED;
}

void ModListView::onRemoveModRequested()
{
	EX_TRY;

	auto mod = _managedPlatform.modDataProvider()->modData(_selectedMod);

	if (!mod.virtual_mod)
	{
		const auto formatMessage =
			"Are you sure want to delete mod \"%s\"?\n\n"
			"It will be deleted to recycle bin, if possible."_lng;
		const auto answer = wxMessageBox(wxString::Format(formatMessage, wxString::FromUTF8(mod.name)),
			wxTheApp->GetAppName(), wxYES_NO | wxNO_DEFAULT | wxICON_WARNING);

		if (answer != wxYES)
			return;

		if (!shellRemove(wxString::FromUTF8(mod.data_path.string())))
			return;
	}

	_modManager.remove(mod.id);

	EX_UNEXPECTED;
}

void ModListView::openGalleryRequested()
{
	EX_TRY;

	auto mod = _managedPlatform.modDataProvider()->modData(_selectedMod);

	wxLaunchDefaultApplication(wxString::FromUTF8((mod.data_path / "Screens").string()));

	EX_UNEXPECTED;
}

void ModListView::updateGalleryState(bool show, bool expand)
{
	EX_TRY;

	if (!show && _galleryExpanded)
		expand = false;

	if (expand && !_galleryShown)
		show = true;

	_galleryShown    = show;
	_galleryExpanded = expand;

	_showGallery->SetBitmap(
		_iconStorage.get(show || expand ? embedded_icon::double_down : embedded_icon::double_up));
	_galleryView->Show(show || expand);
	_galleryView->Expand(expand);

	if (auto topWindow = dynamic_cast<wxTopLevelWindow*>(wxTheApp->GetTopWindow()))
		topWindow->ShowFullScreen(expand, wxFULLSCREEN_ALL);

	Layout();

	EX_UNEXPECTED;
}
