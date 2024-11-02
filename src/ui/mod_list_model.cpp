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

#include <boost/locale.hpp>
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

ModListModel::ModListModel(IModDataProvider& modDataProvider, IIconStorage& iconStorage,
	ModListModelManagedMode managedMode, ModListModelArchivedMode archivedMode)
	: _modDataProvider(modDataProvider)
	, _iconStorage(iconStorage)
	, _managedMode(managedMode)
	, _archivedMode(archivedMode)
{
	reload();
}

bool ModListModel::IsListModel() const
{
	return _managedMode == ModListModelManagedMode::as_flat_list &&
		   _archivedMode == ModListModelArchivedMode::as_flat_list;
}

unsigned int ModListModel::GetColumnCount() const
{
	return static_cast<unsigned int>(ModListModelColumn::total);
}

wxString ModListModel::GetColumnType(unsigned int col) const
{
	switch (static_cast<ModListModelColumn>(col))
	{
	case ModListModelColumn::priority:
	case ModListModelColumn::name: return wxDataViewIconTextRenderer::GetDefaultType();
	case ModListModelColumn::author:
	case ModListModelColumn::category:
	case ModListModelColumn::load_order:
	case ModListModelColumn::version:
	case ModListModelColumn::directory: return wxDataViewTextRenderer::GetDefaultType();
	case ModListModelColumn::checkbox: return wxDataViewToggleRenderer::GetDefaultType();
	case ModListModelColumn::status: return wxDataViewBitmapRenderer::GetDefaultType();
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

	if (pos)
	{
		if (_managedMode == ModListModelManagedMode::as_flat_list)
			return wxDataViewItem();
	}
	else
	{
		if (_archivedMode == ModListModelArchivedMode::as_flat_list)
			return wxDataViewItem();
	}

	for (size_t i = 0; i < _displayed.categories.size(); ++i)
	{
		if (std::visit(
				[&](auto&& arg) {
					using T = std::decay_t<decltype(arg)>;
					if constexpr (std::is_same_v<T, ModListDsplayedData::ManagedGroupTag>)
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
		for (size_t i = 0; i < _displayed.categories.size(); ++i)
			children.push_back(toDataViewItem(i, ItemType::container));

		for (size_t i = 0; i < _displayed.items.size(); ++i)
		{
			if (_list.position(_displayed.items[i]))
			{
				if (_managedMode == ModListModelManagedMode::as_flat_list)
					children.push_back(toDataViewItem(i, ItemType::item));
			}
			else
			{
				if (_archivedMode == ModListModelArchivedMode::as_flat_list)
					children.push_back(toDataViewItem(i, ItemType::item));
			}
		}

		return children.size();
	}

	const auto [type, index] = fromDataViewItem(item);

	std::visit(
		[&](auto&& arg) {
			using T = std::decay_t<decltype(arg)>;
			if constexpr (std::is_same_v<T, ModListDsplayedData::ManagedGroupTag>)
			{
				for (size_t i = 0; i < _displayed.items.size(); ++i)
					if (!_list.rest.contains(_displayed.items[i]))
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
					const bool show = _list.rest.contains(_displayed.items[i]);

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
		switch (static_cast<ModListModelColumn>(col))
		{
		case ModListModelColumn::priority:
		{
			variant = wxVariant(wxDataViewIconText(L"", _iconStorage.get(embedded_icon::blank)));
			break;
		}
		case ModListModelColumn::name:
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

	switch (static_cast<ModListModelColumn>(col))
	{
	case ModListModelColumn::priority:
	{
		wxBitmap icon;
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
	case ModListModelColumn::name:
	{
		variant = wxVariant(wxDataViewIconText(
			wxString::FromUTF8(mod.name), loadModIcon(_iconStorage, mod.data_path, mod.icon)));
		break;
	}
	case ModListModelColumn::author:
	{
		variant = wxVariant(wxString::FromUTF8(mod.author));
		break;
	}
	case ModListModelColumn::category:
	{
		variant = wxVariant(wxString::FromUTF8(wxGetApp().categoryTranslationString(mod.category)));
		break;
	}
	case ModListModelColumn::version:
	{
		variant = wxVariant(wxString::FromUTF8(mod.version));
		break;
	}
	case ModListModelColumn::directory:
	{
		variant = wxVariant(wxString::FromUTF8(mod.id));
		break;
	}
	case ModListModelColumn::checkbox:
	{
		variant = wxVariant(_checked.contains(id));
		break;
	}
	case ModListModelColumn::status:
	{
		variant = wxVariant(wxDataViewIconText(
			L"", _iconStorage.get(position ? embedded_icon::tick_green : embedded_icon::cross_gray)));
		break;
	}
	case ModListModelColumn::load_order:
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

	switch (static_cast<ModListModelColumn>(col))
	{
	case ModListModelColumn::checkbox:
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

	auto compareRest = [&](ModListModelColumn col) {
		return wxDataViewModel::Compare(item1, item2, static_cast<unsigned int>(col), ascending);
	};

	if (type1 != type2)
	{
		if (_managedMode == ModListModelManagedMode::as_flat_list)
			return type1 < type2 ? 1 : -1;

		return type1 < type2 ? -1 : 1;
	}

	if (type1 == ItemType::container)
		return static_cast<ssize_t>(index1) - static_cast<ssize_t>(index2);

	if (static_cast<ModListModelColumn>(column) == ModListModelColumn::priority)
	{
		const auto pos1 = _list.position(_displayed.items[index1]);
		const auto pos2 = _list.position(_displayed.items[index2]);

		if (!pos1 && !pos2)
			return wxDataViewModel::Compare(
				item1, item2, static_cast<unsigned int>(ModListModelColumn::name), true);

		if (pos1 && pos2)
			return ascending ? static_cast<ssize_t>(index1) - static_cast<ssize_t>(index2)
							 : static_cast<ssize_t>(index2) - static_cast<ssize_t>(index1);

		if (pos1)
			return -1;

		return 1;
	}

	if (static_cast<ModListModelColumn>(column) == ModListModelColumn::category)
	{
		const auto& cat1 = _modDataProvider.modData(_displayed.items[index1]).category;
		const auto& cat2 = _modDataProvider.modData(_displayed.items[index2]).category;

		if (cat1.empty() != cat2.empty())
			return ascending ? static_cast<ssize_t>(cat1.empty()) - static_cast<ssize_t>(cat2.empty())
							 : static_cast<ssize_t>(cat2.empty()) - static_cast<ssize_t>(cat1.empty());
	}

	if (auto res = wxDataViewModel::Compare(item1, item2, column, ascending); res != 0)
		return res;

	return compareRest(ModListModelColumn::name);
}

void ModListModel::modList(const ModList& mods)
{
	_list = mods;
	reload();
}

void ModListModel::setChecked(std::unordered_set<std::string> items)
{
	_checked = std::move(items);
	Cleared();
}

const std::unordered_set<std::string>& ModListModel::getChecked() const
{
	return _checked;
}

void ModListModel::applyFilter(const std::string& value)
{
	_filter = boost::locale::fold_case(value);
	reload();
}

void ModListModel::applyCategoryFilter(const std::set<std::string>& value)
{
	_categoryFilter = value;
	reload();
}

bool ModListModel::passFilter(const std::string& id) const
{
	// TODO: move into mod itself?

	if (_filter.empty() && _categoryFilter.empty())
		return true;

	const auto& mod  = _modDataProvider.modData(id);
	const auto& desc = _modDataProvider.description(mod.id);

	if (!_categoryFilter.empty() && !_categoryFilter.contains(mod.category))
	{
		return false;
	}

	if (!_filter.empty())
	{
		return std::ranges::any_of(
			std::initializer_list { mod.id, mod.name, mod.author, mod.category, mod.version, desc },
			[&](const std::string& from) {
				return boost::contains(boost::locale::fold_case(from), _filter);
			});
	}

	return true;
}

void ModListModel::reload()
{
	_displayed.categories.clear();
	_displayed.items.clear();

	for (const auto& mod : _list.data)
		if (passFilter(mod.id))
			_displayed.items.emplace_back(mod.id);

	for (const auto& mod : _list.rest)
		if (passFilter(mod))
			_displayed.items.emplace_back(mod);

	size_t                                  managedCount = 0;
	std::unordered_map<std::string, size_t> cats;
	size_t                                  archivedCount = 0;

	for (const auto& id : _displayed.items)
	{
		auto state = _list.state(id);

		if (state)
		{
			if (_managedMode == ModListModelManagedMode::as_group)
				++managedCount;
		}
		else
		{
			if (_archivedMode == ModListModelArchivedMode::as_single_group)
				++archivedCount;
			else if (_archivedMode == ModListModelArchivedMode::as_individual_groups)
				++cats[_modDataProvider.modData(id).category];
		}
	}

	if (managedCount)
	{
		_displayed.categories.emplace_back(ModListDsplayedData::ManagedGroupTag(),
			wxString::Format(L"%s (%d)", "Managed"_lng, managedCount));
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

void ModListModel::setManagedModsDisplay(ModListModelManagedMode value)
{
	_managedMode = value;
	reload();
}

void ModListModel::setArchivedModsDisplay(ModListModelArchivedMode value)
{
	_archivedMode = value;
	reload();
}
