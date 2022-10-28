// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "plugin_list_model.hpp"

#include "application.h"
#include "domain/imod_data_provider.hpp"
#include "domain/mod_data.hpp"
#include "domain/mod_list.hpp"
#include "interface/imod_manager.hpp"
#include "interface/service/iicon_storage.h"
#include "mod_manager_app.h"
#include "types/embedded_icon.h"
#include "utility/sdlexcept.h"
#include "utility/string_util.h"

#include <fmt/format.h>
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
	const wxString& item = _displayedItems[row];

	switch (static_cast<Column>(col))
	{
	case Column::state:
	{
		wxIcon icon;

		if (auto overriddenState = _items.overriddenState(item); overriddenState.has_value())
		{
			icon = _iconStorage.get(overriddenState == PluginState::enabled ? embedded_icon::tick
																			: embedded_icon::minus);
		}
		else
		{
			if (auto defaultState = _items.defaultState(item); defaultState.has_value())
			{
				icon =
					_iconStorage.get(defaultState->state == PluginState::enabled ? embedded_icon::tick_gray
																				: embedded_icon::minus_gray);
			}
		}

		variant = wxVariant(wxDataViewIconText("", icon));
		break;
	}
	case Column::caption:
	{
		variant = wxVariant(item);
		break;
	}
	case Column::mod:
	{
		wxIcon   icon;
		wxString caption;

		if (auto modIt = _items.state.find(item); modIt != _items.state.end())
		{
			const auto mod = _modDataProvider.modData(modIt->second.mod);
			if (!mod->icon_filename.empty())
				icon = _iconStorage.get((mod->data_path / mod->icon_filename).string());
			caption = mod->caption;
		}

		if (!icon.IsOk())
			icon = _iconStorage.get(embedded_icon::folder);

		variant = wxVariant(wxDataViewIconText(caption, icon));
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

	// no such item
	if (auto defaultState = _items.defaultState(item); !defaultState.has_value())
	{
		attr.SetBackgroundColour(wxColour(255, 127, 127));
		return true;
	}
	else if (auto overriden = _items.overriddenState(item); overriden.has_value() && overriden.value() == defaultState.value().state)
	{
		attr.SetBackgroundColour(*wxLIGHT_GREY);
		return true;
	}

	return false;
}

int PluginListModel::Compare(const wxDataViewItem& item1, const wxDataViewItem& item2, unsigned int column,
							 bool ascending) const
{
	auto compareRest = [&](unsigned int col) {
		return wxDataViewModel::Compare(item1, item2, col, ascending);
	};

	if (static_cast<Column>(column) != Column::state)
		return compareRest(column);

	return compareRest(static_cast<unsigned int>(Column::caption));
}

void PluginListModel::setList(const PluginList& items)
{
	_items = items;

	reload();
}

void PluginListModel::reload()
{
	_displayedItems.clear();
	for (const auto& item : _items.available)
	{
		bool display = _showHidden || _items.overriddenState(item).has_value();
		if (!display)
		{
			const auto value = _items.defaultState(item);
			display          = value.has_value() && value->state == PluginState::disabled;
		}

		if (display)
			_displayedItems.emplace_back(item);

	}

	Reset(_displayedItems.size());
}

wxString PluginListModel::findIdByItem(const wxDataViewItem& item) const
{
	if (!item.IsOk())
		return wxEmptyString;

	const auto row = GetRow(item);

	return _displayedItems[row];
}

wxDataViewItem PluginListModel::findItemById(const wxString& plugin) const
{
	for (size_t i = 0; i < _displayedItems.size(); ++i)
		if (_displayedItems[i] == plugin)
			return GetItem(i);

	return {};
}
