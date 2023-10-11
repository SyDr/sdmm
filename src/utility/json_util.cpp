// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "json_util.h"

#include "utility/sdlexcept.h"

const nlohmann::json* mm::find_value(const nlohmann::json* data, const std::string& key)
{
	const auto it = data->find(key);

	return (it != data->end()) ? &it.value() : nullptr;
}

const nlohmann::json* mm::find_int_value(const nlohmann::json* data, const std::string& key)
{
	const auto result = find_value(data, key);

	return result && result->is_number_integer() ? result : nullptr;
}

const nlohmann::json* mm::find_string_value(const nlohmann::json* data, const std::string& key)
{
	const auto result = find_value(data, key);

	return result && result->is_string() ? result : nullptr;
}

const nlohmann::json* mm::find_array_value(const nlohmann::json* data, const std::string& key)
{
	const auto result = find_value(data, key);

	return result && result->is_array() ? result : nullptr;
}

const nlohmann::json* mm::find_object_value(const nlohmann::json* data, const std::string& key)
{
	const auto result = find_value(data, key);

	return result && result->is_object() ? result : nullptr;
}

std::string mm::get_string_value(const nlohmann::json* data, const std::string& key, const std::string& default_ /* = */ )
{
	const auto result = find_string_value(data, key);

	return result ? result->get<std::string>() : default_;
}

std::optional<bool> mm::get_bool_value(const nlohmann::json* data, const std::string& key)
{
	const auto result = find_value(data, key);

	return result && result->is_boolean() ? result->get<bool>() : std::optional<bool>();
}

mm::i18n mm::get_i18n_value(const nlohmann::json* data, const std::vector<const char *>& lng_list)
{
	i18n result;

	for (const std::string lng : lng_list)
	{
		const nlohmann::json* value = data ? find_string_value(data, lng) : nullptr;

		if (data && value)
			result[lng] = value->get<std::string>();
		else
			result[lng] = std::string();
	}

	return result;
}

nlohmann::json mm::loadJsonFromFile(const std::filesystem::path& path)
{
	std::ifstream datafile(path.string());

	if (datafile)
	{
		std::stringstream stream;
		stream << datafile.rdbuf();
		datafile.close();

		return nlohmann::json::parse(stream);
	}

	return nlohmann::json();
}
