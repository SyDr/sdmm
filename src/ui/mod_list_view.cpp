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
#include "interface/ii18n_service.hpp"
#include "manage_preset_list_view.hpp"
#include "mod_list_model.h"
#include "mod_manager_app.h"
#include "select_exe.h"
#include "type/icon.hpp"
#include "type/interface_label.hpp"
#include "type/interface_size.hpp"
#include "type/mod_description_used_control.hpp"
#include "utility/fs_util.h"
#include "utility/sdlexcept.h"
#include "utility/shell_util.h"
#include "wx/priority_data_renderer.h"
#include "edit_mod_dialog.hpp"

#include <cmark.h>
#include <wx/app.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/collpane.h>
#include <wx/combo.h>
#include <wx/dataview.h>
#include <wx/html/htmlwin.h>
#include <wx/infobar.h>
#include <wx/msgdlg.h>
#include <wx/notifmsg.h>
#include <wx/sizer.h>
#include <wx/srchctrl.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/webview.h>
#include <wx/richmsgdlg.h>

#include <boost/range/algorithm.hpp>

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

		return "dialog/label/categories"_lng + wxString::FromUTF8(std::format(": {}/{}", selections.size(), GetCount()));
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

	createControls(wxString::FromUTF8(_managedPlatform.managedPath().string()));
	_listModel->modList(_modManager.mods());
	_listModel->applyCategoryFilter(_hiddenCategories);
	expandChildren();
	buildLayout();
	bindEvents();
	updateCategoryFilterContent();
	updateControlsState();
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
	buttonSizer->Add(_configure, wxSizerFlags(0).Border(wxALL, 4).Expand());
	buttonSizer->Add(_moveUp, wxSizerFlags(0).Border(wxALL, 4).Expand());
	buttonSizer->Add(_moveDown, wxSizerFlags(0).Border(wxALL, 4).Expand());
	buttonSizer->Add(_changeState, wxSizerFlags(0).Border(wxALL, 4).Expand());
	buttonSizer->AddStretchSpacer(1);
	buttonSizer->Add(_sort, wxSizerFlags(0).Border(wxALL, 4).Expand());

	auto leftGroupSizer = new wxStaticBoxSizer(_group, wxHORIZONTAL);
	leftGroupSizer->Add(buttonSizer, wxSizerFlags(0).Expand());
	leftGroupSizer->Add(listGroupSizer, wxSizerFlags(1).Expand());

	auto rightBottomSizer = new wxBoxSizer(wxHORIZONTAL);
	rightBottomSizer->AddStretchSpacer();
	rightBottomSizer->Add(_showGallery, wxSizerFlags(0).Border(wxALL, 4));
	rightBottomSizer->Add(_openGallery, wxSizerFlags(0).Border(wxALL, 4));

	auto descriptionSizer = new wxStaticBoxSizer(_modDescriptionGroup, wxHORIZONTAL);

	if (_modDescriptionWebView)
		descriptionSizer->Add(_modDescriptionWebView, wxSizerFlags(1).Expand());
	else if (_modDescriptionHtmlWindow)
		descriptionSizer->Add(_modDescriptionHtmlWindow, wxSizerFlags(1).Expand());
	else if (_modDescriptionTextCtrl)
		descriptionSizer->Add(_modDescriptionTextCtrl, wxSizerFlags(1).Expand());

	auto gallerySizer = new wxStaticBoxSizer(_gallery, wxVERTICAL);
	gallerySizer->Add(_galleryView, wxSizerFlags(1).Expand());

	auto rightSizer = new wxBoxSizer(wxVERTICAL);
	rightSizer->Add(descriptionSizer, wxSizerFlags(1).Expand());
	rightSizer->Add(rightBottomSizer, wxSizerFlags(0).Expand());
	rightSizer->Add(gallerySizer, wxSizerFlags(0).Expand());

	auto contentSizer = new wxBoxSizer(wxHORIZONTAL);
	contentSizer->Add(leftGroupSizer, wxSizerFlags(168).Expand());
	contentSizer->Add(rightSizer, wxSizerFlags(100).Expand().Border(wxLEFT, 4));

	auto vertSizer = new wxBoxSizer(wxVERTICAL);
	vertSizer->Add(contentSizer, wxSizerFlags(1).Expand());
	vertSizer->Add(_infoBar, wxSizerFlags(0).Expand());

	this->SetSizer(vertSizer);
}

