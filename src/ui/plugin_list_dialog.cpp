// SD Mod Manager

// Copyright (c) 2026 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "plugin_list_dialog.hpp"

#include "application.h"
#include "domain/mod_data.hpp"
#include "interface/iapp_config.hpp"
#include "interface/iicon_storage.hpp"
#include "interface/iplugin_manager.hpp"
#include "mod_manager_app.h"
#include "type/icon.hpp"
#include "type/interface_label.hpp"
#include "utility/string_util.hpp"

#include <boost/locale.hpp>

#include <wx/bitmap.h>
#include <wx/bmpbuttn.h>
#include <wx/button.h>
#include <wx/imaglist.h>
#include <wx/log.h>
#include <wx/msgdlg.h>
#include <wx/srchctrl.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/textctrl.h>
#include <wx/treectrl.h>
#include <wx/wupdlock.h>

#include <array>
#include <unordered_map>
#include <utility>
using namespace mm;

namespace
{
	class PluginTreeItemData : public wxTreeItemData
	{
	public:
		explicit PluginTreeItemData(std::size_t index)
			: index(index)
		{}

		std::size_t index;
	};

	class PluginTreeGroupData : public wxTreeItemData
	{
	public:
		explicit PluginTreeGroupData(PluginLocation location)
			: location(location)
		{}

		PluginLocation location;
	};

	enum TreeIcon
	{
		folderIcon,
		enabledIcon,
		disabledIcon,
		collisionIcon,
	};

	int pluginImage(const ModPlugin& plugin)
	{
		return plugin.collision ? collisionIcon : plugin.enabled ? enabledIcon : disabledIcon;
	}

	wxString pluginCaption(const ModPlugin& plugin, bool treeHasImages)
	{
		auto caption = wxString::FromUTF8(plugin.caption);
		if (!treeHasImages)
			caption.Prepend(plugin.collision ? L"[!] " : plugin.enabled ? L"[+] " : L"[-] ");

		return caption;
	}

	wxString locationLabel(PluginLocation location)
	{
		switch (location)
		{
		case PluginLocation::era_plugins: return L"EraPlugins";
		case PluginLocation::before_wog: return L"BeforeWoG";
		case PluginLocation::after_wog: return L"AfterWoG";
		}

		return {};
	}
}

PluginListView::PluginListView(wxWindow* parent, IPluginManager& manager, IIconStorage& iconStorage,
	int actionColumnWidth, std::array<int, 2> precedingActionSlotHeights, std::function<void()> onBack)
	: wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL)
	, _manager(manager)
	, _iconStorage(iconStorage)
	, _actionColumnWidth(actionColumnWidth)
	, _precedingActionSlotHeights(precedingActionSlotHeights)
	, _onBack(std::move(onBack))
{
	createControls();
	buildLayout();
	bindEvents();
}

PluginListView::~PluginListView()
{
	_closing = true;

	Unbind(wxEVT_CHAR_HOOK, &PluginListView::onCharHook, this);
	_filterText->Unbind(wxEVT_TEXT, &PluginListView::onFilterChanged, this);
	_tree->Unbind(wxEVT_TREE_SEL_CHANGED, &PluginListView::onTreeSelectionChanged, this);
	_tree->Unbind(wxEVT_TREE_ITEM_ACTIVATED, &PluginListView::onTreeItemActivated, this);
	_changeState->Unbind(wxEVT_BUTTON, &PluginListView::onChangeState, this);
	_back->Unbind(wxEVT_BUTTON, &PluginListView::onBack, this);
}

