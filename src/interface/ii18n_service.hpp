// SD Mod Manager

// Copyright (c) 2020-2024 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <string>

namespace mm
{
	struct II18nService
	{
		virtual ~II18nService() = default;

		[[nodiscard]] virtual std::vector<std::string> available() const = 0;

		[[nodiscard]] virtual std::string category(const std::string& category) const = 0;
		[[nodiscard]] virtual std::string get(const std::string& key) const           = 0;
		[[nodiscard]] virtual std::string languageName(const std::string& code) const = 0;

		[[nodiscard]] virtual std::string legacyCode(const std::string& code) const = 0;
	};
}
