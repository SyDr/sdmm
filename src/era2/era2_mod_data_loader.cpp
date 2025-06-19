// SD Mod Manager

// Copyright (c) 2020-2024 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "application.h"
#include "era2_mod_data_loader.hpp"
#include "system_info.hpp"
#include "utility/json_util.h"

#include "interface/ii18n_service.hpp"

#include <boost/locale.hpp>

using namespace mm;

namespace
{
	std::optional<std::string> get_string_from_json_object(const nlohmann::json& data, const std::string& key)
	{
		if (auto it = data.find(key); it != data.cend() && it->is_string())
			return it->get<std::string>();

		return {};
	}

	std::optional<std::string> get_i18n_string_from_json_object_impl(
		const nlohmann::json& section, const std::string& lng)
	{
		auto it = section.find(lng);
		if (it != section.end() && it->is_string())
			if (auto str = it->get<std::string>(); !str.empty())
				return str;

		return {};
	}

	std::optional<std::string> get_i18n_string_from_json_object_impl(const nlohmann::json& section,
		const std::string& lng, const std::string& legacyLngCode, bool& legacyUsed)
	{
		auto result = get_i18n_string_from_json_object_impl(section, lng);
		if (result)
			return result;

		if (legacyLngCode.empty())
			return result;

		result = get_i18n_string_from_json_object_impl(section, legacyLngCode);
		if (result)
			legacyUsed = true;

		return result;
	}

	std::optional<std::string> get_i18n_string_from_json_object(const nlohmann::json& data,
		const std::string& key, const std::string& preferredLng, const std::string& defaultLng,
		const II18nService& i18Service, bool& legacyUsed)
	{
		const auto section = data.find(key);
		if (section == data.cend() || !section->is_object())
			return {};

		auto result = get_i18n_string_from_json_object_impl(
			*section, preferredLng, i18Service.legacyCode(preferredLng), legacyUsed);
		if (result)
			return result;

		if (preferredLng == defaultLng)
			return {};

		return get_i18n_string_from_json_object_impl(
			*section, defaultLng, i18Service.legacyCode(defaultLng), legacyUsed);
	}

	std::set<std::string> get_string_set_from_json(const nlohmann::json& data)
	{
		std::set<std::string> result;

		for (const auto& item : data)
			if (item.is_string())
				result.emplace(item.get<std::string>());

		return result;
	}
}

ModData Era2ModDataLoader::load(const fs::path& loadFrom, const std::string& preferredLng,
	const std::set<std::string>& defaultIncompatible, const std::set<std::string>& defaultRequires,
	const std::set<std::string>& defaultLoadAfter, const II18nService& i18Service)
{
	bool hasRequires     = false;
	bool hasLoadAfter    = false;
	bool hasIncompatible = false;

	ModData result;
	result.data_path   = loadFrom;
	result.id          = loadFrom.filename().string();
	result.virtual_mod = !is_directory(loadFrom);

	auto supplyResultWithDefaults = [&] {
		if (result.name.empty())
			result.name = result.id;

		if (!hasRequires)
		{
			result.requires_ = defaultRequires;
			if (result.id != "WoG")
				result.requires_.emplace("WoG");
		}

		if (!hasLoadAfter)
		{
			result.load_after = result.requires_;
			result.load_after.insert(defaultLoadAfter.cbegin(), defaultLoadAfter.cend());
			if (result.id != "WoG")
				result.load_after.emplace("WoG");
		}

		if (!hasIncompatible)
			result.incompatible = defaultIncompatible;

		if (result.description.empty())
			result.description = "readme.txt";

		return result;
	};

	if (result.virtual_mod)
		return supplyResultWithDefaults();

	const auto path = loadFrom / mm::SystemInfo::ModInfoFilename;
	const auto data = loadJsonFromFile(path, true);

	if (!data.is_object())
		return supplyResultWithDefaults();

	result.version = get_string_from_json_object(data, "version").value_or("");
	if (result.version.empty())
	{
		result.version = get_string_from_json_object(data, "mod_version").value_or("");

		if (result.version.empty())
		{
			auto version = find_object_value(&data, "version");
			if (!version)
				version = &data;

			result.version = get_string_from_json_object(*version, "mod").value_or("");
		}

		if (!result.version.empty())
			result.legacy_format = true;
	}

	result.name = get_i18n_string_from_json_object(
		data, "name", preferredLng, mm::SystemInfo::DefaultLanguage, i18Service, result.legacy_format)
					  .value_or("");

	if (result.name.empty())
		result.name = get_i18n_string_from_json_object(
			data, "caption", preferredLng, mm::SystemInfo::DefaultLanguage, i18Service, result.legacy_format)
						  .value_or("");

	result.description = get_i18n_string_from_json_object(
		data, "description", preferredLng, mm::SystemInfo::DefaultLanguage, i18Service, result.legacy_format)
							 .value_or("");

	if (result.description.empty())
	{
		const auto description = data.find("description");
		if (description != data.cend() && description->is_object())
		{
			result.legacy_format = true;
			result.description   = get_i18n_string_from_json_object(*description, "full", preferredLng,
				  mm::SystemInfo::DefaultLanguage, i18Service, result.legacy_format)
									 .value_or("readme.txt");
		}
	}

	result.icon = get_string_from_json_object(data, "icon").value_or("");

	if (result.icon.empty())
	{
		if (const auto ico = find_object_value(&data, "icon"))
		{
			result.legacy_format = true;
			result.icon          = get_string_from_json_object(*ico, "file").value_or("");
		}
	}

	result.category = boost::locale::fold_case(get_string_from_json_object(data, "category").value_or(""));
	result.author   = get_string_from_json_object(data, "author").value_or("");
	result.homepage = get_string_from_json_object(data, "homepage").value_or("");

	if (const auto compat = data.find("compatibility"); compat != data.end())
	{
		if (const auto req = compat->find("requires"); req != compat->end() && req->is_array())
		{
			result.requires_  = get_string_set_from_json(*req);
			result.load_after = result.requires_;
			hasRequires       = true;
			hasLoadAfter      = true;
		}

		if (const auto after = compat->find("load_after"); after != compat->end() && after->is_array())
		{
			result.load_after.merge(get_string_set_from_json(*after));
			hasLoadAfter = true;
		}

		if (const auto inc = compat->find("incompatible"); inc != compat->end() && inc->is_array())
		{
			result.incompatible = get_string_set_from_json(*inc);
			hasIncompatible     = true;
		}
	}

	return supplyResultWithDefaults();
}
