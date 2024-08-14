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
		container = 0,
		item      = 1,
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

ModListModel::ModListModel(
	IModDataProvider& modDataProvider, IIconStorage& iconStorage, ModListModelMode mode)
	: _modDataProvider(modDataProvider)
	, _iconStorage(iconStorage)
	, _mode(mode)
{
	reload();
}

bool ModListModel::IsListModel() const
{
	return _mode == ModListModelMode::flat;
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
	case Column::version:
	case Column::directory: return wxDataViewTextRenderer::GetDefaultType();
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

bool ModListModel::HasContainerColumns(const wxDataViewItem&) const
{
	return true;
}

wxDataViewItem ModListModel::GetParent(const wxDataViewItem& item) const
{
	const auto [type, row] = fromDataViewItem(item);

	if (type == ItemType::container)
		return wxDataViewItem();

	const auto pos     = _list.position(_displayed.items[row]);
	const auto enabled = _list.enabled(_displayed.items[row]);

	if (_mode == ModListModelMode::flat || (_mode == ModListModelMode::modern && pos))
		return wxDataViewItem();

	for (size_t i = 0; i < _displayed.categories.size(); ++i)
	{
		if (std::visit(
				[&](auto&& arg) {
					using T = std::decay_t<decltype(arg)>;
					if constexpr (std::is_same_v<T, ModListDsplayedData::EnabledGroupTag>)
						return enabled;
					else if constexpr (std::is_same_v<T, ModListDsplayedData::ArchivedGroupTag>)
						return !pos;
					else  // T == std::string
						return _modDataProvider.modData(_displayed.items[row]).category == arg;
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
		if (_mode == ModListModelMode::flat)
		{
			for (size_t i = 0; i < _displayed.items.size(); ++i)
				children.push_back(toDataViewItem(i, ItemType::item));
		}

		if (_mode == ModListModelMode::modern)
		{
			for (size_t i = 0; i < _displayed.items.size(); ++i)
				if (_list.position(_displayed.items[i]))
					children.push_back(toDataViewItem(i, ItemType::item));

			for (size_t i = 0; i < _displayed.categories.size(); ++i)
				children.push_back(toDataViewItem(i, ItemType::container));
		}

		if (_mode == ModListModelMode::classic)
		{
			for (size_t i = 0; i < _displayed.categories.size(); ++i)
				children.push_back(toDataViewItem(i, ItemType::container));
		}

		return children.size();
	}

	const auto [type, index] = fromDataViewItem(item);

	std::visit(
		[&](auto&& arg) {
			using T = std::decay_t<decltype(arg)>;
			if constexpr (std::is_same_v<T, ModListDsplayedData::EnabledGroupTag>)
			{
				for (size_t i = 0; i < _displayed.items.size(); ++i)
					if (_list.enabled(_displayed.items[i]))
						children.push_back(toDataViewItem(i, ItemType::item));
			}
			else if constexpr (std::is_same_v<T, ModListDsplayedData::ArchivedGroupTag>)
			{
				for (size_t i = 0; i < _displayed.items.size(); ++i)
					if (_list.rest.contains(_displayed.items[i]))
						children.push_back(toDataViewItem(i, ItemType::item));
			}
			else  // T == std::string
			{
				for (size_t i = 0; i < _displayed.items.size(); ++i)
				{
					const bool show = _mode == ModListModelMode::modern ? !_list.position(_displayed.items[i])
																		: !_list.enabled(_displayed.items[i]);

					if (show && _modDataProvider.modData(_displayed.items[i]).category == arg)
						children.push_back(toDataViewItem(i, ItemType::item));
				}
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
		switch (static_cast<Column>(col))
		{
		case Column::priority:
		{
			if (_mode != ModListModelMode::modern)
				variant = wxVariant(wxDataViewIconText(
					L"", _iconStorage.get(!index ? embedded_icon::tick_green : embedded_icon::blank)));
			break;
		}
		case Column::name:
		{
			variant = wxVariant(wxDataViewIconText(_displayed.categories[index].second, wxIcon()));
			break;
		}
		}
		return;
	}

	const auto& id       = _displayed.items[index];
	const auto& mod      = _modDataProvider.modData(id);
	const auto  position = _list.position(id);

	switch (static_cast<Column>(col))
	{
	case Column::priority:
	{
		wxIcon   icon;
		wxString text;

		if (position)
		{
			text = wxString::Format(L"%u", *position);

			const auto& icon_ = [&]() {
				switch (*_list.state(id))
				{
				case ModList::ModState::enabled: return embedded_icon::tick_green;
				case ModList::ModState::disabled: return embedded_icon::cross_gray;
				}

				return embedded_icon::blank;
			}();

			icon = _iconStorage.get(icon_);
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
	case Column::directory:
	{
		variant = wxVariant(wxString::FromUTF8(mod.id));
		break;
	}
	case Column::checkbox:
	{
		variant = wxVariant(_checked.contains(id));
		break;
	}
	case Column::status:
	{
		variant = wxVariant(wxDataViewIconText(
			L"", _iconStorage.get(position ? embedded_icon::tick_green : embedded_icon::cross_gray)));
		break;
	}
	case Column::load_order:
	{
		wxString value;
		if (position)
			value = wxString::Format(L"%u", *position);

		variant = wxVariant(value);
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

	if (_modDataProvider.modData(mod).virtual_mod)
		attr.SetBackgroundColour(wxColour(255, 127, 127));

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

	if (static_cast<Column>(column) == Column::priority)
	{
		const auto pos1 = _list.position(_displayed.items[index1]);
		const auto pos2 = _list.position(_displayed.items[index2]);

		if (!pos1 && !pos2)
			return wxDataViewModel::Compare(item1, item2, static_cast<unsigned int>(Column::name), true);

		if (pos1 && pos2)
			return ascending ? static_cast<ssize_t>(index1) - static_cast<ssize_t>(index2)
							 : static_cast<ssize_t>(index2) - static_cast<ssize_t>(index1);

		if (pos1)
			return -1;

		return 1;
	}

	if (static_cast<Column>(column) == Column::category)
	{
		const auto& cat1 = _modDataProvider.modData(_displayed.items[index1]).category;
		const auto& cat2 = _modDataProvider.modData(_displayed.items[index2]).category;

		if (cat1.empty() != cat2.empty())
			return ascending ? static_cast<ssize_t>(cat1.empty()) - static_cast<ssize_t>(cat2.empty())
							 : static_cast<ssize_t>(cat2.empty()) - static_cast<ssize_t>(cat1.empty());
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
	_displayed.items.clear();

	for (const auto& mod : _list.data)
		_displayed.items.emplace_back(mod.id);

	for (const auto& mod : _list.rest)
		_displayed.items.emplace_back(mod);

	size_t                                  enabledCount = 0;
	std::unordered_map<std::string, size_t> cats;
	size_t                                  archivedCount = 0;

	for (const auto& id : _displayed.items)
	{
		if (_mode == ModListModelMode::classic)
		{
			if (!_list.enabled(id))
				++cats[_modDataProvider.modData(id).category];
			else
				++enabledCount;
		}
		else if (_mode == ModListModelMode::modern)
		{
			auto pos = _list.position(id);
			if (!pos)
				++archivedCount;
		}
	}

	if (enabledCount)
	{
		_displayed.categories.emplace_back(ModListDsplayedData::EnabledGroupTag(),
			wxString::Format(L"%s (%d)", "Enabled"_lng, enabledCount));
	}

	for (const auto& cat : cats)
		_displayed.categories.emplace_back(cat.first,
			wxString::Format(L"%s (%d)",
				cat.first.empty() ? "Without category"_lng
								  : wxString::FromUTF8(wxGetApp().categoryTranslationString(cat.first)),
				cat.second));

	if (archivedCount)
	{
		_displayed.categories.emplace_back(ModListDsplayedData::ArchivedGroupTag(),
			wxString::Format(L"%s (%d)", "Archived"_lng, archivedCount));
	}

	std::sort(_displayed.categories.begin(), _displayed.categories.end(),
		[](const ModListDsplayedData::CategoryAndCaption&  left,
			const ModListDsplayedData::CategoryAndCaption& right) {
			if (left.first.index() != right.first.index())
				return left.first.index() < right.first.index();

			auto lstr = std::get_if<std::string>(&left.first);
			auto rstr = std::get_if<std::string>(&right.first);

			if (lstr && rstr)
			{
				if (lstr->empty())
					return false;

				if (rstr->empty())
					return true;
			}

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

std::string ModListModel::findIdByItem(const wxDataViewItem& item) const
{
	if (!item.IsOk())
		return {};

	const auto [type, index] = fromDataViewItem(item);
	if (type == ItemType::container)
		return {};

	return _displayed.items[index];
}

std::optional<ModListDsplayedData::GroupItemsBy> ModListModel::itemGroupByItem(
	const wxDataViewItem& item) const
{
	if (!item.IsOk())
		return {};

	const auto [type, index] = fromDataViewItem(item);
	if (type != ItemType::container)
		return {};

	return _displayed.categories[index].first;
}
