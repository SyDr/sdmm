// SD Mod Manager

// Copyright (c) 2020-2024 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "mod_list_view.h"

#include "application.h"
#include "configure_main_list_view.h"
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
#include "mod_manager_app.h"
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
#include <wx/combo.h>
#include <wx/dataview.h>
#include <wx/infobar.h>
#include <wx/msgdlg.h>
#include <wx/notifmsg.h>
#include <wx/sizer.h>
#include <wx/srchctrl.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/webview.h>

#include <algorithm>

class mmCheckListBoxComboPopup : public wxCheckListBox, public wxComboPopup
{
public:
	void Init() override {}

	bool Create(wxWindow* parent) override
	{
		return wxCheckListBox::Create(parent, wxID_ANY, wxPoint(0, 0), wxDefaultSize);
	}

	wxWindow* GetControl() override
	{
		return this;
	}

	void SetStringValue(const wxString&) override {}

	wxString GetStringValue() const override
	{
		wxArrayInt selections;
		GetCheckedItems(selections);

		return "Categories:"_lng + wxString::FromUTF8(std::format(" {}/{}", selections.size(), GetCount()));
	}

	wxSize GetAdjustedSize(int minWidth, int, int) override
	{
		auto res = GetBestSize();

		if (res.x < minWidth)
			res.x = minWidth;

		return res;
	}

	void OnKeyUp(wxKeyEvent& event)
	{
		if (event.GetRawKeyCode() == VK_ESCAPE ||
			(event.GetRawKeyCode() == VK_F4 && GetComboCtrl()->IsPopupShown()))
		{
			Dismiss();
			event.Skip();
		}
		else
		{
			event.Skip();
		}
	}

private:
	wxDECLARE_EVENT_TABLE();
};

wxBEGIN_EVENT_TABLE(mmCheckListBoxComboPopup, wxCheckListBox) EVT_KEY_UP(mmCheckListBoxComboPopup::OnKeyUp)
	wxEND_EVENT_TABLE();

using namespace mm;

ModListView::ModListView(
	wxWindow* parent, IModPlatform& managedPlatform, IIconStorage& iconStorage, wxStatusBar* statusBar)
	: _managedPlatform(managedPlatform)
	, _modManager(*managedPlatform.modManager())
	, _listModel(new ModListModel(*managedPlatform.modDataProvider(), iconStorage,
		  managedPlatform.localConfig()->managedModsDisplay(),
		  managedPlatform.localConfig()->archivedModsDisplay()))
	, _iconStorage(iconStorage)
	, _statusBar(statusBar)
	, _collapsedCategories(managedPlatform.localConfig()->collapsedCategories())
	, _hiddenCategories(managedPlatform.localConfig()->hiddenCategories())
{
	MM_EXPECTS(parent, mm::no_parent_window_error);
	MM_PRECONDTION(statusBar);

	Create(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);

	createControls(wxString::FromUTF8(managedPlatform.managedPath().string()));
	_listModel->modList(_modManager.mods());
	_listModel->applyCategoryFilter(_hiddenCategories);
	expandChildren();
	buildLayout();
	bindEvents();
	updateControlsState();
	updateCategoryFilterContent();
}