void ModListView::bindEvents()
{
	_filterText->Bind(wxEVT_TEXT, [&](wxCommandEvent& event) {
		const auto str = event.GetString();
		_listModel->applyFilter(str.ToStdString(wxConvUTF8));
		expandChildren();
		if (!followSelection())
			updateControlsState();

		_statusBar->SetStatusText(_listModel->status());
		_filterText->ShowCancelButton(!str.IsEmpty());

		event.Skip();
	});

	_filterPopup->Bind(wxEVT_CHECKLISTBOX, [&](wxCommandEvent&) {
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
		if (!followSelection())
			updateControlsState();

		_statusBar->SetStatusText(_listModel->status());
	});

	_list->Bind(wxEVT_DATAVIEW_COLUMN_SORTED, [&](wxDataViewEvent&) { followSelection(); });

	_list->Bind(wxEVT_DATAVIEW_ITEM_COLLAPSING, [&](wxDataViewEvent& event) {
		if (auto item = _listModel->itemGroupByItem(event.GetItem()); item.has_value())
		{
			_collapsedCategories.emplace(*item);
			_managedPlatform.localConfig()->collapsedCategories(_collapsedCategories);
		}
		else
			event.Veto();
	});

	_list->Bind(wxEVT_DATAVIEW_ITEM_EXPANDING, [&](const wxDataViewEvent& event) {
		if (auto item = _listModel->itemGroupByItem(event.GetItem()); item.has_value())
		{
			_collapsedCategories.erase(*item);
			_managedPlatform.localConfig()->collapsedCategories(_collapsedCategories);
		}
	});

	_list->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, [&](wxDataViewEvent&) {
		const auto item = _list->GetSelection();
		const auto mod  = _listModel->findMod(item);
		_selectedMod    = mod ? mod->id : "";
		updateControlsState();
	});

	_list->Bind(
		wxEVT_DATAVIEW_ITEM_ACTIVATED, [&](wxDataViewEvent&) { onSwitchSelectedModStateRequested(); });

	_list->GetMainWindow()->Bind(wxEVT_MOTION, [&](wxMouseEvent& event) {
		wxDataViewItem    item;
		wxDataViewColumn* column = nullptr;

		wxPoint pos = _list->ScreenToClient(_list->GetMainWindow()->ClientToScreen(event.GetPosition()));
		_list->HitTest(pos, item, column);

		if (item.IsOk() && column &&
			static_cast<ModListModelColumn>(column->GetModelColumn()) == ModListModelColumn::support)
		{
			auto id = _listModel->findIdByItem(item);
			if (!id.empty())
			{
				const auto& mod = _managedPlatform.modDataProvider()->modData(id);

				if (!mod.support.empty())
				{
					SetCursor(wxCursor(wxCURSOR_HAND));

					event.Skip();
					return;
				}
			}
		}

		SetCursor(wxCursor());

		event.Skip();
	});

	_list->GetMainWindow()->Bind(wxEVT_LEFT_UP, [&](wxMouseEvent& event) {
		wxDataViewItem    item;
		wxDataViewColumn* column = nullptr;

		wxPoint pos = _list->ScreenToClient(_list->GetMainWindow()->ClientToScreen(event.GetPosition()));
		_list->HitTest(pos, item, column);

		if (item.IsOk() && column &&
			static_cast<ModListModelColumn>(column->GetModelColumn()) == ModListModelColumn::support)
		{
			auto id = _listModel->findIdByItem(item);
			if (!id.empty())
			{
				const auto& mod = _managedPlatform.modDataProvider()->modData(id);

				if (!mod.support.empty())
				{
					wxMenu menu;

					for (const auto& i : mod.support)
						menu.Append(wxID_ANY, wxString::FromUTF8(i));

					_list->PopupMenu(&menu, pos);

					event.Skip();
					return;
				}
			}
		}

		event.Skip();
	});

	_list->Bind(wxEVT_DATAVIEW_ITEM_BEGIN_DRAG, [&](wxDataViewEvent& event) {
		auto moveFrom = _listModel->findIdByItem(event.GetItem());
		if (moveFrom.empty())
		{
			event.Veto();
			return;
		}

		event.SetDataObject(new wxTextDataObject(wxString::FromUTF8(moveFrom)));
		event.SetDragFlags(wxDrag_DefaultMove);
	});

	_list->Bind(wxEVT_DATAVIEW_ITEM_DROP_POSSIBLE, [&](wxDataViewEvent& event) {
		if (!event.GetItem().IsOk())
		{
			event.Veto();
			return;
		}

		auto moveTarget = _listModel->itemGroupByItem(event.GetItem());
		if (moveTarget.has_value() &&
			!std::holds_alternative<ModListDsplayedData::ManagedGroupTag>(*moveTarget))
		{
			// i.e. hover over archived group
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

	_list->Bind(wxEVT_DATAVIEW_ITEM_DROP, [&](const wxDataViewEvent& event) {
		wxTextDataObject from;
		from.SetData(wxDF_UNICODETEXT, event.GetDataSize(), event.GetDataBuffer());

		const auto moveFrom = from.GetText().utf8_string();

		auto moveTarget = _listModel->itemGroupByItem(event.GetItem());
		if (moveTarget.has_value() &&
			!std::holds_alternative<ModListDsplayedData::ManagedGroupTag>(*moveTarget))
		{
			// i.e. hover over archived group
			_modManager.archive(moveFrom);
			return;
		}

		_modManager.move(moveFrom, _listModel->findIdByItem(event.GetItem()));
	});

	_list->Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, [&](wxDataViewEvent& event) {
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

	_configure->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) {
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
			if (!followSelection())
				updateControlsState();
		}
	});

	_moveUp->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { _modManager.moveUp(_selectedMod); });
	_moveDown->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { _modManager.moveDown(_selectedMod); });
	_changeState->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { onSwitchSelectedModStateRequested(); });

	_sort->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { onSortModsRequested({}, {}); });

	_openGallery->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { openGalleryRequested(); });

	_showGallery->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) {
		_managedPlatform.localConfig()->screenshotsExpanded(!_galleryShown);
		updateGalleryState(!_galleryShown);
	});

	Bind(wxEVT_TIMER, [&](wxTimerEvent&) { _infoBar->Dismiss(); });

	Bind(wxEVT_CHAR_HOOK, [&](wxKeyEvent& event) {
		if (!event.ControlDown() || event.GetKeyCode() != 'F')
		{
			event.Skip();
			return;
		}

		_filterText->SetFocusFromKbd();
	});

	if (_modDescriptionWebView)
	{
		// WebView2 sends wxEVT_WEBVIEW_NAVIGATING events even for SetPage calls
		_modDescriptionWebView->Bind(wxEVT_WEBVIEW_NAVIGATED, [&](wxWebViewEvent&) {
			_modDescriptionWebView->Bind(wxEVT_WEBVIEW_NAVIGATING, &ModListView::OnWebViewNavigating, this);
		});
	}
	else if (_modDescriptionHtmlWindow)
	{
		_modDescriptionHtmlWindow->Bind(wxEVT_HTML_LINK_CLICKED,
			[&](const wxHtmlLinkEvent& event) { wxLaunchDefaultBrowser(event.GetLinkInfo().GetHref()); });
	}
	else if (_modDescriptionTextCtrl)
	{
		_modDescriptionTextCtrl->Bind(wxEVT_TEXT_URL, [=](wxTextUrlEvent& event) {
			if (!event.GetMouseEvent().ButtonDClick(wxMOUSE_BTN_LEFT))
			{
				event.Skip();
				return;
			}

			const auto url = _modDescriptionTextCtrl->GetRange(event.GetURLStart(), event.GetURLEnd());
			wxLaunchDefaultBrowser(url);
		});
	}
}

