// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <string>
#include <memory>

class wxString;
class wxIcon;

namespace mm
{
	struct IModPlatform;

	class IPlatformDescriptor
	{
	public:
		virtual ~IPlatformDescriptor() = default;

		virtual wxString getId() const = 0;
		virtual wxString getPlatformName() const = 0;

		virtual wxIcon getIcon() const = 0;

		virtual std::unique_ptr<IModPlatform> create() const = 0;
	};
}
