// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "mod_list_model.h"

#include "application.h"
#include "domain/imod_data_provider.hpp"
#include "interface/imod_manager.hpp"
#include "domain/mod_data.hpp"
#include "domain/mod_list.hpp"
#include "interface/service/iicon_storage.h"
#include "mod_manager_app.h"
#include "types/embedded_icon.h"
#include "utility/sdlexcept.h"
#include "utility/string_util.h"

#include <wx/app.h>
#include <wx/msgdlg.h>

using namespace mm;

ModListModel::ModListModel(IModDataProvider& modDataProvider, IIconStorage& iconStorage,
						   bool showHidden)
	: _modDataProvider(modDataProvider)
	, _iconStorage(iconStorage)
	, _showHidden(showHidden)
{
	reload();
}

unsigned int ModListModel::GetColumnCount() const
{
	return static_cast<unsigned int>(Column::total);
}

wxString ModListModel::GetColumnType(unsigned int col) const
{
	switch (static_cast<Column>(col))
	{
	case Column::priority:
	case Column::caption: return wxDataViewIconTextRenderer::GetDefaultType();
	case Column::author:
	case Column::category:
	case Column::version: return wxDataViewTextRenderer::GetDefaultType();
	case Column::checkbox: return wxDataViewToggleRenderer::GetDefaultType();
	}

	return wxEmptyString;
}

void ModListModel::GetValueByRow(wxVariant& variant, unsigned row, unsigned col) const
{
	const auto& item = _displayedItems[row];

	switch (static_cast<Column>(col))
	{
	case Column::priority:
	{
		wxIcon   icon;
		wxString text;

		if (bool const active = _list.isActive(item))
		{
			text = wxString::Format("%u", row);
			icon = _iconStorage.get(embedded_icon::tick);
		}

		variant = wxVariant(wxDataViewIconText(text, icon));
		break;
	}
	case Column::caption:
	{
		wxIcon icon;

		const auto mod = _modDataProvider.modData(item);

		if (!mod->icon_filename.empty())
			icon = _iconStorage.get((mod->data_path / mod->icon_filename).string());
		else
			icon = _iconStorage.get(embedded_icon::folder);

		variant = wxVariant(wxDataViewIconText(mod->caption, icon));
		break;
	}
	case Column::author:
	{
		const auto mod = _modDataProvider.modData(item);
		variant        = wxVariant(mod->authors);
		break;
	}
	case Column::category:
	{
		const auto mod = _modDataProvider.modData(item);
		variant        = wxVariant(wxGetApp().categoryTranslationString(mod->category));
		break;
	}
	case Column::version:
	{
		const auto mod = _modDataProvider.modData(item);
		variant        = wxVariant(mod->mod_version);
		break;
	}
	case Column::checkbox:
	{
		variant = wxVariant(_checked.contains(item));
		break;
	}
	}
}

bool ModListModel::SetValueByRow(const wxVariant&, unsigned row, unsigned col)
{
	switch (static_cast<Column>(col))
	{
	case Column::checkbox:
		const auto& item = _displayedItems[row];

		if (auto it = _checked.find(item); it != _checked.cend())
			_checked.erase(it);
		else
			_checked.emplace(item);

		return true;
	}

	return false;
}

bool ModListModel::GetAttrByRow(unsigned row, unsigned, wxDataViewItemAttr& attr) const
{
	const auto& mod = _displayedItems[row];

	if (_list.hidden.count(mod))
	{
		attr.SetBackgroundColour(*wxLIGHT_GREY);
		return true;
	}

	if (_modDataProvider.modData(mod)->virtual_mod)
	{
		attr.SetBackgroundColour(wxColour(255, 127, 127));
	}

	return false;
}

int ModListModel::Compare(const wxDataViewItem& item1, const wxDataViewItem& item2,
						  unsigned int column, bool ascending) const
{
	auto compareRest = [&](unsigned int col) {
		return wxDataViewIndexListModel::Compare(item1, item2, col, ascending);
	};

	if (static_cast<Column>(column) == Column::caption)
		return compareRest(column);

	if (static_cast<Column>(column) == Column::priority)
	{
		const auto row1 = GetRow(item1);
		const auto row2 = GetRow(item2);

		bool const active1 = _list.isActive(_displayedItems[GetRow(item1)]);
		bool const active2 = _list.isActive(_displayedItems[GetRow(item2)]);

		if (!active1 && !active2)
			return Compare(item1, item2, static_cast<unsigned int>(Column::category), ascending);

		if (active1 && active2)
			return ascending ? row1 - row2 : row2 - row1;

		if (active1)
			return -1;

		return 1;
	}

	if (auto res = compareRest(column))
		return res;

	return compareRest(static_cast<unsigned int>(Column::caption));
}

void ModListModel::setModList(ModList const& mods)
{
	_list = mods;
	reload();
}

void ModListModel::setChecked(std::unordered_set<wxString> items)
{
	_checked = std::move(items);
	Reset(_displayedItems.size());
}

std::unordered_set<wxString> const& ModListModel::getChecked() const
{
	return _checked;
}

void ModListModel::reload()
{
	_displayedItems = _list.active;

	if (_showInactive)
	{
		for (const auto& mod : _list.available)
		{
			if (!_list.isActive(mod) && (_showHidden || !_list.hidden.count(mod)))
				_displayedItems.emplace_back(mod);
		}
	}

	Reset(_displayedItems.size());
}

const ModData* ModListModel::findMod(const wxDataViewItem& item) const
{
	if (!item.IsOk())
		return nullptr;

	return _modDataProvider.modData(_displayedItems[GetRow(item)]);
}

wxDataViewItem ModListModel::findItemById(const wxString& id) const
{
	for (size_t i = 0; i < _displayedItems.size(); ++i)
		if (_displayedItems[i] == id)
			return GetItem(i);

	return {};
}

wxString ModListModel::findIdByItem(wxDataViewItem const& item) const
{
	if (!item.IsOk())
		return wxEmptyString;

	return _displayedItems[GetRow(item)];
}

void ModListModel::showHidden(bool show)
{
	_showHidden = show;
	reload();
}

void ModListModel::showInactive(bool show)
{
	_showInactive = show;
	reload();
}
