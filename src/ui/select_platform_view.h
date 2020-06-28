// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <memory>
#include <vector>

#include <wx/dialog.h>

class wxListView;
class wxStaticBox;

namespace mm
{
	class SelectPlatformView : public wxDialog
	{
	public:
		explicit SelectPlatformView(wxWindow *parent);

		std::string selectedPlatform() const;

	private:
		void prepareData();
		void createControls();
		void bindEvents();
		void buildLayout();
		void fillData();
		void selectionMade(const std::string& id);

	private:
		struct Platform
		{
			const std::string id;
			const std::string name;

			Platform(const std::string& id, const std::string& name)
				: id(id)
				, name(name)
			{}
		};

		std::vector<Platform> _platformList;
		std::string _selectedPlatform;

		wxStaticBox* _mainGroup = nullptr;
		wxListView* _listView = nullptr;
		wxButton* _buttonOk = nullptr;
	};
}