void PluginListView::createControls()
{
	_pluginsGroup  = new wxStaticBox(this, wxID_ANY, wxEmptyString);
	_filterText    = new wxSearchCtrl(_pluginsGroup, wxID_ANY);
	_filterText->SetDescriptiveText("dialog/label/filter"_lng);
	_tree          = new wxTreeCtrl(_pluginsGroup, wxID_ANY, wxDefaultPosition, wxDefaultSize,
				 wxTR_HIDE_ROOT | wxTR_HAS_BUTTONS | wxTR_NO_LINES | wxTR_SINGLE | wxTR_FULL_ROW_HIGHLIGHT);
	_tree->SetDoubleBuffered(true);
	_treeHasImages = createTreeImages();

	_descriptionGroup = new wxStaticBox(this, wxID_ANY, "dialog/plugin_manager/description"_lng);
	_description      = new wxTextCtrl(_descriptionGroup, wxID_ANY, wxEmptyString, wxDefaultPosition,
			 wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_NOHIDESEL);
	_description->SetBackgroundColour(_descriptionGroup->GetBackgroundColour());

	const bool useLabels = wxGetApp().appConfig().interfaceLabel() != InterfaceLabel::dont_show;
	if (useLabels)
	{
		_changeState = new wxButton(_pluginsGroup, wxID_ANY, "dialog/button/enable"_lng);
		_back        = new wxButton(_pluginsGroup, wxID_ANY, "dialog/button/back"_lng);
		_changeState->SetBitmap(_iconStorage.get(Icon::Stock::checkmark_green, Icon::Size::x16));
		_back->SetBitmap(_iconStorage.get(Icon::Stock::back, Icon::Size::x16));
	}
	else
	{
		_changeState = new wxBitmapButton(_pluginsGroup, wxID_ANY,
			_iconStorage.get(Icon::Stock::checkmark_green, Icon::Size::x16), wxDefaultPosition,
			{ FromDIP(24), FromDIP(24) }, wxBU_EXACTFIT);
		_back = new wxBitmapButton(_pluginsGroup, wxID_ANY,
			_iconStorage.get(Icon::Stock::back, Icon::Size::x16), wxDefaultPosition,
			{ FromDIP(24), FromDIP(24) }, wxBU_EXACTFIT);
	}

	_changeState->SetToolTip("dialog/button/enable"_lng);
	_back->SetToolTip("dialog/button/back"_lng);
	_changeState->Disable();
}

bool PluginListView::createTreeImages()
{
	const std::array bitmaps = {
		_iconStorage.get(Icon::Stock::folder),
		_iconStorage.get(Icon::Stock::checkmark_green),
		_iconStorage.get(Icon::Stock::cross_red),
		_iconStorage.get(Icon::Stock::question),
	};

	for (const auto& bitmap : bitmaps)
		if (!bitmap.IsOk() || bitmap.GetSize() != bitmaps.front().GetSize())
			return false;

	const auto imageSize = bitmaps.front().GetSize();
	auto       images    = std::make_unique<wxImageList>(
        imageSize.GetWidth(), imageSize.GetHeight(), true, static_cast<int>(bitmaps.size()));

	wxLogNull suppressImageListErrors;
	for (const auto& bitmap : bitmaps)
		if (images->Add(bitmap) == wxNOT_FOUND)
			return false;

	_tree->AssignImageList(images.release());
	return true;
}

void PluginListView::buildLayout()
{
	auto filterSizer = new wxBoxSizer(wxHORIZONTAL);
	filterSizer->Add(_filterText, wxSizerFlags(1).Expand().Border(wxALL, 4));

	auto treeSizer = new wxBoxSizer(wxVERTICAL);
	treeSizer->Add(filterSizer, wxSizerFlags(0).Expand());
	treeSizer->Add(_tree, wxSizerFlags(1).Expand().Border(wxALL, 4));

	auto buttonSizer = new wxBoxSizer(wxVERTICAL);
	buttonSizer->SetMinSize(_actionColumnWidth, -1);
	for (const auto height : _precedingActionSlotHeights)
		buttonSizer->AddSpacer(height);
	buttonSizer->Add(_changeState, wxSizerFlags(0).Border(wxALL, 4).Expand());
	buttonSizer->AddStretchSpacer(1);
	buttonSizer->Add(_back, wxSizerFlags(0).Border(wxALL, 4).Expand());

	auto leftGroupSizer = new wxStaticBoxSizer(_pluginsGroup, wxHORIZONTAL);
	leftGroupSizer->Add(buttonSizer, wxSizerFlags(0).Expand());
	leftGroupSizer->Add(treeSizer, wxSizerFlags(1).Expand());

	auto descriptionSizer = new wxStaticBoxSizer(_descriptionGroup, wxVERTICAL);
	descriptionSizer->Add(_description, wxSizerFlags(1).Expand().Border(wxALL, 4));

	auto contentSizer = new wxBoxSizer(wxHORIZONTAL);
	contentSizer->Add(leftGroupSizer, wxSizerFlags(168).Expand());
	contentSizer->Add(descriptionSizer, wxSizerFlags(100).Expand().Border(wxLEFT, 4));

	auto mainSizer = new wxBoxSizer(wxVERTICAL);
	mainSizer->Add(contentSizer, wxSizerFlags(1).Expand());
	SetSizer(mainSizer);
}

