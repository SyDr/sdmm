// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <deque>
#include <memory>
#include <vector>

#include <wigwag/token_pool.hpp>
#include <wx/frame.h>

#include "utility/wx_widgets_ptr.hpp"

namespace mm
{
	class Application;
	struct IModPlatform;
	class IPanelView;
	class ModListView;

	class MainFrame : public wxFrame
	{
	public:
		explicit MainFrame(Application& app);

	private:
		void createMenuBar();
		void reloadModel();

		void OnMenuItemSelected(wxCommandEvent& event);
		void OnAbout();
		void OnMenuToolsChangeDirectory();
		void OnMenuManageProfilesRequested(bool saveAs);
		void OnMenuChangePlatform();
		void OnMenuToolsLanguageSelected(const wxString& value);
		void OnMenuCheckForUpdates();
		void OnMenuToolsListModFiles();
		void OnMenuToolsSortMods();
		void OnCloseWindow(wxCloseEvent& event);

		void saveWindowProperties();

	private:
		Application& _app;

		std::unique_ptr<IModPlatform> _currentPlatform;
		wxWidgetsPtr<ModListView> _modListView;

		wxWidgetsPtr<wxMenuBar> _mainMenu = nullptr;
		std::map<int, std::function<void()>> _menuItems; // itemId, handler

		wigwag::token_pool _tokenPool;
	};
}