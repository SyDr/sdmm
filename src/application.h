// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

class wxString;

namespace mm
{
	struct IAppConfig;
	struct II18nService;
	struct IIconStorage;
	struct IPlatformService;

	struct Application
	{
		virtual ~Application() = default;

		virtual IAppConfig&       appConfig() const       = 0;
		virtual II18nService&     i18nService() const     = 0;
		virtual IIconStorage&     iconStorage() const     = 0;
		virtual IPlatformService& platformService() const = 0;
	};
}

wxString operator"" _lng(const char* s, std::size_t);