void ModListView::createControls(const wxString& managedPath)
{
	_group = new wxStaticBox(this, wxID_ANY, managedPath);

	_filterText = new wxSearchCtrl(this, wxID_ANY);
	_filterText->SetDescriptiveText("dialog/label/filter"_lng);

	_filterCategory = new wxComboCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
		{ GetTextExtent("dialog/label/categories"_lng).x + GetTextExtent(wxString::FromUTF8(": 99/99")).x + FromDIP(32),
			-1 },
		wxCB_READONLY);

	_filterPopup = new mmCheckListBoxComboPopup();
	_filterCategory->SetPopupControl(_filterPopup);

	createListControl();

	_modDescriptionGroup = new wxStaticBox(this, wxID_ANY, L"");

	// TODO: create separate method (and use variant for control itself?)
	const auto descriptionControl = wxGetApp().appConfig().modDescriptionUsedControl();
	if (descriptionControl == ModDescriptionUsedControl::try_to_use_webview2 &&
		wxWebView::IsBackendAvailable(wxString::FromUTF8(wxWebViewBackendEdge)))
	{
		_modDescriptionWebView = wxWebView::New();
		_modDescriptionWebView->Create(_modDescriptionGroup, wxID_ANY);
		_modDescriptionWebView->EnableContextMenu(false);
		_modDescriptionWebView->EnableHistory(false);
		_modDescriptionWebView->EnableAccessToDevTools(false);
		_modDescriptionWebView->SetPage(L"<html></html>", L"");
	}
	else if (descriptionControl != ModDescriptionUsedControl::use_plain_text_control)
	{
		_modDescriptionHtmlWindow = new wxHtmlWindow(_modDescriptionGroup);
		_modDescriptionHtmlWindow->SetHTMLBackgroundColour(_modDescriptionGroup->GetBackgroundColour());
	}
	else
	{
		_modDescriptionTextCtrl = new wxTextCtrl(_modDescriptionGroup, wxID_ANY, wxEmptyString,
			wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_NOHIDESEL);
		_modDescriptionTextCtrl->SetBackgroundColour(_modDescriptionGroup->GetBackgroundColour());
	}

	const bool useLabels = wxGetApp().appConfig().interfaceLabel() != InterfaceLabel::dont_show;

	if (useLabels)
	{
		_configure   = new wxButton(_group, wxID_ANY, "dialog/button/configure"_lng);
		_moveUp      = new wxButton(_group, wxID_ANY, "dialog/button/move_up"_lng);
		_moveDown    = new wxButton(_group, wxID_ANY, "dialog/button/move_down"_lng);
		_changeState = new wxButton(_group, wxID_ANY, "dialog/button/enable"_lng);
		_sort        = new wxButton(_group, wxID_ANY, "dialog/button/sort"_lng);

		_configure->SetBitmap(_iconStorage.get(Icon::Stock::cog, Icon::Size::x16));
		_moveUp->SetBitmap(_iconStorage.get(Icon::Stock::up, Icon::Size::x16));
		_moveDown->SetBitmap(_iconStorage.get(Icon::Stock::down, Icon::Size::x16));
		_changeState->SetBitmap(_iconStorage.get(Icon::Stock::checkmark_green, Icon::Size::x16));
		_sort->SetBitmap(_iconStorage.get(Icon::Stock::sort, Icon::Size::x16));
	}
	else
	{
		_configure = new wxBitmapButton(_group, wxID_ANY, _iconStorage.get(Icon::Stock::cog, Icon::Size::x16),
			wxDefaultPosition, { FromDIP(24), FromDIP(24) }, wxBU_EXACTFIT);
		_moveUp    = new wxBitmapButton(_group, wxID_ANY, _iconStorage.get(Icon::Stock::up, Icon::Size::x16),
			   wxDefaultPosition, { FromDIP(24), FromDIP(24) }, wxBU_EXACTFIT);
		_moveDown = new wxBitmapButton(_group, wxID_ANY, _iconStorage.get(Icon::Stock::down, Icon::Size::x16),
			wxDefaultPosition, { FromDIP(24), FromDIP(24) }, wxBU_EXACTFIT);
		_changeState = new wxBitmapButton(_group, wxID_ANY,
			_iconStorage.get(Icon::Stock::checkmark_green, Icon::Size::x16), wxDefaultPosition,
			{ FromDIP(24), FromDIP(24) }, wxBU_EXACTFIT);
		_sort = new wxBitmapButton(_group, wxID_ANY, _iconStorage.get(Icon::Stock::sort, Icon::Size::x16),
			wxDefaultPosition, { FromDIP(24), FromDIP(24) }, wxBU_EXACTFIT);
	}

	_configure->SetToolTip("dialog/settings/configure_main_view/caption"_lng); // TODO: own lng entry?
	_moveUp->SetToolTip("dialog/button/move_up"_lng);
	_moveDown->SetToolTip("dialog/button/move_down"_lng);
	_changeState->SetToolTip("dialog/button/enable"_lng);
	_sort->SetToolTip("dialog/button/sort"_lng);

	_moveUp->Disable();
	_moveDown->Disable();
	_changeState->Disable();

	_menu.openHomepage = _menu.menu.Append(wxID_ANY, "dialog/button/open_homepage"_lng);
	_menu.openDir      = _menu.menu.Append(wxID_ANY, "dialog/button/open_directory"_lng);
	_menu.archive      = _menu.menu.Append(wxID_ANY, "dialog/button/archive"_lng);
	_menu.edit         = _menu.menu.Append(wxID_ANY, "dialog/button/edit"_lng);
	_menu.menu.AppendSeparator();
	_menu.deleteOrRemove = _menu.menu.Append(wxID_ANY, L"placeholder");

	_galleryShown = _managedPlatform.localConfig()->screenshotsExpanded();
	_showGallery  = new wxButton(this, wxID_ANY, "dialog/button/screenshots"_lng);
	_showGallery->SetBitmap(
		_iconStorage.get(_galleryShown ? Icon::Stock::double_down : Icon::Stock::double_up, Icon::Size::x16));

	wxSize goodSize = _showGallery->GetBestSize();
	goodSize.SetWidth(goodSize.GetHeight());

	_openGallery = new wxButton(this, wxID_ANY, wxEmptyString, wxDefaultPosition, goodSize, wxBU_EXACTFIT);
	_openGallery->Disable();
	_openGallery->SetBitmap(_iconStorage.get(Icon::Stock::folder, Icon::Size::x16));

	_gallery = new wxStaticBox(this, wxID_ANY, L"");

	_galleryView = new ImageGalleryView(_gallery, wxID_ANY);
	_galleryView->Show(_galleryShown);
	_gallery->Show(_galleryShown);

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
	if (auto interfaceSize = wxGetApp().appConfig().interfaceSize(); interfaceSize != InterfaceSize::standard)
		_list->SetRowHeight(FromDIP(toBaseSize(interfaceSize)));

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
		case ModListModelColumn::name: r = new wxDataViewIconTextRenderer(); break;
		case ModListModelColumn::support:
			r = new wxDataViewBitmapRenderer(
				wxDataViewBitmapRenderer::GetDefaultType(), wxDATAVIEW_CELL_ACTIVATABLE);
			break;
		case ModListModelColumn::priority:
		{
			int size = FromDIP(32);
			if (auto interfaceSize = wxGetApp().appConfig().interfaceSize();
				interfaceSize != InterfaceSize::standard)
				size = FromDIP(16 + toBaseSize(wxGetApp().appConfig().interfaceSize()));
			r = new mmPriorityDataRenderer(size);
			break;
		}
		default: r = new wxDataViewTextRenderer(); break;
		}

		r->SetAlignment(wxALIGN_CENTER_VERTICAL);
		if (i == columns.size() - 1)
			r->EnableEllipsize(wxELLIPSIZE_END);

		int flags = wxDATAVIEW_COL_RESIZABLE;  // | wxDATAVIEW_COL_REORDERABLE; // TODO: give it back and
											   // allow sorting on main window instead

		if (typed != ModListModelColumn::version && typed != ModListModelColumn::support)
			flags |= wxDATAVIEW_COL_SORTABLE;

		int width = FromDIP(120);
		if (typed != ModListModelColumn::author)
			width = wxCOL_WIDTH_AUTOSIZE;

		wxString name =
			wxString::FromUTF8(wxGetApp().i18nService().column(std::string(magic_enum::enum_name((typed)))));
		wxAlignment alignment = wxALIGN_LEFT;

		if (typed == ModListModelColumn::support)
		{
			flags &= ~wxDATAVIEW_COL_RESIZABLE;
			alignment = wxALIGN_CENTER;
			name.clear();
			width = FromDIP(24);
			if (auto interfaceSize = wxGetApp().appConfig().interfaceSize();
				interfaceSize == InterfaceSize::big)
				width *= 2;
		}

		auto c = new wxDataViewColumn(name, r, column, width, alignment, flags);

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

	auto setDescription = [&](wxString content) {
		if (_modDescriptionWebView)
		{
			content.Replace(L"$", L"${sign}");
			content.Replace(L"`", L"${tick}");

			_modDescriptionWebView->RunScript(
				wxString::Format(L"const tick = '`'; const sign = '$'; document.open(); "
								 L"document.write(String.raw`%s`);"
								 L"document.close(); "
								 L"window.scrollTo(0, 0); ",
					content));
		}
		else if (_modDescriptionHtmlWindow)
		{
			_modDescriptionHtmlWindow->SetPage(content);
			_modDescriptionHtmlWindow->Scroll(0, 0);
			_modDescriptionHtmlWindow->SetHTMLBackgroundColour(_modDescriptionGroup->GetBackgroundColour());
		}
		else if (_modDescriptionTextCtrl)
		{
			_modDescriptionTextCtrl->SetValue(content);
			_modDescriptionTextCtrl->SetBackgroundColour(_modDescriptionGroup->GetBackgroundColour());
		}
	};

	if (_selectedMod.empty())
	{
		_selectedModCached.clear();
		_moveUp->Disable();
		_moveDown->Disable();
		_changeState->Disable();
		setDescription(L"");
		_openGallery->Disable();
		_galleryView->Reset();

		return;
	}

	const auto& mod = _managedPlatform.modDataProvider()->modData(_selectedMod);

	_changeState->Enable();

	if (wxGetApp().appConfig().interfaceLabel() == InterfaceLabel::dont_show)
	{
		_changeState->SetBitmap(wxNullBitmap);
		_changeState->SetBitmap(_iconStorage.get(
			_modManager.mods().enabled(mod.id) ? Icon::Stock::cross_gray : Icon::Stock::checkmark_green,
			Icon::Size::x16));
	}
	else
	{
		_changeState->SetBitmap(wxNullBitmap);
		_changeState->SetBitmap(_iconStorage.get(
			_modManager.mods().enabled(mod.id) ? Icon::Stock::cross_gray : Icon::Stock::checkmark_green,
			Icon::Size::x16));

		_changeState->SetLabelText(_modManager.mods().enabled(mod.id) ? "dialog/button/disable"_lng : "dialog/button/enable"_lng);
	}

	_changeState->SetToolTip(_modManager.mods().enabled(mod.id) ? "dialog/button/disable"_lng : "dialog/button/enable"_lng);

	_moveUp->Enable(_modManager.mods().canMoveUp(mod.id));
	_moveDown->Enable(_modManager.mods().canMoveDown(mod.id));

	if (_selectedMod != _selectedModCached)
	{
		auto description = "message/status/no_description_available"_lng;

		if (mod.virtual_mod)
		{
			description = "message/info/virtual_mod"_lng;
		}
		else if (auto desc = _managedPlatform.modDataProvider()->description(mod.id); !desc.empty())
		{
			if (!_modDescriptionTextCtrl)
			{
				auto cnvt = std::unique_ptr<char, decltype(&std::free)>(
					cmark_markdown_to_html(desc.c_str(), desc.size(), CMARK_OPT_DEFAULT), &std::free);

				desc = cnvt.get();

				auto asString = wxString::FromUTF8(desc);

				if (asString.empty())
					asString = wxString(desc.c_str(), wxConvLocal, desc.size());

				if (!asString.empty())
					std::swap(asString, description);
			}
			else
			{
				description = wxString::FromUTF8(desc);
			}
		}

		setDescription(description);

		_openGallery->Enable(fs::exists(mod.data_path / "Screens"));
		_galleryView->SetPath(mod.data_path / "Screens");

		_selectedModCached = _selectedMod;
	}

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
			items.emplace_back(item, "column/without_category"_lng);
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
	if (!displayedItems.empty())
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

