// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "application.h"
#include "era2_mod_loader.hpp"
#include "system_info.hpp"
#include "utility/json_util.h"

using namespace mm;

namespace
{
	std::optional<std::string> get_string_from_json_object(const nlohmann::json& data, const std::string& key)
	{
		if (auto it = data.find(key); it != data.cend() && it->is_string())
			return it->get<std::string>();

		return {};
	}

	std::optional<std::string> get_i18n_string_from_json_object(const nlohmann::json& data,
		const std::string& key, const std::string& preferredLng, const std::string& defaultLng)
	{
		const auto section = data.find(key);
		if (section == data.cend() || !section->is_object())
			return {};

		auto it = section->find(preferredLng);
		if (it != section->end() && it->is_string())
			if (auto str = it->get<std::string>(); !str.empty())
				return str;

		if (preferredLng == defaultLng)
			return {};

		it = section->find(defaultLng);
		if (it != section->end() && it->is_string())
			if (auto str = it->get<std::string>(); !str.empty())
				return str;

		return {};
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

ModData era2_mod_loader::updateAvailability(const fs::path& loadFrom, const std::string& preferredLng,
	const std::set<std::string>& defaultIncompatible, const std::set<std::string>& defaultRequires,
	const std::set<std::string>& defaultLoadAfter)
{
	bool hasRequires     = false;
	bool hasLoadAfter    = false;
	bool hasIncompatible = false;

	ModData result;
	result.data_path   = loadFrom;
	result.id          = loadFrom.filename().wstring();
	result.virtual_mod = !is_directory(loadFrom);

	auto supplyResultWithDefaults = [&] {
		if (result.caption.empty())
			result.caption = result.id.ToStdString(wxConvUTF8);

		if (!hasRequires)
		{
			result.requires_ = defaultRequires;
			if (result.id != L"WoG")
				result.requires_.emplace("WoG");
		}

		if (!hasLoadAfter)
		{
			result.load_after = defaultLoadAfter;
			if (result.id != L"WoG")
				result.load_after.emplace("WoG");
		}

		if (!hasIncompatible)
			result.incompatible = defaultIncompatible;

		return result;
	};

	if (result.virtual_mod)
		return supplyResultWithDefaults();

	const auto path = loadFrom / mm::SystemInfo::ModInfoFilename;

	boost::nowide::ifstream datafile(path);

	if (!datafile)
		return supplyResultWithDefaults();

	nlohmann::json data;

	try
	{
		data = nlohmann::json::parse(datafile);
	}
	catch (const nlohmann::json::parse_error& e)
	{
		wxLogError(wxString::FromUTF8(e.what()));
		wxLogError(wxString::Format("Error while parsing file %s"_lng, wxString::FromUTF8(path.string())));
	}

	if (!data.is_object())
		return supplyResultWithDefaults();

	auto version = find_object_value(&data, "version");
	if (!version)
		version = &data;

	result.mod_platform = get_string_from_json_object(*version, "platform").value_or("");
	result.mod_version  = get_string_from_json_object(*version, "mod").value_or("");
	result.info_version = get_string_from_json_object(*version, "info").value_or("");

	result.caption =
		get_i18n_string_from_json_object(data, "caption", preferredLng, mm::SystemInfo::DefaultLanguage)
			.value_or("");

	const auto description = data.find("description");
	if (description != data.cend() && description->is_object())
	{
		result.full_description = get_i18n_string_from_json_object(
			*description, "full", preferredLng, mm::SystemInfo::DefaultLanguage)
									  .value_or("readme.txt");

		result.short_description = get_i18n_string_from_json_object(
			*description, "short", preferredLng, mm::SystemInfo::DefaultLanguage)
									   .value_or("");
	}

	if (const auto ico = find_object_value(&data, "icon"))
	{
		result.icon_filename = get_string_from_json_object(*ico, "file").value_or("");

		if (const auto index = find_int_value(ico, "index"))
			result.icon_index = index->get<size_t>();
	}

	result.category      = get_string_from_json_object(data, "category").value_or("");
	result.authors       = get_string_from_json_object(data, "author").value_or("");
	result.homepage_link = get_string_from_json_object(data, "homepage").value_or("");

	if (const auto compat = data.find("compatibility"); compat != data.end())
	{
		if (const auto req = compat->find("requires"); req != compat->end() && req->is_array())
		{
			result.requires_ = get_string_set_from_json(*req);
			hasRequires      = true;
		}

		if (const auto after = compat->find("load_after"); after != compat->end() && after->is_array())
		{
			result.load_after = get_string_set_from_json(*after);
			hasLoadAfter      = true;
		}

		if (const auto inc = compat->find("incompatible"); inc != compat->end() && inc->is_array())
		{
			result.incompatible = get_string_set_from_json(*inc);
			hasIncompatible     = true;
		}
	}

	return supplyResultWithDefaults();
}
