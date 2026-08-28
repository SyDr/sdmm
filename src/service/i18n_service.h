// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "interface/ii18n_service.hpp"

#include "type/filesystem.hpp"

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
		std::string column(const std::string& key) const override;

		std::string languageName(const std::string& code) const override;
		std::vector<std::string> legacyCodes(const std::string& code) const override;

	private:
		struct LngMetadata
		{
			std::string              name;
			std::string              author;
			std::vector<std::string> legacyCodes;
		};

		using AvailableLngMetadata = std::unordered_map<std::string, LngMetadata>;

		AvailableLngMetadata loadAvailableLngMetadata(const fs::path& path) const;
		void                 build_cache(const nlohmann::json& data, const std::string& prefix);

	private:
		std::unordered_map<std::string, LngMetadata> _availableLanguages;

		std::map<std::string, std::string> _data;
	};
}
