// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <memory>
#include <string>

class wxIcon;

namespace mm
{
	struct IModPlatform;

	class IPlatformDescriptor
	{
	public:
		virtual ~IPlatformDescriptor() = default;

		[[nodiscard]] virtual std::string getId() const           = 0;
		[[nodiscard]] virtual std::string getPlatformName() const = 0;

		[[nodiscard]] virtual wxIcon getIcon() const = 0;

		[[nodiscard]] virtual std::unique_ptr<IModPlatform> create() const = 0;
	};
}
