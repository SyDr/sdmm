// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <deque>
#include <memory>
#include <vector>

#include <wx/frame.h>

#include "type/filesystem.hpp"
#include "utility/wx_widgets_ptr.hpp"

class wxInfoBarGeneric;
class wxDropFilesEvent;
class wxNotebook;

namespace mm
{
	struct Application;
	struct IModPlatform;
	struct IIconStorage;
	class ModListView;

	class MainFrame : public wxFrame
	{
	public:
		explicit MainFrame(Application& app);
		~MainFrame() override;

		void reloadModelIfNeeded();
		void updateCheckCompleted(const nlohmann::json& value, bool automatic);

	private:
		void createMenuBar();
		void reloadModel();

		void OnMenuItemSelected(wxCommandEvent& event);
		void OnAbout();
		void OnMenuToolsChangeDirectory();
		void OnMenuToolsReloadDataFromDisk();
		void OnMenuToolsChangeSettings();
		void OnMenuToolsLanguageSelected(const std::string& value);
		void OnMenuCheckForUpdates();
		void OnMenuModAddNewMods();
		void OnMenuModListModFiles();
		void OnMenuModOpenModFolder();
		void OnMenuModCreateNewMod();
		void OnCloseWindow(wxCloseEvent& event);
		void OnFilesDropped(wxDropFilesEvent& event);
		void importModPackages(const std::vector<fs::path>& packages);

		void saveWindowProperties();

		void selectExeToLaunch();
		void updateExecutableRelatedData();
		void onLaunchGameRequested();

	private:
		Application&                  _app;
		std::unique_ptr<IIconStorage> _iconStorage;

		std::unique_ptr<IModPlatform> _currentPlatform;
		wxWidgetsPtr<ModListView>     _modListView = nullptr;
		wxWidgetsPtr<wxNotebook>      _mainPages   = nullptr;

		wxWidgetsPtr<wxInfoBarGeneric> _infoBar = nullptr;
		wxTimer                        _infoBarTimer;
		wxString                       _newVersionUrl;

		wxWidgetsPtr<wxMenuBar>              _mainMenu = nullptr;
		std::map<int, std::function<void()>> _menuItems;  // itemId, handler

		wxWidgetsPtr<wxMenuItem>  _launchMenuItem = nullptr;
		wxWidgetsPtr<wxStatusBar> _statusBar      = nullptr;
	};
}