bool ModListView::followSelection()
{
	// wxLogDebug(__FUNCTION__);
	const auto itemToSelect = _listModel->findItemById(_selectedMod);

	if (itemToSelect.IsOk())
	{
		_list->EnsureVisible(itemToSelect);
		_list->Select(itemToSelect);

		return true;
	}

	if (_selectedMod.empty())
		return true;

	_selectedMod.clear();

	return false;
}

void ModListView::OnListItemContextMenu(const wxDataViewItem& item)
{
	if (const auto mod = _listModel->findMod(item))
	{
		_menu.openHomepage->Enable(!mod->homepage.empty());
		_menu.openDir->Enable(!mod->virtual_mod);
		_menu.deleteOrRemove->SetItemLabel(mod->virtual_mod ? "dialog/button/remove_from_list"_lng : "dialog/button/delete"_lng);
		_list->PopupMenu(&_menu.menu);
	}
}

void ModListView::OnMenuItemSelected(const wxCommandEvent& event)
{
	if (event.GetEventObject() == &_menu.menu)
	{
		const auto itemId = event.GetId();

		const auto mod = _listModel->findMod(_list->GetSelection());

		if (itemId == _menu.openHomepage->GetId())
			wxLaunchDefaultBrowser(wxString::FromUTF8(mod->homepage));
		else if (itemId == _menu.openDir->GetId())
			wxLaunchDefaultApplication(wxString::FromUTF8(mod->data_path.string()));
		else if (itemId == _menu.archive->GetId())
			onResetSelectedModStateRequested();
		else if (itemId == _menu.edit->GetId())
			onEditModRequested();
		else if (itemId == _menu.deleteOrRemove->GetId())
			onRemoveModRequested();

		return;
	}

	// otherwise this is menu for support (TODO: rewrite this mess)

	auto menu = dynamic_cast<wxMenu*>(event.GetEventObject());
	if (!menu)
		return;

	auto items = menu->GetMenuItems();
	for (const auto& item : items)
	{
		if (item->GetId() == event.GetId())
		{
			wxLaunchDefaultBrowser(item->GetItemLabel());
			break;
		}
	}
}