void ModListView::buildLayout()
{
	auto filterSizer = new wxBoxSizer(wxHORIZONTAL);
	filterSizer->Add(_filterText, wxSizerFlags(1).Expand().Border(wxALL, 4));
	filterSizer->Add(_filterCategory, wxSizerFlags(0).Expand().Border(wxALL, 4));

	auto listGroupSizer = new wxBoxSizer(wxVERTICAL);
	listGroupSizer->Add(filterSizer, wxSizerFlags(0).Expand());
	listGroupSizer->Add(_list, wxSizerFlags(1).Expand().Border(wxALL, 4));

	auto buttonSizer = new wxBoxSizer(wxVERTICAL);
	buttonSizer->Add(_configure, wxSizerFlags(0).Border(wxALL, 4));
	buttonSizer->Add(_moveUp, wxSizerFlags(0).Border(wxALL, 4));
	buttonSizer->Add(_moveDown, wxSizerFlags(0).Border(wxALL, 4));
	buttonSizer->Add(_changeState, wxSizerFlags(0).Border(wxALL, 4));
	buttonSizer->AddStretchSpacer(1);
	buttonSizer->Add(_sort, wxSizerFlags(0).Border(wxALL, 4));

	auto leftGroupSizer = new wxStaticBoxSizer(_group, wxHORIZONTAL);
	leftGroupSizer->Add(buttonSizer, wxSizerFlags(0).Expand());
	leftGroupSizer->Add(listGroupSizer, wxSizerFlags(1).Expand());

	auto rightBottomSizer = new wxBoxSizer(wxHORIZONTAL);
	rightBottomSizer->AddStretchSpacer();
	rightBottomSizer->Add(_showGallery, wxSizerFlags(0).Border(wxALL, 4));
	rightBottomSizer->Add(_openGallery, wxSizerFlags(0).Border(wxALL, 4));

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
	_filterText->Bind(wxEVT_TEXT, [=](wxCommandEvent& event) {
		const auto str = event.GetString();
		_listModel->applyFilter(str.ToStdString(wxConvUTF8));
		expandChildren();
		followSelection();
		updateControlsState();

		_filterText->ShowCancelButton(!str.IsEmpty());

		event.Skip();
	});

	_filterPopup->Bind(wxEVT_CHECKLISTBOX, [=](wxCommandEvent&) {
		_filterPopup->GetComboCtrl()->SetValueByUser(_filterPopup->GetStringValue());

		wxArrayInt            selections;
		std::set<std::string> selected;

		_filterPopup->GetCheckedItems(selections);
		for (const auto& i : selections)
			selected.emplace(_categories[i]);

		_hiddenCategories = { _categories.cbegin(), _categories.cend() };
		for (const auto& item : selected)
			_hiddenCategories.erase(item);

		_listModel->applyCategoryFilter(_hiddenCategories);
		_managedPlatform.localConfig()->hiddenCategories(_hiddenCategories);

		expandChildren();
		followSelection();
		updateControlsState();
	});

	_list->Bind(wxEVT_DATAVIEW_COLUMN_SORTED, [=](wxDataViewEvent&) { followSelection(); });

	_list->Bind(wxEVT_DATAVIEW_ITEM_COLLAPSING, [=](wxDataViewEvent& event) {
		if (auto item = _listModel->itemGroupByItem(event.GetItem()); item.has_value())
		{
			_collapsedCategories.emplace(*item);
			_managedPlatform.localConfig()->collapsedCategories(_collapsedCategories);
		}
		else
			event.Veto();
	});

	_list->Bind(wxEVT_DATAVIEW_ITEM_EXPANDING, [=](wxDataViewEvent& event) {
		if (auto item = _listModel->itemGroupByItem(event.GetItem()); item.has_value())
		{
			_collapsedCategories.erase(*item);
			_managedPlatform.localConfig()->collapsedCategories(_collapsedCategories);
		}
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
		_listModel->modList(_modManager.mods());

		expandChildren();
		followSelection();
		updateControlsState();
		updateCategoryFilterContent();
	});

	_configure->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) {
		const auto columns  = _managedPlatform.localConfig()->listColumns();
		const auto managed  = _managedPlatform.localConfig()->managedModsDisplay();
		const auto archived = _managedPlatform.localConfig()->archivedModsDisplay();

		ConfigureMainListView dialog(this, _iconStorage, columns, managed, archived);

		if (dialog.ShowModal() != wxID_OK)
			return;

		const auto newColumns  = dialog.getColumns();
		const auto newManaged  = dialog.getManagedMode();
		const auto newArchived = dialog.getArchivedMode();

		if (columns != newColumns)
		{
			_managedPlatform.localConfig()->listColumns(newColumns);
			createListColumns();
		}

		if (newManaged != managed || newArchived != archived)
		{
			_managedPlatform.localConfig()->managedModsDisplay(newManaged);
			_managedPlatform.localConfig()->archivedModsDisplay(newArchived);

			_listModel->setManagedModsDisplay(newManaged);
			_listModel->setArchivedModsDisplay(newArchived);

			expandChildren();
			followSelection();
			updateControlsState();
		}
	});

	_moveUp->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { _modManager.moveUp(_selectedMod); });
	_moveDown->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { _modManager.moveDown(_selectedMod); });
	_changeState->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { onSwitchSelectedModStateRequested(); });

	_sort->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { onSortModsRequested({}, {}); });

	_modDescriptionPlain->Bind(wxEVT_TEXT_URL, [=](wxTextUrlEvent& event) {
		if (!event.GetMouseEvent().ButtonDClick(wxMOUSE_BTN_LEFT))
		{
			event.Skip();
			return;
		}

		const auto url = _modDescriptionPlain->GetRange(event.GetURLStart(), event.GetURLEnd());
		wxLaunchDefaultBrowser(url);
	});

	_openGallery->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { openGalleryRequested(); });

	_showGallery->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { updateGalleryState(!_galleryShown); });

	Bind(wxEVT_TIMER, [=](wxTimerEvent&) { _infoBar->Dismiss(); });

	Bind(wxEVT_CHAR_HOOK, [=](wxKeyEvent& event) {
		if (!event.ControlDown() || event.GetKeyCode() != 'F')
		{
			event.Skip();
			return;
		}

		_filterText->SetFocusFromKbd();
	});
}

