// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <memory>
#include <deque>

namespace mm
{
	struct IModPlatform;
	class IPlatformDescriptor;

	class IPlatformService
	{
	public:
		virtual ~IPlatformService() = default;

		virtual std::deque<IPlatformDescriptor*> availablePlatforms() const = 0;

		virtual std::unique_ptr<IModPlatform> create(const wxString& platformId) const = 0;
	};
}