void ModListView::OnWebViewNavigating(wxWebViewEvent& event)
{
	wxLaunchDefaultBrowser(event.GetURL());
	event.Veto();
}

void ModListView::onSwitchSelectedModStateRequested()
{
	EX_TRY;

	if (_selectedMod.empty())
		return;

	const bool warnAboutConflicts = _managedPlatform.localConfig()->warnAboutConflictsBeforeEnabling();
	const bool autoSort =
		_managedPlatform.localConfig()->conflictResolveMode() == ConflictResolveMode::automatic;

	const auto& enabling  = _modManager.mods().enabled(_selectedMod) ? std::string() : _selectedMod;
	const auto& disabling = _modManager.mods().enabled(_selectedMod) ? _selectedMod : std::string();

	if (warnAboutConflicts && !enabling.empty())
	{
		if (!autoSort)
		{
			if (!warnBeforeEnabling(enabling)) // message: x incompatible with y, z
				return;
		}
		else
		{
			if (!warnBeforeEnablingAndSort(enabling)) // message: enabling would disable y, z, a
				return;
		}
	}

	_modManager.switchState(_selectedMod);

	if (!autoSort)
		return;

	onSortModsRequested(enabling, disabling);

	static bool messageWasShown = false;
	if (!messageWasShown)
	{
		_infoBar->ShowMessage(
			wxString::Format("message/notification/automatic_resolve_mode_enabled"_lng));
		_infoBarTimer.StartOnce(10000);
		messageWasShown = true;
	}

	EX_UNEXPECTED;
}

