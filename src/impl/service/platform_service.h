// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "interface/service/iplatform_service.h"

namespace mm
{
	class Application;

	class PlatformService : public IPlatformService
	{
	public:
		explicit PlatformService(const Application& app);

		std::deque<IPlatformDescriptor*> availablePlatforms() const override;
		std::unique_ptr<IModPlatform> create(const wxString& platformId) const override;

	private:
		std::deque<std::unique_ptr<IPlatformDescriptor>> _platforms;
	};
}
