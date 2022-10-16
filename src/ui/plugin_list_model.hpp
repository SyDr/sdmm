// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <deque>
#include <memory>
#include <unordered_set>

#include <wx/dataview.h>

#include "domain/plugin_list.hpp"
#include "utility/wx_widgets_ptr.hpp"

namespace mm
{
	class IIconStorage;
	struct IModManager;
	struct IModDataProvider;
	struct ModData;

	class PluginListModel : public wxDataViewIndexListModel
	{
	public:
		explicit PluginListModel(IModDataProvider& modDataProvider, IIconStorage& iconStorage);

		enum class Column
		{
			state,
			caption,
			mod,

			total,
		};

		unsigned int GetColumnCount() const override;
		wxString     GetColumnType(unsigned int col) const override;

		void GetValueByRow(wxVariant& variant, unsigned row, unsigned col) const override;
		bool SetValueByRow(const wxVariant& variant, unsigned row, unsigned col) override;
		bool GetAttrByRow(unsigned row, unsigned col, wxDataViewItemAttr& attr) const override;

		int Compare(const wxDataViewItem& item1, const wxDataViewItem& item2, unsigned int column,
					bool ascending) const override;

		void setList(PluginList const& items);

		wxString       findIdByItem(const wxDataViewItem& item) const;
		wxDataViewItem findItemById(const wxString& plugin) const;

	private:
		void reload();

	private:
		IModDataProvider& _modDataProvider;
		IIconStorage&     _iconStorage;
		PluginList        _items;

		wxString              _selected;
		std::vector<wxString> _displayedItems;

		bool _showHidden = false;
	};
}
