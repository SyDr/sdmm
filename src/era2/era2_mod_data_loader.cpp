// SD Mod Manager

// Copyright (c) 2020-2024 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "era2_mod_data_loader.hpp"
#include "system_info.hpp"
#include "utility/json_util.h"

#include "interface/ii18n_service.hpp"

#include <boost/locale.hpp>

#include <sstream>

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
		const std::string& lng, const std::vector<std::string>& legacyLngCodes, bool& legacyUsed)
	{
		if (auto result = get_i18n_string_from_json_object_impl(section, lng))
			return result;

		for (const auto& legacyLngCode : legacyLngCodes)
		{
			if (legacyLngCode.empty() || legacyLngCode == lng)
				continue;

			if (auto result = get_i18n_string_from_json_object_impl(section, legacyLngCode))
			{
				legacyUsed = true;
				return result;
			}
		}

		return {};
	}

	std::optional<std::string> get_i18n_string_from_json_object(const nlohmann::json& data,
		const std::string& key, const std::string& preferredLng, const std::string& defaultLng,
		const II18nService& i18Service, bool& legacyUsed)
	{
		const auto section = data.find(key);
		if (section == data.cend() || !section->is_object())
			return {};

		auto result = get_i18n_string_from_json_object_impl(
			*section, preferredLng, i18Service.legacyCodes(preferredLng), legacyUsed);
		if (result)
			return result;

		if (preferredLng == defaultLng)
			return {};

		return get_i18n_string_from_json_object_impl(
			*section, defaultLng, i18Service.legacyCodes(defaultLng), legacyUsed);
	}

	std::set<std::string> get_folded_string_set_from_json(const nlohmann::json& data)
	{
		std::set<std::string> result;

		for (const auto& item : data)
			if (item.is_string())
				result.emplace(boost::locale::fold_case(item.get<std::string>()));

		return result;
	}

	std::string get_i18n_or_plain_string(const nlohmann::json& data, const std::string& key,
		const std::string& preferredLng, const II18nService& i18Service, bool& legacyUsed)
	{
		if (auto plain = get_string_from_json_object(data, key))
			return *plain;

		return get_i18n_string_from_json_object(
			data, key, preferredLng, mm::SystemInfo::DefaultLanguage, i18Service, legacyUsed)
			.value_or("");
	}

	fs::path metadataPath(const std::string& value)
	{
		try
		{
			return fs::path(boost::nowide::widen(value));
		}
		catch (...)
		{
			return fs::path(value);
		}
	}

	using IniValues = std::unordered_map<std::string, std::string>;

	IniValues loadLegacyInfo(const fs::path& path)
	{
		boost::nowide::ifstream input(path);
		IniValues               values;
		if (!input)
			return values;

		std::string line;
		bool        infoSection = false;
		bool        firstLine   = true;
		while (std::getline(input, line))
		{
			if (!line.empty() && line.back() == '\r')
				line.pop_back();
			if (firstLine && line.starts_with("\xEF\xBB\xBF"))
				line.erase(0, 3);
			firstLine = false;

			auto trimmed = line;
			boost::trim(trimmed);
			if (trimmed.empty() || trimmed.starts_with(';') || trimmed.starts_with('#'))
				continue;

			if (trimmed.front() == '[' && trimmed.back() == ']')
			{
				auto section = trimmed.substr(1, trimmed.size() - 2);
				boost::trim(section);
				infoSection = boost::iequals(section, "info");
				continue;
			}

			if (!infoSection)
				continue;

			const auto separator = line.find('=');
			if (separator == std::string::npos)
				continue;

			auto key   = line.substr(0, separator);
			auto value = line.substr(separator + 1);
			boost::trim(key);
			boost::trim(value);
			boost::algorithm::to_lower(key);
			if (!key.empty())
				values[key] = value;
		}

		return values;
	}

	std::string iniValue(const IniValues& values, std::string key)
	{
		boost::algorithm::to_lower(key);
		const auto found = values.find(key);
		return found == values.end() ? std::string() : found->second;
	}

	std::string localizedIniValue(const IniValues& values, const std::string& key,
		const std::string& preferredLng, const II18nService& i18Service)
	{
		std::vector<std::string> languages;
		auto appendLanguage = [&languages](const std::string& language) {
			if (!language.empty() && std::ranges::find(languages, language) == languages.end())
				languages.emplace_back(language);
		};

		appendLanguage(preferredLng);
		for (const auto& legacy : i18Service.legacyCodes(preferredLng))
			appendLanguage(legacy);
		appendLanguage(SystemInfo::DefaultLanguage);
		for (const auto& legacy : i18Service.legacyCodes(SystemInfo::DefaultLanguage))
			appendLanguage(legacy);

		for (const auto& language : languages)
			if (auto value = iniValue(values, key + "." + language); !value.empty())
				return value;

		return iniValue(values, key);
	}

	bool loadLegacyMetadata(ModData& result, const fs::path& loadFrom, const std::string& preferredLng,
		const II18nService& i18Service)
	{
		const auto values = loadLegacyInfo(loadFrom / "mod_info.ini");
		if (values.empty())
			return false;

		result.legacy_format = true;
		result.name          = localizedIniValue(values, "Caption", preferredLng, i18Service);
		result.description = metadataPath(
			localizedIniValue(values, "Description File", preferredLng, i18Service));
		result.version     = iniValue(values, "Version");
		result.author      = iniValue(values, "Author");
		result.homepage    = iniValue(values, "Homepage");
		result.icon        = iniValue(values, "Icon File");
		result.category    = boost::locale::fold_case(iniValue(values, "Category"));

		if (const auto priority = iniValue(values, "Priority"); !priority.empty())
		{
			try
			{
				std::size_t parsed = 0;
				const auto  value  = std::stoi(priority, &parsed);
				if (parsed == priority.size())
					result.priority = value;
			}
			catch (...)
			{}
		}

		return true;
	}
}

