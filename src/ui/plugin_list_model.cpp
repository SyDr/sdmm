// SD Mod Manager

// Copyright (c) 2026 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "plugin_list_model.hpp"

#include "application.h"
#include "domain/mod_data.hpp"
#include "domain/mod_list.hpp"
#include "icon_helper.hpp"
#include "interface/iicon_storage.hpp"
#include "interface/imod_data_provider.hpp"
#include "interface/imod_manager.hpp"
#include "mod_manager_app.h"
#include "type/icon.hpp"
#include "utility/sdlexcept.h"

#include <wx/app.h>
#include <wx/msgdlg.h>

using namespace mm;

PluginListModel::PluginListModel(IModDataProvider& modDataProvider, IIconStorage& iconStorage)
	: _modDataProvider(modDataProvider)
	, _iconStorage(iconStorage)
{
	reload();
}

unsigned int PluginListModel::GetColumnCount() const
{
	return static_cast<unsigned int>(Column::total);
}

wxString PluginListModel::GetColumnType(unsigned int col) const
{
	switch (static_cast<Column>(col))
	{
	case Column::state: return wxDataViewIconTextRenderer::GetDefaultType();
	case Column::caption: return wxDataViewTextRenderer::GetDefaultType();
	case Column::mod: return wxDataViewIconTextRenderer::GetDefaultType();
	}

	return wxEmptyString;
}

void PluginListModel::GetValueByRow(wxVariant& variant, unsigned row, unsigned col) const
{
	const auto& item = _displayedItems[row];

	switch (static_cast<Column>(col))
	{
	case Column::state:
	{
		Icon::Stock icon  = item.source.enabled() ? Icon::Stock::checkmark : Icon::Stock::cross;
		wxColour    color = *wxLIGHT_GREY;

		if (const auto it = _items.managed.find(item.id); it != _items.managed.end())
		{
			color = Icon::MM_GREEN;
			icon  = it->second == PluginState::forced_enable ? Icon::Stock::checkmark : Icon::Stock::cross;
		}

		variant = wxVariant(wxDataViewIconText(L"", _iconStorage.get(icon, {}, color)));
		break;
	}
	case Column::caption:
	{
		variant = wxVariant(wxString::FromUTF8(item.source.v.string()));
		break;
	}
	case Column::mod:
	{
		const auto& mod = _modDataProvider.modData(item.modId);

		variant = wxVariant(wxDataViewIconText(wxString::FromUTF8(mod.name),
			loadModIcon(_iconStorage, mod.data_path, mod.icon, Icon::Size::x16)));
		break;
	}
	}
}

bool PluginListModel::SetValueByRow(const wxVariant&, unsigned, unsigned)
{
	return false;
}

bool PluginListModel::GetAttrByRow(unsigned row, unsigned, wxDataViewItemAttr& attr) const
{
	const auto& item = _displayedItems[row];

	// 	if (!_items.available.contains(item.id))
	// 	{
	// 		attr.SetBackgroundColour(*wxRED);
	// 		return true;
	// 	}

	if (!_mods.enabled(item.modId))
	{
		attr.SetBackgroundColour(*wxLIGHT_GREY);
		return true;
	}

	return false;
}

int PluginListModel::Compare(
	const wxDataViewItem& item1, const wxDataViewItem& item2, unsigned int column, bool ascending) const
{
	auto compareRest = [&](unsigned int col) {
		return wxDataViewModel::Compare(item1, item2, col, ascending);
	};

	auto compareName = [&]() {
		wxVariant text1;
		wxVariant text2;

		GetValue(text1, item1, column);
		GetValue(text2, item2, column);

		const wxString left  = text1.GetString();
		const wxString right = text2.GetString();

		return ascending ? left.CmpNoCase(right) : right.CmpNoCase(left);
	};
	if (static_cast<Column>(column) == Column::caption)
		return compareName();

	if (static_cast<Column>(column) != Column::state)
		return compareRest(column);

	return compareName();
}

void PluginListModel::setList(const PluginList& items)
{
	_items = items;

	reload();
}

void PluginListModel::showHidden(bool value)
{
	_showHidden = value;
	reload();
}

void PluginListModel::showFromAllMods(bool value)
{
	_showFromAllMods = value;
	reload();
}

void PluginListModel::reload()
{
	_displayedItems.clear();

	for (const auto& [id, v] : _items.available)
	{
		for (const auto& [modId, source] : v)
		{
			const bool show = [&] {
				const bool showFromMod = _showFromAllMods || _mods.enabled(modId);
				const bool showPlugin  = _showHidden || !source.enabled() || _items.managed.contains(id);

				return showFromMod && showPlugin;
			}();

			if (show)
				_displayedItems.emplace_back(Plugin(modId, source.v));
		}
	}

	Reset(_displayedItems.size());
}

std::optional<Plugin> PluginListModel::findIdByItem(const wxDataViewItem& item) const
{
	if (!item.IsOk())
		return {};

	const auto row = GetRow(item);

	return _displayedItems[row];
}

wxDataViewItem PluginListModel::findItemById(const Plugin& item) const
{
	for (size_t i = 0; i < _displayedItems.size(); ++i)
		if (_displayedItems[i] == item)
			return GetItem(i);

	return {};
}

void PluginListModel::modList(const ModList& mods)
{
	_mods = mods;

	reload();
}
