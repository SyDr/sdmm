// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <nlohmann/json.hpp>

#include <string>
#include <unordered_map>
#include <filesystem>
#include <optional>

class wxString;

namespace mm
{
	using i18n = std::unordered_map<std::string, std::string>;

	const nlohmann::json* find_value(const nlohmann::json* data, const std::string& key);

	const nlohmann::json* find_array_value(const nlohmann::json* data, const std::string& key);
	const nlohmann::json* find_int_value(const nlohmann::json* data, const std::string& key);
	const nlohmann::json* find_object_value(const nlohmann::json* data, const std::string& key);
	const nlohmann::json* find_string_value(const nlohmann::json* data, const std::string& key);

	std::string get_string_value(const nlohmann::json* data, const std::string& key, const std::string& default_ = {});
	std::optional<bool> get_bool_value(const nlohmann::json* data, const std::string& key);
	i18n get_i18n_value(const nlohmann::json* data, const std::vector<const char*>& lng_list);

	nlohmann::json loadJsonFromFile(const std::filesystem::path& path);
}
