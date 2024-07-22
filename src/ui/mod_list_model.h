// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <deque>
#include <memory>
#include <unordered_set>

#include <wx/dataview.h>

#include "domain/mod_list.hpp"
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
		explicit ModListModel(IModDataProvider& modDataProvider, IIconStorage& iconStorage, bool showHidden);

		enum class Column
		{
			priority,
			name,
			author,
			category,
			version,
			checkbox,
			activity,
			load_order,

			total,
		};

		unsigned int GetColumnCount() const override;
		wxString     GetColumnType(unsigned int col) const override;

		bool           IsContainer(const wxDataViewItem& item) const override;
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

		const ModData* findMod(const wxDataViewItem& item) const;
		wxDataViewItem findItemById(const std::string& id) const;
		std::string    findIdByItem(wxDataViewItem const& item) const;

	private:
		void reload();

	private:
		ModList                         _list;
		std::vector<std::string>        _displayedItems;
		std::unordered_set<std::string> _checked;

		IModDataProvider& _modDataProvider;
		IIconStorage&     _iconStorage;

		bool _showHidden   = false;
		bool _showInactive = true;
	};
}