ModData Era2ModDataLoader::load(const std::string& id, const fs::path& loadFrom,
	const std::string& preferredLng, const std::set<std::string>& defaultIncompatible,
	const std::set<std::string>& defaultRequires, const std::set<std::string>& defaultLoadAfter,
	const II18nService& i18Service)
{
	bool hasRequires     = false;
	bool hasLoadAfter    = false;
	bool hasIncompatible = false;

	ModData result;
	result.data_path   = loadFrom;
	result.id          = id;
	result.dir         = loadFrom.filename().string();
	result.virtual_mod = !is_directory(loadFrom);

	auto supplyResultWithDefaults = [&] {
		if (result.name.empty())
			result.name = result.dir;

		if (!hasRequires)
		{
			result.requires_ = defaultRequires;
			if (result.id != "wog")
				result.requires_.emplace("wog");
		}

		if (!hasLoadAfter)
		{
			result.load_after = result.requires_;
			result.load_after.insert(defaultLoadAfter.cbegin(), defaultLoadAfter.cend());
			if (result.id != "wog")
				result.load_after.emplace("wog");
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
	{
		loadLegacyMetadata(result, loadFrom, preferredLng, i18Service);
		return supplyResultWithDefaults();
	}

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

	result.description = metadataPath(get_i18n_string_from_json_object(
		data, "description", preferredLng, mm::SystemInfo::DefaultLanguage, i18Service, result.legacy_format)
							 .value_or(""));

	if (result.description.empty())
	{
		const auto description = data.find("description");
		if (description != data.cend() && description->is_object())
		{
			result.legacy_format = true;
			result.description   = metadataPath(get_i18n_string_from_json_object(*description, "full", preferredLng,
				  mm::SystemInfo::DefaultLanguage, i18Service, result.legacy_format)
									 .value_or("readme.txt"));
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

	if (auto s = get_string_from_json_object(data, "support"); s.has_value() && !s.value().empty())
	{
		result.support.emplace_back(s.value());
	}
	else if (auto v = data.find("support"); v != data.end() && v.value().is_array())
	{
		for (const auto& item : v.value())
			if (item.is_string() && !item.get<std::string>().empty())
				result.support.emplace_back(item.get<std::string>());
	}

	if (const auto& priority = data.find("priority"); priority != data.end() && priority->is_number_integer())
		result.priority = priority->get<int>();

	if (const auto plugins = data.find("plugins"); plugins != data.end() && plugins->is_object())
	{
		for (const auto& [filename, metadata] : plugins->items())
		{
			if (filename.empty() || !metadata.is_object())
				continue;

			PluginMetadata item;
			item.caption =
				get_i18n_or_plain_string(metadata, "caption", preferredLng, i18Service, result.legacy_format);
			item.description = get_i18n_or_plain_string(
				metadata, "description", preferredLng, i18Service, result.legacy_format);

			if (item.caption.empty())
				item.caption = filename;
			if (item.description.empty())
				item.description = i18Service.get("message/status/no_description_available");

			result.plugins.emplace(filename, std::move(item));
		}
	}

	if (const auto compat = data.find("compatibility"); compat != data.end())
	{
		if (const auto req = compat->find("requires"); req != compat->end() && req->is_array())
		{
			result.requires_  = get_folded_string_set_from_json(*req);
			result.load_after = result.requires_;
			hasRequires       = true;
			hasLoadAfter      = true;
		}

		if (const auto after = compat->find("load_after"); after != compat->end() && after->is_array())
		{
			result.load_after.merge(get_folded_string_set_from_json(*after));
			hasLoadAfter = true;
		}

		if (const auto inc = compat->find("incompatible"); inc != compat->end() && inc->is_array())
		{
			result.incompatible = get_folded_string_set_from_json(*inc);
			hasIncompatible     = true;
		}
	}

	return supplyResultWithDefaults();
}
