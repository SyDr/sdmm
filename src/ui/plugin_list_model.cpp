// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "plugin_list_model.hpp"

#include "application.h"
#include "domain/mod_data.hpp"
#include "domain/mod_list.hpp"
#include "interface/iicon_storage.h"
#include "interface/imod_data_provider.hpp"
#include "interface/imod_manager.hpp"
#include "mod_manager_app.h"
#include "type/embedded_icon.h"
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
		const auto& iconRef = [&] {
			using namespace embedded_icon;
 			if (_items.managed.contains(item))
				return !item.active() ? tick : minus;
			else
				return item.active() ? tick_gray : minus_gray;
		}();

		variant = wxVariant(wxDataViewIconText("", _iconStorage.get(iconRef)));
		break;
	}
	case Column::caption:
	{
		variant = wxVariant(item.toString());
		break;
	}
	case Column::mod:
	{
		wxIcon   icon;
		wxString caption;

		const auto mod = _modDataProvider.modData(item.modId);
		if (!mod->icon_filename.empty())
			icon = _iconStorage.get((mod->data_path / mod->icon_filename).string());
		caption = mod->caption;

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

	if (!_items.available.contains(item))
	{
		attr.SetBackgroundColour(wxColour(255, 127, 127));
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

	if (static_cast<Column>(column) != Column::state)
		return compareRest(column);

	return compareRest(static_cast<unsigned int>(Column::caption));
}

void PluginListModel::setList(const PluginList& items)
{
	_items = items;

	reload();
}

void PluginListModel::showAll(bool value)
{
	_showHidden = value;
	reload();
}

void PluginListModel::reload()
{
	_displayedItems.clear();

	for (const auto& item : _items.available)
	{
		bool display = _showHidden || !item.active() || _items.managed.contains(item);

		if (display)
			_displayedItems.emplace_back(item);
	}

	for (const auto& item : _items.managed)
	{
		if (_items.available.contains(item))
			continue;

		_displayedItems.emplace_back(item);
	}

	Reset(_displayedItems.size());
}

PluginSource PluginListModel::findIdByItem(const wxDataViewItem& item) const
{
	if (!item.IsOk())
		return {};

	const auto row = GetRow(item);

	return _displayedItems[row];
}

wxDataViewItem PluginListModel::findItemById(const PluginSource& item) const
{
	for (size_t i = 0; i < _displayedItems.size(); ++i)
		if (_displayedItems[i] == item)
			return GetItem(i);

	return {};
}
