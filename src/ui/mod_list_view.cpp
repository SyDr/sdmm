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
#include <wx/infobar.h>
#include <wx/msgdlg.h>
#include <wx/notifmsg.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/webview.h>

#include <algorithm>

using namespace mm;

ModListView::ModListView(
	wxWindow* parent, IModPlatform& managedPlatform, IIconStorage& iconStorage, ModListModelMode listMode)
	: _managedPlatform(managedPlatform)
	, _modManager(*managedPlatform.modManager())
	, _listModel(new ModListModel(*managedPlatform.modDataProvider(), iconStorage, listMode))
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

	auto buttonSizer = new wxBoxSizer(wxVERTICAL);
	buttonSizer->Add(_moveUp, wxSizerFlags(0).Border(wxALL, 4));
	buttonSizer->Add(_moveDown, wxSizerFlags(0).Border(wxALL, 4));
	buttonSizer->Add(_changeState, wxSizerFlags(0).Border(wxALL, 4));
	buttonSizer->Add(_resetState, wxSizerFlags(0).Border(wxALL, 4));
	buttonSizer->AddStretchSpacer(1);
	buttonSizer->Add(_sort, wxSizerFlags(0).Border(wxALL, 4));

	auto leftGroupSizer = new wxStaticBoxSizer(_group, wxHORIZONTAL);
	leftGroupSizer->Add(buttonSizer, wxSizerFlags(0).Expand());
	leftGroupSizer->Add(listGroupSizer, wxSizerFlags(1).Expand());

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

	auto vertSizer = new wxBoxSizer(wxVERTICAL);
	vertSizer->Add(contentSizer, wxSizerFlags(1).Expand());
	vertSizer->Add(_infoBar, wxSizerFlags(0).Expand());

	this->SetSizer(vertSizer);
}

void ModListView::bindEvents()
{
	_list->Bind(wxEVT_DATAVIEW_COLUMN_SORTED, [=](wxDataViewEvent&) { followSelection(); });

	_list->Bind(wxEVT_DATAVIEW_ITEM_COLLAPSING, [=](wxDataViewEvent& event) {
		if (auto item = _listModel->itemGroupByItem(event.GetItem()); item.has_value())
			_hiddenCategories.emplace(*item);
		else
			event.Veto();
	});

	_list->Bind(wxEVT_DATAVIEW_ITEM_EXPANDING, [=](wxDataViewEvent& event) {
		if (auto item = _listModel->itemGroupByItem(event.GetItem()); item.has_value())
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

		if (!_modManager.mods().canMove(_selectedMod, moveTo))
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
	_resetState->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { onResetSelectedModStateRequested(); });
	_sort->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { onSortModsRequested({}, {}); });

	_openGallery->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { openGalleryRequested(); });

	_showGallery->Bind(
		wxEVT_BUTTON, [=](wxCommandEvent&) { updateGalleryState(!_galleryShown, _galleryExpanded); });
	_expandGallery->Bind(
		wxEVT_BUTTON, [=](wxCommandEvent&) { updateGalleryState(_galleryShown, !_galleryExpanded); });

	Bind(wxEVT_TIMER, [=](wxTimerEvent&) { _infoBar->Dismiss(); });
}