void PluginListView::bindEvents()
{
	Bind(wxEVT_CHAR_HOOK, &PluginListView::onCharHook, this);
	_filterText->Bind(wxEVT_TEXT, &PluginListView::onFilterChanged, this);
	_tree->Bind(wxEVT_TREE_SEL_CHANGED, &PluginListView::onTreeSelectionChanged, this);
	_tree->Bind(wxEVT_TREE_ITEM_ACTIVATED, &PluginListView::onTreeItemActivated, this);
	_changeState->Bind(wxEVT_BUTTON, &PluginListView::onChangeState, this);
	_back->Bind(wxEVT_BUTTON, &PluginListView::onBack, this);
}

void PluginListView::onCharHook(wxKeyEvent& event)
{
	if (_closing)
		return;

	if (event.GetKeyCode() == WXK_ESCAPE)
	{
		_onBack();
		return;
	}

	if (event.ControlDown() && event.GetKeyCode() == 'F')
	{
		_filterText->SetFocusFromKbd();
		return;
	}

	event.Skip();
}

void PluginListView::onFilterChanged(wxCommandEvent& event)
{
	if (_closing)
		return;

	const auto preferredSelection = selectedPluginIndex();
	const auto value              = event.GetString();
	const auto insertionPoint     = _filterText->GetInsertionPoint();
	_filter                       = boost::locale::fold_case(value.ToStdString(wxConvUTF8));
	populateTree(preferredSelection, false);
	_filterText->ShowCancelButton(!value.IsEmpty());
	_filterText->SetFocus();
	_filterText->SetInsertionPoint(insertionPoint);
	event.Skip();
}

void PluginListView::onTreeSelectionChanged(wxTreeEvent& event)
{
	if (!_closing && !_populatingTree)
		updateSelection();
	event.Skip();
}

void PluginListView::onTreeItemActivated(wxTreeEvent& event)
{
	if (!_closing)
		toggleSelected();
	event.Skip();
}

void PluginListView::onChangeState(wxCommandEvent&)
{
	if (!_closing)
		toggleSelected();
}

void PluginListView::onBack(wxCommandEvent&)
{
	if (!_closing)
		_onBack();
}

bool PluginListView::load(const ModData& mod)
{
	auto plugins = _manager.list(mod);
	if (plugins.empty())
		return false;

	_mod     = mod;
	_plugins = std::move(plugins);
	_filter.clear();
	_filterText->ChangeValue(wxEmptyString);
	_filterText->ShowCancelButton(false);
	_pluginsGroup->SetLabel(
		wxString::Format("dialog/plugin_manager/page_caption"_lng, wxString::FromUTF8(mod.name)));
	_tree->DeleteAllItems();
	populateTree();
	Layout();
	return true;
}

