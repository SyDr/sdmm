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
	class IIconStorage;
	struct IModManager;
	struct IModDataProvider;
	struct ModData;

	class ModListModel : public wxDataViewIndexListModel
	{
	public:
		explicit ModListModel(IModDataProvider& modDataProvider, IIconStorage& iconStorage,
							  bool showHidden);

		enum class Column
		{
			priority,
			caption,
			author,
			category,
			version,
			checkbox,

			total,
		};

		unsigned int GetColumnCount() const override;
		wxString     GetColumnType(unsigned int col) const override;

		void GetValueByRow(wxVariant& variant, unsigned row, unsigned col) const override;
		bool SetValueByRow(const wxVariant& variant, unsigned row, unsigned col) override;
		bool GetAttrByRow(unsigned row, unsigned col, wxDataViewItemAttr& attr) const override;

		int Compare(const wxDataViewItem& item1, const wxDataViewItem& item2, unsigned int column,
					bool ascending) const override;

		void                                setModList(ModList const& mods);
		void                                setChecked(std::unordered_set<wxString> items);
		std::unordered_set<wxString> const& getChecked() const;

		void showHidden(bool show);
		void showInactive(bool show);

		const ModData* findMod(const wxDataViewItem& item) const;
		wxDataViewItem findItemById(const wxString& id) const;
		wxString       findIdByItem(wxDataViewItem const& item) const;

	private:
		void reload();

	private:
		ModList                      _list;
		std::vector<wxString>        _displayedItems;
		std::unordered_set<wxString> _checked;

		IModDataProvider& _modDataProvider;
		IIconStorage&     _iconStorage;

		bool _showHidden   = false;
		bool _showInactive = true;
	};
}