void ModListView::createControls(const wxString& managedPath)
{
	_group = new wxStaticBox(this, wxID_ANY, wxString::Format("Mod list (%s)"_lng, managedPath));

	createListControl();

	_modDescription = wxWebView::New();
	_modDescription->Create(this, wxID_ANY, wxString(), wxDefaultPosition, wxDefaultSize);
	_modDescription->EnableContextMenu(false);
	_modDescription->EnableHistory(false);
	_modDescription->SetPage(L"", L"");
	_modDescription->Hide();

	_modDescriptionPlain = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
		wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2 | wxTE_AUTO_URL | wxTE_BESTWRAP);

	_moveUp = new wxBitmapButton(_group, wxID_ANY, _iconStorage.get(embedded_icon::up), wxDefaultPosition,
		{ FromDIP(24), FromDIP(24) }, wxBU_EXACTFIT);
	_moveUp->SetToolTip("Move Up"_lng);

	_moveDown = new wxBitmapButton(_group, wxID_ANY, _iconStorage.get(embedded_icon::down), wxDefaultPosition,
		{ FromDIP(24), FromDIP(24) }, wxBU_EXACTFIT);
	_moveDown->SetToolTip("Move Down"_lng);

	_changeState = new wxBitmapButton(_group, wxID_ANY, _iconStorage.get(embedded_icon::tick_green),
		wxDefaultPosition, { FromDIP(24), FromDIP(24) }, wxBU_EXACTFIT);
	_changeState->SetToolTip("Enable"_lng);

	_resetState = new wxBitmapButton(_group, wxID_ANY, _iconStorage.get(embedded_icon::reset_position),
		wxDefaultPosition, { FromDIP(24), FromDIP(24) }, wxBU_EXACTFIT);
	_resetState->SetToolTip("Archive"_lng);

	_sort = new wxBitmapButton(_group, wxID_ANY, _iconStorage.get(embedded_icon::sort), wxDefaultPosition,
		{ FromDIP(24), FromDIP(24) }, wxBU_EXACTFIT);
	_sort->SetToolTip("Sort"_lng);

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

	_infoBar = new wxInfoBar(this);
	_infoBarTimer.SetOwner(this);
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
	auto rPriority = new mmPriorityDataRenderer();

	auto r1             = new wxDataViewIconTextRenderer();
	auto r2             = new wxDataViewTextRenderer();
	auto r3             = new wxDataViewTextRenderer();
	auto r4             = new wxDataViewTextRenderer();
	auto rDirectoryName = new wxDataViewTextRenderer();

	rPriority->SetAlignment(wxALIGN_CENTER_VERTICAL);

	r1->SetAlignment(wxALIGN_CENTER_VERTICAL);
	r2->SetAlignment(wxALIGN_CENTER_VERTICAL);
	r3->SetAlignment(wxALIGN_CENTER_VERTICAL);
	r4->SetAlignment(wxALIGN_CENTER_VERTICAL);
	rDirectoryName->SetAlignment(wxALIGN_CENTER_VERTICAL);

	rDirectoryName->EnableEllipsize(wxELLIPSIZE_END);

	auto columnPriority = new wxDataViewColumn("Priority"_lng, rPriority,
		static_cast<unsigned int>(ModListModel::Column::priority), wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER,
		wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_REORDERABLE);
	auto column1 = new wxDataViewColumn("Mod"_lng, r1, static_cast<unsigned int>(ModListModel::Column::name),
		wxCOL_WIDTH_AUTOSIZE, wxALIGN_LEFT,
		wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_REORDERABLE);
	auto column2 = new wxDataViewColumn("Category"_lng, r2,
		static_cast<unsigned int>(ModListModel::Column::category), wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER,
		wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_REORDERABLE);
	auto column3 =
		new wxDataViewColumn("Version"_lng, r3, static_cast<unsigned int>(ModListModel::Column::version),
			wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER, wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_REORDERABLE);
	auto column4             = new wxDataViewColumn("Author"_lng, r4,
					static_cast<unsigned int>(ModListModel::Column::author), wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER,
					wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_REORDERABLE);
	auto columnDirectoryName = new wxDataViewColumn("Directory"_lng, rDirectoryName,
		static_cast<unsigned int>(ModListModel::Column::directory), wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER,
		wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE | wxDATAVIEW_COL_REORDERABLE);

	_list->AppendColumn(columnPriority);
	_list->AppendColumn(column1);
	_list->AppendColumn(column2);
	_list->AppendColumn(column3);
	_list->AppendColumn(column4);
	_list->AppendColumn(columnDirectoryName);

	columnPriority->SetSortOrder(true);
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
		_resetState->Disable();
		_modDescription->SetPage(L"", L"");
		_modDescriptionPlain->SetValue(L"");
		_openGallery->Disable();
		_galleryView->Reset();

		return;
	}

	const auto& mod = _managedPlatform.modDataProvider()->modData(_selectedMod);

	_changeState->Enable();
	_changeState->SetBitmap(wxNullBitmap);
	_changeState->SetBitmap(_iconStorage.get(
		_modManager.mods().enabled(mod.id) ? embedded_icon::cross_gray : embedded_icon::tick_green));
	_changeState->SetToolTip(_modManager.mods().enabled(mod.id) ? "Disable"_lng : "Enable"_lng);

	_resetState->Enable(_modManager.mods().position(mod.id).has_value());

	_moveUp->Enable(_modManager.mods().canMoveUp(mod.id));
	_moveDown->Enable(_modManager.mods().canMoveDown(mod.id));

	bool useRichDescription = false;
	auto description        = "No description available"_lng;

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

			desc               = cnvt.get();
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
		auto cat = _listModel->itemGroupByItem(item);

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

	if (itemId == _menu.openHomepage->GetId())
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

	_modManager.switchState(_selectedMod);

	if (_managedPlatform.localConfig()->conflictResolveMode() == ConflictResolveMode::automatic)
	{
		const auto& enabling  = _modManager.mods().enabled(_selectedMod) ? _selectedMod : std::string();
		const auto& disabling = _modManager.mods().enabled(_selectedMod) ? std::string() : _selectedMod;

		onSortModsRequested(enabling, disabling);

		static bool messageWasShown = false;
		if (!messageWasShown)
		{
			_infoBar->ShowMessage(wxString::Format("Automatic mod conflict resolve mode selected"_lng));
			_infoBarTimer.StartOnce(5000);
			messageWasShown = true;
		}
	}

	EX_UNEXPECTED;
}

void ModListView::onResetSelectedModStateRequested()
{
	EX_TRY;

	if (_selectedMod.empty())
		return;

	_modManager.archive(_selectedMod);

	EX_UNEXPECTED;
}

void ModListView::onSortModsRequested(const std::string& enablingMod, const std::string& disablingMod)
{
	wxBusyCursor bc;

	EX_TRY;

	auto mods = ResolveModConflicts(
		_modManager.mods(), *_managedPlatform.modDataProvider(), enablingMod, disablingMod);
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