bool ModListView::warnBeforeEnabling(const std::string& enablingMod)
{
	const auto& incompatible = _managedPlatform.modDataProvider()->modData(enablingMod).incompatible;

	std::vector<std::string> activeIncompatible;
	for (const auto& item : _modManager.mods().enabled())
		if (incompatible.contains(item))
			activeIncompatible.emplace_back(item);

	if (!activeIncompatible.empty())
	{
		for (auto& item : activeIncompatible)
			item = _managedPlatform.modDataProvider()->modData(item).name;

		const auto message = wxString::Format("message/question/warn_about_conflicts_before_enabling"_lng,
			wxString::FromUTF8(enablingMod), wxString::FromUTF8(boost::join(activeIncompatible, ", ")));

		return warnBeforeEnableImpl(message, L"");
	}

	return true;
}

bool ModListView::warnBeforeEnablingAndSort(const std::string& enablingMod)
{
	auto wouldBe = _modManager.mods();
	wouldBe.switchState(_selectedMod);

	auto newEnabled = ResolveModConflicts(wouldBe, *_managedPlatform.modDataProvider(), enablingMod, {});

	auto enabledSorted = newEnabled;
	auto currentSorted = _modManager.mods().enabled();

	boost::sort(enabledSorted);
	boost::sort(currentSorted);

	std::vector<std::string> wouldBeDisabled;
	boost::set_difference(currentSorted, enabledSorted, std::back_inserter(wouldBeDisabled));

	if (!wouldBeDisabled.empty())
	{
		for (auto& item : wouldBeDisabled)
			item = _managedPlatform.modDataProvider()->modData(item).name;

		const auto message =
			wxString::Format("message/question/warn_about_conflicts_before_enabling_sort"_lng,
				wxString::FromUTF8(boost::join(wouldBeDisabled, ", ")));

		return warnBeforeEnableImpl(message, L"");
	}

	return true;
}

