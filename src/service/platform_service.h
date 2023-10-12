// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "interface/iplatform_service.hpp"

namespace mm
{
	struct Application;

	class PlatformService : public IPlatformService
	{
	public:
		explicit PlatformService(const Application& app);

		[[nodiscard]] std::deque<IPlatformDescriptor*> availablePlatforms() const override;
		[[nodiscard]] std::unique_ptr<IModPlatform>    create(const std::string& platformId) const override;

	private:
		std::deque<std::unique_ptr<IPlatformDescriptor>> _platforms;
	};
}
