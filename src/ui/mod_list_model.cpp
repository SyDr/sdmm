// SD Mod Manager

// Copyright (c) 2020-2024 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "mod_list_model.h"

#include "application.h"
#include "domain/mod_data.hpp"
#include "domain/mod_list.hpp"
#include "icon_helper.hpp"
#include "interface/iicon_storage.hpp"
#include "interface/imod_data_provider.hpp"
#include "interface/imod_manager.hpp"
#include "mod_manager_app.h"
#include "type/embedded_icon.h"
#include "utility/sdlexcept.h"

#include <wx/app.h>
#include <wx/msgdlg.h>

using namespace mm;

namespace
{
	static constexpr auto categoryLimit = 1'000'000;

	enum class ItemType
	{
		container,
		item,
	};

	wxDataViewItem toDataViewItem(size_t index, ItemType type)
	{
		return wxDataViewItem(reinterpret_cast<void*>(static_cast<long>(type) * categoryLimit + index + 1));
	}

	std::pair<ItemType, size_t> fromDataViewItem(const wxDataViewItem& item)
	{
		const auto casted = reinterpret_cast<size_t>(item.GetID()) - 1;

		return { static_cast<ItemType>(casted / categoryLimit), casted % categoryLimit };
	}
}

ModListModel::ModListModel(IModDataProvider& modDataProvider, IIconStorage& iconStorage, bool showHidden)
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
	case Column::name: return wxDataViewIconTextRenderer::GetDefaultType();
	case Column::author:
	case Column::category:
	case Column::load_order:
	case Column::version: return wxDataViewTextRenderer::GetDefaultType();
	case Column::checkbox: return wxDataViewToggleRenderer::GetDefaultType();
	case Column::status: return wxDataViewBitmapRenderer::GetDefaultType();
	}

	return wxEmptyString;
}

bool ModListModel::IsContainer(const wxDataViewItem& item) const
{
	if (!item.IsOk())
		return true;

	const auto [type, row] = fromDataViewItem(item);

	return type == ItemType::container;
}

wxDataViewItem ModListModel::GetParent(const wxDataViewItem& item) const
{
	const auto [type, row] = fromDataViewItem(item);

	if (type == ItemType::container)
		return wxDataViewItem();

	if (_list.activePosition(_displayed.items[row]).has_value())
		return toDataViewItem(0, ItemType::container);

	for (size_t i = 0; i < _displayed.categories.size(); ++i)
	{
		if (std::visit(
				[&](auto&& arg) {
					using T = std::decay_t<decltype(arg)>;
					if constexpr (std::is_same_v<T, std::monostate>)
						return _list.activePosition(_displayed.items[i]).has_value();
					else  // T == std::string
						return _modDataProvider.modData(_displayed.items[i]).category == arg;
				},
				_displayed.categories[i].first))
			return toDataViewItem(i, ItemType::container);
	}

	return wxDataViewItem();
}

unsigned int ModListModel::GetChildren(const wxDataViewItem& item, wxDataViewItemArray& children) const
{
	if (!item.IsOk())
	{
		for (size_t i = 0; i < _displayed.categories.size(); ++i)
			children.push_back(toDataViewItem(i, ItemType::container));

		return children.size();
	}

	const auto [type, index] = fromDataViewItem(item);

	std::visit(
		[&](auto&& arg) {
			using T = std::decay_t<decltype(arg)>;
			if constexpr (std::is_same_v<T, std::monostate>)
			{
				for (size_t i = 0; i < _displayed.items.size(); ++i)
					if (_list.activePosition(_displayed.items[i]).has_value())
						children.push_back(toDataViewItem(i, ItemType::item));
			}
			else  // T == std::string
			{
				for (size_t i = 0; i < _displayed.items.size(); ++i)
					if (!_list.activePosition(_displayed.items[i]).has_value() &&
						_modDataProvider.modData(_displayed.items[i]).category == arg)
						children.push_back(toDataViewItem(i, ItemType::item));
			}
		},
		_displayed.categories[index].first);

	return children.size();
}

void ModListModel::GetValue(wxVariant& variant, const wxDataViewItem& item, unsigned int col) const
{
	if (!item.IsOk())
		return;

	const auto [type, index] = fromDataViewItem(item);

	if (type == ItemType::container)
	{
		variant = wxVariant(
			wxDataViewIconText(_displayed.categories[index].second, _iconStorage.get(embedded_icon::tick)));
		return;
	}

	const auto& rowData = _displayed.items[index];
	const auto& mod     = _modDataProvider.modData(rowData);

	switch (static_cast<Column>(col))
	{
	case Column::priority:
	{
		wxIcon   icon;
		wxString text;

		if (bool const active = _list.isActive(rowData))
		{
			text = wxString::Format(L"%u", index);
			icon = _iconStorage.get(embedded_icon::tick);
		}

		variant = wxVariant(wxDataViewIconText(text, icon));
		break;
	}
	case Column::name:
	{
		variant = wxVariant(wxDataViewIconText(
			wxString::FromUTF8(mod.name), loadModIcon(_iconStorage, mod.data_path, mod.icon)));
		break;
	}
	case Column::author:
	{
		variant = wxVariant(wxString::FromUTF8(mod.author));
		break;
	}
	case Column::category:
	{
		variant = wxVariant(wxString::FromUTF8(wxGetApp().categoryTranslationString(mod.category)));
		break;
	}
	case Column::version:
	{
		variant = wxVariant(wxString::FromUTF8(mod.version));
		break;
	}
	case Column::checkbox:
	{
		variant = wxVariant(_checked.contains(rowData));
		break;
	}
	case Column::status:
	{
		variant = wxVariant(
			wxDataViewIconText(L"", _iconStorage.get(_list.isActive(rowData) ? embedded_icon::tick_green
																			 : embedded_icon::cross_gray)));
		break;
	}
	case Column::load_order:
	{
		variant = wxVariant(_list.isActive(rowData) ? wxString::Format(L"%u", index) : wxString());
		break;
	}
	}
}

