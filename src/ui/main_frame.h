// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <deque>
#include <memory>
#include <vector>

#include <wx/frame.h>

#include "utility/wx_widgets_ptr.hpp"


namespace mm
{
	struct Application;
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
		void OnMenuToolsReloadDataFromDisk();
		void OnMenuChangePlatform();
		void OnMenuToolsLanguageSelected(const wxString& value);
		void OnMenuCheckForUpdates();
		void OnMenuToolsListModFiles();
		void OnMenuToolsSortMods();
		void OnMenuToolsChooseConflictResolveMode();
		void OnCloseWindow(wxCloseEvent& event);

		void saveWindowProperties();

		void selectExeToLaunch();
		void updateExecutableIcon();
		void onLaunchGameRequested();

	private:
		Application& _app;

		std::unique_ptr<IModPlatform> _currentPlatform;

		wxWidgetsPtr<wxMenuBar>              _mainMenu = nullptr;
		std::map<int, std::function<void()>> _menuItems;  // itemId, handler

		wxWidgetsPtr<wxButton> _launchButton       = nullptr;
		wxWidgetsPtr<wxButton> _launchManageButton = nullptr;
	};
}