void ModListView::createControls(const wxString& managedPath)
{
	_group = new wxStaticBox(this, wxID_ANY, wxString::Format("Mod list (%s)"_lng, managedPath));

	_filterText = new wxSearchCtrl(this, wxID_ANY);
	_filterText->SetDescriptiveText("Filter"_lng);

	_filterCategory = new wxComboCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
		{ GetTextExtent("Categories:"_lng).x + GetTextExtent(wxString::FromUTF8(" 99/99")).x + FromDIP(32),
			-1 },
		wxCB_READONLY);

	_filterPopup = new mmCheckListBoxComboPopup();
	_filterCategory->SetPopupControl(_filterPopup);

	createListControl();

	_modDescription = wxWebView::New();
	_modDescription->Create(this, wxID_ANY);
	_modDescription->EnableContextMenu(false);
	_modDescription->EnableHistory(false);
	_modDescription->SetPage(L"", L"");
	_modDescription->Hide();

	_modDescriptionPlain = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
		wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2 | wxTE_AUTO_URL | wxTE_BESTWRAP | wxTE_NOHIDESEL);

	_configure = new wxBitmapButton(_group, wxID_ANY, _iconStorage.get(embedded_icon::cog), wxDefaultPosition,
		{ FromDIP(24), FromDIP(24) }, wxBU_EXACTFIT);
	_configure->SetToolTip("Configure view"_lng);

	_moveUp = new wxBitmapButton(_group, wxID_ANY, _iconStorage.get(embedded_icon::up), wxDefaultPosition,
		{ FromDIP(24), FromDIP(24) }, wxBU_EXACTFIT);
	_moveUp->SetToolTip("Move Up"_lng);

	_moveDown = new wxBitmapButton(_group, wxID_ANY, _iconStorage.get(embedded_icon::down), wxDefaultPosition,
		{ FromDIP(24), FromDIP(24) }, wxBU_EXACTFIT);
	_moveDown->SetToolTip("Move Down"_lng);

	_changeState = new wxBitmapButton(_group, wxID_ANY, _iconStorage.get(embedded_icon::tick_green),
		wxDefaultPosition, { FromDIP(24), FromDIP(24) }, wxBU_EXACTFIT);
	_changeState->SetToolTip("Enable"_lng);

	_sort = new wxBitmapButton(_group, wxID_ANY, _iconStorage.get(embedded_icon::sort), wxDefaultPosition,
		{ FromDIP(24), FromDIP(24) }, wxBU_EXACTFIT);
	_sort->SetToolTip("Sort"_lng);

	_menu.openHomepage   = _menu.menu.Append(wxID_ANY, "Go to homepage"_lng);
	_menu.openDir        = _menu.menu.Append(wxID_ANY, "Open directory"_lng);
	_menu.archive        = _menu.menu.Append(wxID_ANY, "Archive"_lng);
	_menu.deleteOrRemove = _menu.menu.Append(wxID_ANY, L"placeholder");

	_showGallery = new wxButton(this, wxID_ANY, "Screenshots"_lng);
	_showGallery->SetBitmap(_iconStorage.get(embedded_icon::double_down));

	wxSize goodSize = _showGallery->GetBestSize();
	goodSize.SetWidth(goodSize.GetHeight());

	_openGallery = new wxButton(this, wxID_ANY, wxEmptyString, wxDefaultPosition, goodSize, wxBU_EXACTFIT);
	_openGallery->SetBitmap(_iconStorage.get(embedded_icon::folder));

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
	_list->ClearColumns();

	auto columns = _managedPlatform.localConfig()->listColumns();
	std::erase_if(columns, [](const int v) { return v < 0; });

	for (size_t i = 0; i < columns.size(); ++i)
	{
		const auto column = columns[i];
		const auto typed  = static_cast<ModListModelColumn>(column);

		wxWidgetsPtr<wxDataViewRenderer> r = nullptr;
		switch (typed)
		{
			using enum ModListModelColumn;
		case ModListModelColumn::name: r = new wxDataViewIconTextRenderer(); break;
		case ModListModelColumn::priority: r = new mmPriorityDataRenderer(); break;
		default: r = new wxDataViewTextRenderer(); break;
		}

		r->SetAlignment(wxALIGN_CENTER_VERTICAL);
		if (i == columns.size() - 1)
			r->EnableEllipsize(wxELLIPSIZE_END);

		int flags = wxDATAVIEW_COL_RESIZABLE;  // | wxDATAVIEW_COL_REORDERABLE; // TODO: give it back and
											   // allow sorting on main window instead

		if (typed != ModListModelColumn::version)
			flags |= wxDATAVIEW_COL_SORTABLE;

		auto c = new wxDataViewColumn(wxString::FromUTF8(wxGetApp().translationString(to_string(typed))), r,
			column, wxCOL_WIDTH_AUTOSIZE, wxALIGN_LEFT, flags);

		_list->AppendColumn(c);

		if (typed == ModListModelColumn::priority)
			c->SetSortOrder(true);
	}
}

