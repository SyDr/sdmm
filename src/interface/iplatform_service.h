// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <deque>
#include <memory>

namespace mm
{
	struct IModPlatform;
	class IPlatformDescriptor;

	struct IPlatformService
	{
		virtual ~IPlatformService() = default;

		[[nodiscard]] virtual std::deque<IPlatformDescriptor*> availablePlatforms() const = 0;

		[[nodiscard]] virtual std::unique_ptr<IModPlatform> create(const wxString& platformId) const = 0;
	};
}