void PluginListView::populateTree(std::optional<std::size_t> preferredSelection, bool focusSelection)
{
	wxWindowUpdateLocker updateLocker(_tree);
	_populatingTree = true;

	auto root = _tree->GetRootItem();
	if (!root.IsOk())
		root = _tree->AddRoot(L"Plugins");

	std::array<std::vector<std::size_t>, 3> matchingPlugins;
	for (std::size_t i = 0; i < _plugins.size(); ++i)
		if (pluginPassesFilter(_plugins[i]))
			matchingPlugins[static_cast<std::size_t>(_plugins[i].location)].push_back(i);

	std::array<wxTreeItemId, 3> groups;
	std::vector<wxTreeItemId>   obsoleteGroups;
	wxTreeItemIdValue           groupCookie;
	for (auto group = _tree->GetFirstChild(root, groupCookie); group.IsOk();
		 group = _tree->GetNextChild(root, groupCookie))
	{
		const auto data = dynamic_cast<PluginTreeGroupData*>(_tree->GetItemData(group));
		if (!data)
		{
			obsoleteGroups.push_back(group);
			continue;
		}

		const auto index = static_cast<std::size_t>(data->location);
		if (index >= groups.size() || groups[index].IsOk())
			obsoleteGroups.push_back(group);
		else
			groups[index] = group;
	}
	for (const auto& group : obsoleteGroups)
		_tree->Delete(group);

	for (std::size_t i = 0; i < groups.size(); ++i)
		if (matchingPlugins[i].empty() && groups[i].IsOk())
		{
			_tree->Delete(groups[i]);
			groups[i] = wxTreeItemId();
		}

	wxTreeItemId previousGroup;
	for (const auto location :
		{ PluginLocation::era_plugins, PluginLocation::before_wog, PluginLocation::after_wog })
	{
		const auto index = static_cast<std::size_t>(location);
		if (matchingPlugins[index].empty())
			continue;

		if (!groups[index].IsOk())
		{
			const auto image = _treeHasImages ? folderIcon : -1;
			const auto label = locationLabel(location);
			auto       data  = new PluginTreeGroupData(location);
			groups[index] = previousGroup.IsOk()
				? _tree->InsertItem(root, previousGroup, label, image, image, data)
				: _tree->PrependItem(root, label, image, image, data);
		}

		previousGroup = groups[index];
	}

	wxTreeItemId firstPlugin;
	wxTreeItemId preferredPlugin;
	std::optional<std::size_t> firstPluginIndex;
	for (const auto location :
		{ PluginLocation::era_plugins, PluginLocation::before_wog, PluginLocation::after_wog })
	{
		const auto locationIndex = static_cast<std::size_t>(location);
		const auto group         = groups[locationIndex];
		if (!group.IsOk())
			continue;

		const auto& desired = matchingPlugins[locationIndex];
		std::unordered_map<std::size_t, wxTreeItemId> existingItems;
		std::vector<wxTreeItemId>                     obsoleteItems;
		wxTreeItemIdValue                             itemCookie;
		for (auto item = _tree->GetFirstChild(group, itemCookie); item.IsOk();
			 item = _tree->GetNextChild(group, itemCookie))
		{
			const auto data = dynamic_cast<PluginTreeItemData*>(_tree->GetItemData(item));
			if (!data || std::ranges::find(desired, data->index) == desired.end() ||
				!existingItems.emplace(data->index, item).second)
				obsoleteItems.push_back(item);
		}
		for (const auto& item : obsoleteItems)
			_tree->Delete(item);

		wxTreeItemId previousItem;
		for (const auto index : desired)
		{
			wxTreeItemId item;
			if (const auto existing = existingItems.find(index); existing != existingItems.end())
				item = existing->second;
			else
			{
				const auto& plugin = _plugins[index];
				const auto  image  = pluginImage(plugin);
				const auto  label  = pluginCaption(plugin, _treeHasImages);
				auto        data   = new PluginTreeItemData(index);
				item = previousItem.IsOk()
					? _tree->InsertItem(group, previousItem, label, _treeHasImages ? image : -1,
						  _treeHasImages ? image : -1, data)
					: _tree->PrependItem(group, label, _treeHasImages ? image : -1,
						  _treeHasImages ? image : -1, data);
			}

			if (!firstPlugin.IsOk())
			{
				firstPlugin      = item;
				firstPluginIndex = index;
			}
			if (preferredSelection && *preferredSelection == index)
				preferredPlugin = item;

			previousItem = item;
		}

		if (!_tree->IsExpanded(group))
			_tree->Expand(group);
	}

	const auto selection = preferredPlugin.IsOk() ? preferredPlugin : firstPlugin;
	if (selection.IsOk())
	{
		const bool selectionChanged = _tree->GetSelection() != selection;
		if (selectionChanged)
			_tree->SelectItem(selection);
		if (focusSelection)
			_tree->SetFocusedItem(selection);
		if (selectionChanged)
			_tree->EnsureVisible(selection);
	}

	_populatingTree = false;
	const auto selectionIndex = preferredPlugin.IsOk() ? preferredSelection : firstPluginIndex;
	if (selectionIndex != preferredSelection)
		updateSelection();
}

