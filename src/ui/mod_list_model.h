// SD Mod Manager

// Copyright (c) 2020-2024 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <deque>
#include <memory>
#include <unordered_set>

#include <wx/dataview.h>

#include "domain/mod_list.hpp"
#include "type/mod_list_model_structs.hpp"
#include "utility/wx_widgets_ptr.hpp"

namespace mm
{
	struct IIconStorage;
	struct IModManager;
	struct IModDataProvider;
	struct ModData;

	class ModListModel : public wxDataViewModel
	{
	public:
		explicit ModListModel(IModDataProvider& modDataProvider, IIconStorage& iconStorage,
			ModListModelManagedMode  managedMode  = ModListModelManagedMode::as_flat_list,
			ModListModelArchivedMode archivedMode = ModListModelArchivedMode::as_single_group);

		bool IsListModel() const override;

		unsigned int GetColumnCount() const override;
		wxString     GetColumnType(unsigned int col) const override;

		bool IsContainer(const wxDataViewItem& item) const override;
		bool HasContainerColumns(const wxDataViewItem& item) const override;

		wxDataViewItem GetParent(const wxDataViewItem& item) const override;
		unsigned int   GetChildren(const wxDataViewItem& item, wxDataViewItemArray& children) const override;
		void GetValue(wxVariant& variant, const wxDataViewItem& item, unsigned int col) const override;
		bool SetValue(const wxVariant& variant, const wxDataViewItem& item, unsigned int col) override;
		bool GetAttr(const wxDataViewItem& item, unsigned int col, wxDataViewItemAttr& attr) const override;

		int Compare(const wxDataViewItem& item1, const wxDataViewItem& item2, unsigned int column,
			bool ascending) const override;

		void modList(const ModList& mods);

		void                                   setChecked(std::unordered_set<std::string> items);
		const std::unordered_set<std::string>& getChecked() const;

		void applyFilter(const std::string& value);
		void applyCategoryFilter(const std::set<std::string>& value);

		const ModData*                                   findMod(const wxDataViewItem& item) const;
		wxDataViewItem                                   findItemById(const std::string& id) const;
		std::string                                      findIdByItem(const wxDataViewItem& item) const;
		std::optional<ModListDsplayedData::GroupItemsBy> itemGroupByItem(const wxDataViewItem& item) const;

		void setManagedModsDisplay(ModListModelManagedMode value);
		void setArchivedModsDisplay(ModListModelArchivedMode value);

		wxString status() const;

	private:
		bool passFilter(const std::string& id) const;

		void reload();

	private:
		ModListModelManagedMode  _managedMode  = ModListModelManagedMode::as_flat_list;
		ModListModelArchivedMode _archivedMode = ModListModelArchivedMode::as_single_group;

		ModList             _list;
		ModListDsplayedData _displayed;

		std::string           _filter;
		std::set<std::string> _categoryFilter;

		std::unordered_set<std::string> _checked;

		IModDataProvider& _modDataProvider;
		IIconStorage&     _iconStorage;
	};
}
