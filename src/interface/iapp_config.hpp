// SD Mod Manager

// Copyright (c) 2020-2025 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "type/chrono.hpp"
#include "type/filesystem.hpp"

#include <string>
#include <vector>

namespace mm
{
	struct MainWindowProperties;
	enum class UpdateCheckMode;
	enum class ModDescriptionUsedControl;
	enum class InterfaceSize;
	enum class InterfaceLabel;

	struct IAppConfig
	{
		virtual ~IAppConfig() = default;

		[[nodiscard]] virtual bool     portableMode() const = 0;
		[[nodiscard]] virtual fs::path dataPath() const     = 0;
		[[nodiscard]] virtual fs::path programPath() const  = 0;

		virtual void save() = 0;

		[[nodiscard]] virtual std::string currentLanguageCode() const                        = 0;
		virtual void                      setCurrentLanguageCode(const std::string& lngCode) = 0;

		[[nodiscard]] virtual std::string selectedPlatform() const                                = 0;
		virtual void                      setSelectedPlatformCode(const std::string& newPlatform) = 0;

		virtual void setMainWindowProperties(const MainWindowProperties& props) = 0;
		[[nodiscard]] virtual MainWindowProperties mainWindow() const           = 0;

		[[nodiscard]] virtual fs::path getDataPath() const                  = 0;
		virtual void                   setDataPath(const fs::path& path)    = 0;
		virtual void                   forgetDataPath(const fs::path& path) = 0;

		[[nodiscard]] virtual bool dataPathHasStar(const fs::path& path) const          = 0;
		virtual void               starDataPath(const fs::path& path, bool star = true) = 0;
		virtual void               unstarDataPath(const fs::path& path)                 = 0;

		[[nodiscard]] virtual std::vector<fs::path> getKnownDataPathList() const = 0;

		[[nodiscard]] virtual UpdateCheckMode updateCheckMode() const                = 0;
		virtual void                          updateCheckMode(UpdateCheckMode value) = 0;

		[[nodiscard]] virtual time_point lastUpdateCheck() const           = 0;
		virtual void                     lastUpdateCheck(time_point value) = 0;

		// TODO: maybe
		//		[[nodiscard]] virtual std::string lastUpdateTag() const                    = 0;
		//		virtual void                      lastUpdateTag(const std::string& value)  = 0;

		[[nodiscard]] virtual ModDescriptionUsedControl modDescriptionUsedControl() const = 0;
		virtual bool modDescriptionUsedControl(ModDescriptionUsedControl value)           = 0;

		[[nodiscard]] virtual InterfaceSize interfaceSize() const              = 0;
		virtual bool                        interfaceSize(InterfaceSize value) = 0;

		[[nodiscard]] virtual InterfaceLabel interfaceLabel() const               = 0;
		virtual bool                         interfaceLabel(InterfaceLabel value) = 0;
	};
}
