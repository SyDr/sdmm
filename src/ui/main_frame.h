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
	struct IIconStorage;

	class MainFrame : public wxFrame
	{
	public:
		explicit MainFrame(Application& app);

		void reloadModelIfNeeded();

	private:
		void createMenuBar();
		void reloadModel();

		void OnMenuItemSelected(wxCommandEvent& event);
		void OnAbout();
		void OnMenuToolsChangeDirectory();
		void OnMenuToolsReloadDataFromDisk();
		void OnMenuToolsLanguageSelected(const std::string& value);
		void OnMenuCheckForUpdates();
		void OnMenuToolsListModFiles();
		void OnMenuToolsChooseConflictResolveMode();
		void OnCloseWindow(wxCloseEvent& event);

		void saveWindowProperties();

		void selectExeToLaunch();
		void updateExecutableRelatedData();
		void onLaunchGameRequested();

	private:
		Application&                  _app;
		std::unique_ptr<IIconStorage> _iconStorage;

		std::unique_ptr<IModPlatform> _currentPlatform;

		wxWidgetsPtr<wxMenuBar>              _mainMenu = nullptr;
		std::map<int, std::function<void()>> _menuItems;  // itemId, handler

		wxWidgetsPtr<wxMenuItem>  _launchMenuItem = nullptr;
		wxWidgetsPtr<wxStatusBar> _statusBar      = nullptr;
	};
}