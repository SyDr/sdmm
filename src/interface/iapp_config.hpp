// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "type/filesystem.hpp"

#include <string>
#include <vector>

namespace mm
{
	struct MainWindowProperties;

	struct IAppConfig
	{
		virtual ~IAppConfig() = default;

		virtual bool     portableMode() const = 0;
		virtual fs::path dataPath() const     = 0;
		virtual fs::path programPath() const  = 0;

		virtual void save() = 0;

		virtual std::string currentLanguageCode() const                        = 0;
		virtual void        setCurrentLanguageCode(const std::string& lngCode) = 0;

		virtual std::string selectedPlatform() const                                = 0;
		virtual void        setSelectedPlatformCode(const std::string& newPlatform) = 0;

		virtual void                 setMainWindowProperties(const MainWindowProperties& props) = 0;
		virtual MainWindowProperties mainWindow() const                                         = 0;

		virtual fs::path getDataPath() const                  = 0;
		virtual void     setDataPath(const fs::path& path)    = 0;
		virtual void     forgetDataPath(const fs::path& path) = 0;

		virtual bool dataPathHasStar(const fs::path& path) const          = 0;
		virtual void starDataPath(const fs::path& path, bool star = true) = 0;
		virtual void unstarDataPath(const fs::path& path)                 = 0;

		virtual std::vector<fs::path> getKnownDataPathList() const = 0;
	};
}