void ModListView::updateControlsState()
{
	// wxLogDebug(__FUNCTION__);

	EX_TRY;

	_statusBar->SetStatusText(_listModel->status());

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

	const auto& mod = _managedPlatform.modDataProvider()->modData(_selectedMod);

	_changeState->Enable();
	_changeState->SetBitmap(wxNullBitmap);
	_changeState->SetBitmap(_iconStorage.get(
		_modManager.mods().enabled(mod.id) ? embedded_icon::cross_gray : embedded_icon::tick_green));
	_changeState->SetToolTip(_modManager.mods().enabled(mod.id) ? "Disable"_lng : "Enable"_lng);

	_moveUp->Enable(_modManager.mods().canMoveUp(mod.id));
	_moveDown->Enable(_modManager.mods().canMoveDown(mod.id));

	bool useRichDescription = false;
	auto description        = "No description available"_lng;

	if (mod.virtual_mod)
	{
		description = "This mod is virtual, there is no corresponding directory on disk"_lng;
	}
	else if (auto desc = _managedPlatform.modDataProvider()->description(mod.id); !desc.empty())
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

void ModListView::updateCategoryFilterContent()
{
	wxArrayInt            selections;
	std::set<std::string> selected;
	_filterPopup->GetCheckedItems(selections);
	for (const auto& i : selections)
		if (!_hiddenCategories.contains(_categories[i]))
			selected.emplace(_categories[i]);

	std::set<std::string> cats;
	for (const auto& item : _modManager.mods().data)
		cats.emplace(_managedPlatform.modDataProvider()->modData(item.id).category);

	for (const auto& item : _modManager.mods().rest)
		cats.emplace(_managedPlatform.modDataProvider()->modData(item).category);

	std::vector<std::pair<std::string, wxString>> items;

	for (const auto& item : cats)
	{
		if (!item.empty())
			items.emplace_back(item, wxString::FromUTF8(wxGetApp().categoryTranslationString(item)));
		else
			items.emplace_back(item, "Without category"_lng);
	}

	std::sort(items.begin(), items.end(), [](const auto& l, const auto& r) { return l.second < r.second; });

	_categories.clear();
	wxArrayString displayedItems;

	for (const auto& item : items)
	{
		_categories.emplace_back(item.first);
		displayedItems.Add(item.second);
	}

	_filterPopup->Clear();
	_filterPopup->InsertItems(displayedItems, 0);

	for (size_t i = 0; i < _categories.size(); ++i)
		if (!_hiddenCategories.contains(_categories[i]))
			_filterPopup->Check(i);

	_filterCategory->SetText(_filterPopup->GetStringValue());

	Layout();
}

void ModListView::expandChildren()
{
	wxDataViewItemArray children;
	_listModel->GetChildren(wxDataViewItem(), children);

	for (const auto& item : children)
	{
		auto cat = _listModel->itemGroupByItem(item);

		if (!cat.has_value() || !_collapsedCategories.contains(*cat))
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
	else if (itemId == _menu.archive->GetId())
		onResetSelectedModStateRequested();
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

	auto next = _modManager.mods().next(_selectedMod);

	std::swap(next, _selectedMod);

	_modManager.archive(next);

	EX_UNEXPECTED;
}

void ModListView::onSortModsRequested(const std::string& enablingMod, const std::string& disablingMod)
{
	wxBusyCursor bc;

	EX_TRY;

	auto enabled = ResolveModConflicts(
		_modManager.mods(), *_managedPlatform.modDataProvider(), enablingMod, disablingMod);

	if (enabled != _modManager.mods().enabled())
		_managedPlatform.apply(enabled);

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

	auto& mod = _managedPlatform.modDataProvider()->modData(_selectedMod);

	wxLaunchDefaultApplication(wxString::FromUTF8((mod.data_path / "Screens").string()));

	EX_UNEXPECTED;
}

void ModListView::updateGalleryState(bool show)
{
	EX_TRY;

	_galleryShown = show;

	_showGallery->SetBitmap(_iconStorage.get(show ? embedded_icon::double_down : embedded_icon::double_up));
	_galleryView->Show(show);

	Layout();

	EX_UNEXPECTED;
}