bool PluginListView::pluginPassesFilter(const ModPlugin& plugin) const
{
	if (_filter.empty())
		return true;

	const auto location = locationLabel(plugin.location).ToStdString(wxConvUTF8);
	return std::ranges::any_of(
		std::initializer_list { plugin.canonical_filename, plugin.caption, plugin.description, location },
		[this](const std::string& value) {
			return boost::contains(boost::locale::fold_case(value), _filter);
		});
}

void PluginListView::focusSelectedPlugin()
{
	const auto selected = _tree->GetSelection();
	if (!selected.IsOk())
		return;

	_tree->SetFocus();
	_tree->SetFocusedItem(selected);
	_tree->EnsureVisible(selected);
}

std::optional<std::size_t> PluginListView::selectedPluginIndex()
{
	const auto selected = _tree->GetSelection();
	if (!selected.IsOk())
		return std::nullopt;

	const auto data = dynamic_cast<PluginTreeItemData*>(_tree->GetItemData(selected));
	if (!data || data->index >= _plugins.size())
		return std::nullopt;

	return data->index;
}

ModPlugin* PluginListView::selectedPlugin()
{
	const auto index = selectedPluginIndex();
	return index ? &_plugins[*index] : nullptr;
}

void PluginListView::updateSelection()
{
	const auto plugin = selectedPlugin();
	if (!plugin)
	{
		updateChangeStateButton(nullptr);
		_description->Clear();
		return;
	}

	updateChangeStateButton(plugin);
	_description->SetValue(wxString::FromUTF8(plugin->description));
}

void PluginListView::updateChangeStateButton(const ModPlugin* plugin)
{
	const bool     enabling = !plugin || !plugin->enabled;
	const auto     icon     = enabling ? Icon::Stock::checkmark_green : Icon::Stock::cross_gray;
	const wxString label    = enabling ? "dialog/button/enable"_lng : "dialog/button/disable"_lng;

	_changeState->SetBitmap(wxNullBitmap);
	_changeState->SetBitmap(_iconStorage.get(icon, Icon::Size::x16));
	if (wxGetApp().appConfig().interfaceLabel() == InterfaceLabel::show)
		_changeState->SetLabelText(label);
	_changeState->SetToolTip(label);
	_changeState->Enable(plugin != nullptr);
}

void PluginListView::updatePluginItem(const wxTreeItemId& item, const ModPlugin& plugin)
{
	if (_treeHasImages)
	{
		const auto image = pluginImage(plugin);
		_tree->SetItemImage(item, image, wxTreeItemIcon_Selected);
		_tree->SetItemImage(item, image, wxTreeItemIcon_Normal);
	}
	else
	{
		_tree->SetItemText(item, pluginCaption(plugin, false));
	}
}

void PluginListView::toggleSelected()
{
	const auto plugin = selectedPlugin();
	if (!plugin || !_mod)
		return;

	const auto item = _tree->GetSelection();
	try
	{
		_manager.toggle(*_mod, *plugin);
		plugin->enabled = !plugin->enabled;
		std::swap(plugin->current_path, plugin->counterpart_path);
		updatePluginItem(item, *plugin);
		updateChangeStateButton(plugin);
	}
	catch (const PluginToggleError& error)
	{
		wxString message;
		switch (error.code())
		{
		case PluginToggleErrorCode::collision:
			message = wxString::Format("message/error/plugin_collision"_lng,
				wxString::FromUTF8(plugin->canonical_filename),
				wxString::FromUTF8(plugin->canonical_filename));
			break;
		case PluginToggleErrorCode::missing:
			message = wxString::Format(
				"message/error/plugin_missing"_lng, wxString::FromUTF8(plugin->canonical_filename));
			break;
		case PluginToggleErrorCode::filesystem:
			message = wxString::Format("message/error/plugin_toggle_failed"_lng,
				wxString::FromUTF8(plugin->canonical_filename), wxStringFromUnspecified(error.what()));
			break;
		}

		wxMessageBox(message, wxTheApp->GetAppName(), wxOK | wxICON_ERROR, this);
	}
}
