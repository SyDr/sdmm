// SD Mod Manager

// Copyright (c) 2020-2024 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <string>

namespace mm
{
	struct ModData;

	struct IModDataProvider
	{
		virtual ~IModDataProvider() = default;

		[[nodiscard]] virtual const ModData&     modData(const std::string& id)     = 0;
		[[nodiscard]] virtual const std::string& description(const std::string& id) = 0;
	};
}