bool ModListView::warnBeforeEnableImpl(const wxString& message, const wxString& detailed)
{
	wxRichMessageDialog rmd(this, message, wxTheApp->GetAppName(), wxYES_NO | wxNO_DEFAULT);
	rmd.ShowCheckBox("message/info/dont_show_again"_lng);
	if (!detailed.empty())
		rmd.ShowDetailedText(detailed);

	const int answer = rmd.ShowModal();

	if (rmd.IsCheckBoxChecked())
		_managedPlatform.localConfig()->warnAboutConflictsBeforeEnabling(false);

	return answer == wxID_YES;
}

void ModListView::onResetSelectedModStateRequested()
{
	EX_TRY;

	if (_selectedMod.empty())
		return;

	auto next = _modManager.mods().next(_selectedMod);

	std::swap(next, _selectedMod);

	_modManager.archive(next);

	updateControlsState();

	EX_UNEXPECTED;
}

void ModListView::onEditModRequested()
{
	EX_TRY;

	if (_selectedMod.empty())
		return;

	EditModDialog cnmd(this, _managedPlatform, _selectedMod);
	cnmd.ShowModal();

	EX_UNEXPECTED;
}

void ModListView::onSortModsRequested(
	const std::string& enablingMod, const std::string& disablingMod)
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

	auto& mod = _managedPlatform.modDataProvider()->modData(_selectedMod);

	if (!mod.virtual_mod)
	{
		const auto formatMessage = "message/question/delete_mod"_lng;
		const auto answer        = wxMessageBox(wxString::Format(formatMessage, wxString::FromUTF8(mod.name)),
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

	_showGallery->SetBitmap(
		_iconStorage.get(show ? Icon::Stock::double_down : Icon::Stock::double_up, Icon::Size::x16));
	_galleryView->Show(show);
	_gallery->Show(show);

	Layout();

	EX_UNEXPECTED;
}
