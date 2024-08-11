// SD Mod Manager

// Copyright (c) 2020-2024 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <deque>
#include <memory>
#include <unordered_set>

#include <wx/dataview.h>

#include "domain/mod_list.hpp"
#include "utility/wx_widgets_ptr.hpp"
#include "mod_list_model_mode.hpp"

namespace mm
{
	struct IIconStorage;
	struct IModManager;
	struct IModDataProvider;
	struct ModData;

	class ModListModel : public wxDataViewModel
	{
	public:
		enum class Column
		{
			status,
			priority,
			name,
			author,
			category,
			version,
			checkbox,
			load_order,

			total,
		};

		explicit ModListModel(IModDataProvider& modDataProvider, IIconStorage& iconStorage, bool showHidden,
			ModListModelMode mode = ModListModelMode::flat);

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

		void                                   setModList(ModList const& mods);
		void                                   setChecked(std::unordered_set<std::string> items);
		std::unordered_set<std::string> const& getChecked() const;

		void showHidden(bool show);
		void showInactive(bool show);

		const ModData*             findMod(const wxDataViewItem& item) const;
		wxDataViewItem             findItemById(const std::string& id) const;
		std::string                findIdByItem(const wxDataViewItem& item) const;
		std::optional<std::string> categoryByItem(const wxDataViewItem& item) const;

		struct DisplayedData
		{
			std::vector<std::string> items;

			// std::string -> category as in info file or empty string
			// std::monostate -> Active group
			using category_type    = std::variant<std::string, std::monostate>;
			using cat_plus_display = std::pair<category_type, wxString>;

			std::vector<cat_plus_display> categories;
		};

	private:
		void reload();

	private:
		const ModListModelMode _mode = ModListModelMode::flat;

		ModList       _list;
		DisplayedData _displayed;

		std::unordered_set<std::string> _checked;

		IModDataProvider& _modDataProvider;
		IIconStorage&     _iconStorage;

		bool _showHidden   = false;
		bool _showInactive = true;
	};
}