bool ModListModel::SetValue(const wxVariant&, const wxDataViewItem& item, unsigned int col)
{
	const auto [type, index] = fromDataViewItem(item);

	if (type == ItemType::container)
		return false;

	switch (static_cast<Column>(col))
	{
	case Column::checkbox:
		const auto& myItem = _displayed.items[index];

		if (auto it = _checked.find(myItem); it != _checked.cend())
			_checked.erase(it);
		else
			_checked.emplace(myItem);

		return true;
	}

	return false;
}

bool ModListModel::GetAttr(const wxDataViewItem& item, unsigned int, wxDataViewItemAttr& attr) const
{
	const auto [type, index] = fromDataViewItem(item);
	if (type == ItemType::container)
		return false;

	const auto& mod = _displayed.items[index];

	if (_list.hidden.count(mod))
	{
		attr.SetBackgroundColour(*wxLIGHT_GREY);
		return true;
	}

	if (_modDataProvider.modData(mod).virtual_mod)
	{
		attr.SetBackgroundColour(wxColour(255, 127, 127));
	}

	return false;
}

int ModListModel::Compare(
	const wxDataViewItem& item1, const wxDataViewItem& item2, unsigned int column, bool ascending) const
{
	const auto [type1, index1] = fromDataViewItem(item1);
	const auto [type2, index2] = fromDataViewItem(item2);

	auto compareRest = [&](Column col) {
		return wxDataViewModel::Compare(item1, item2, static_cast<unsigned int>(col), ascending);
	};

	if (type1 != type2)
		return type1 < type2;

	if (type1 == ItemType::container)
		return static_cast<ssize_t>(index1) - static_cast<ssize_t>(index2);

	if (static_cast<Column>(column) == Column::priority || static_cast<Column>(column) == Column::status ||
		static_cast<Column>(column) == Column::load_order)
	{
		const bool active1 = _list.isActive(_displayed.items[index1]);
		const bool active2 = _list.isActive(_displayed.items[index2]);

		if (!active1 && !active2)
			return wxDataViewModel::Compare(item1, item2, static_cast<unsigned int>(Column::name), true);

		if (active1 && active2)
			return ascending ? static_cast<ssize_t>(index1) - static_cast<ssize_t>(index2)
							 : static_cast<ssize_t>(index2) - static_cast<ssize_t>(index1);

		if (active1)
			return -1;

		return 1;
	}

	if (auto res = wxDataViewModel::Compare(item1, item2, column, ascending); res != 0)
		return res;

	return compareRest(Column::name);
}

void ModListModel::setModList(ModList const& mods)
{
	_list = mods;
	reload();
}

void ModListModel::setChecked(std::unordered_set<std::string> items)
{
	_checked = std::move(items);
	Cleared();
}

std::unordered_set<std::string> const& ModListModel::getChecked() const
{
	return _checked;
}

void ModListModel::reload()
{
	_displayed.categories.clear();
	_displayed.items = _list.active;

	if (_showInactive)
	{
		for (const auto& mod : _list.available)
		{
			if (!_list.isActive(mod) && (_showHidden || !_list.hidden.count(mod)))
				_displayed.items.emplace_back(mod);
		}
	}

	if (!_list.active.empty())
		_displayed.categories.emplace_back(
			std::monostate(), wxString::Format(L"%s (%d)", "Active"_lng, _list.active.size()));

	std::unordered_map<std::string, size_t> cats;
	for (const auto& item : _displayed.items)
		if (!_list.activePosition(item).has_value())
			cats[_modDataProvider.modData(item).category]++;

	for (const auto& cat : cats)
		_displayed.categories.emplace_back(cat.first,
			wxString::Format(L"%s (%d)",
				cat.first.empty() ? "Without category"_lng
								  : wxString::FromUTF8(wxGetApp().categoryTranslationString(cat.first)),
				cat.second));

	std::sort(_displayed.categories.begin(), _displayed.categories.end(),
		[](const DisplayedData::cat_plus_display& left, const DisplayedData::cat_plus_display& right) {
			if (std::holds_alternative<std::monostate>(left.first))
				return true;

			if (std::holds_alternative<std::monostate>(right.first))
				return false;

			if (std::get<std::string>(left.first).empty())
				return false;

			if (std::get<std::string>(right.first).empty())
				return true;

			return left.second < right.second;
		});

	Cleared();
}

const ModData* ModListModel::findMod(const wxDataViewItem& item) const
{
	if (!item.IsOk())
		return nullptr;

	const auto [type, index] = fromDataViewItem(item);
	if (type == ItemType::container)
		return nullptr;

	return &_modDataProvider.modData(_displayed.items[index]);
}

wxDataViewItem ModListModel::findItemById(const std::string& id) const
{
	for (size_t i = 0; i < _displayed.items.size(); ++i)
		if (_displayed.items[i] == id)
			return toDataViewItem(i, ItemType::item);

	return {};
}

std::string ModListModel::findIdByItem(wxDataViewItem const& item) const
{
	if (!item.IsOk())
		return {};

	const auto [type, index] = fromDataViewItem(item);
	if (type == ItemType::container)
		return {};

	return _displayed.items[index];
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
