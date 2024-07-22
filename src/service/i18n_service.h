// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "interface/ii18n_service.hpp"

#include <nlohmann/json.hpp>
#include <unordered_map>
#include <wx/string.h>

namespace mm
{
	struct IAppConfig;

	struct I18nService : II18nService
	{
		explicit I18nService(const IAppConfig& config);

		std::vector<std::string> available() const override;

		std::string category(const std::string& category) const override;
		std::string get(const std::string& key) const override;
		std::string languageName(const std::string& code) const override;

		std::string legacyCode(const std::string& code) const override;

	private:
		void build_cache(const nlohmann::json& data, const std::string& prefix);

	private:
		std::vector<std::pair<std::string, std::string>> _availableLanguages;

		std::map<std::string, std::string> _data;
	};
}
