// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "types/filesystem.hpp"

#include <filesystem>
#include <vector>

class wxString;

namespace mm
{
	struct MainWindowProperties;

	struct IAppConfig
	{
		virtual ~IAppConfig() = default;

		virtual bool   portableMode() const = 0;
		virtual fspath dataPath() const     = 0;
		virtual fspath programPath() const  = 0;

		virtual void save() = 0;

		virtual auto currentLanguageCode() const -> std::string      = 0;
		virtual void setCurrentLanguageCode(const wxString& lngCode) = 0;

		virtual wxString selectedPlatform() const                             = 0;
		virtual void     setSelectedPlatformCode(const wxString& newPlatform) = 0;

		virtual void                 setMainWindowProperties(const MainWindowProperties& props) = 0;
		virtual MainWindowProperties mainWindow() const                                         = 0;

		virtual fspath getDataPath() const                = 0;
		virtual void   setDataPath(const fspath& path)    = 0;
		virtual void   forgetDataPath(const fspath& path) = 0;

		virtual bool dataPathHasStar(const fspath& path) const          = 0;
		virtual void starDataPath(const fspath& path, bool star = true) = 0;
		virtual void unstarDataPath(const fspath& path)                 = 0;

		virtual std::vector<fspath> getKnownDataPathList() const = 0;
	};
}
