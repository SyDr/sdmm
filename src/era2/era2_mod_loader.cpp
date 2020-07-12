// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "application.h"
#include "era2_mod_loader.hpp"
#include "system_info.hpp"
#include "utility/json_util.h"

#include <filesystem>

using namespace mm;

namespace
{
	ModData supplyWithDefaults(ModData what, std::set<wxString> const& defaultIncompatible,
							   std::set<wxString> const& defaultRequires,
							   std::set<wxString> const& defaultLoadAfter, bool useIncompatbile,
							   bool useRequires, bool useLoadAfter)
	{
		if (!useRequires)
		{
			what.requires = defaultRequires;
			if (what.id != "WoG")
				what.requires.emplace("WoG");
		}

		if (!useLoadAfter)
		{
			what.load_after = defaultLoadAfter;
			if (what.id != "WoG")
				what.load_after.emplace("WoG");
		}

		if (!useIncompatbile)
		{
			what.incompatible = defaultIncompatible;
		}

		return what;
	}
}

ModData mm::era2_mod_loader::load(std::filesystem::path const& loadFrom, wxString const& preferredLng,
								  std::set<wxString> const& defaultIncompatible,
								  std::set<wxString> const& defaultRequires,
								  std::set<wxString> const& defaultLoadAfter)
{
	bool hasRequires     = false;
	bool hasLoadAfter    = false;
	bool hasIncompatible = false;

	ModData result;
	result.data_path   = loadFrom;
	result.id          = wxString::FromUTF8(loadFrom.filename().u8string());
	result.virtual_mod = !std::filesystem::is_directory(loadFrom);

	auto supplyResultWithDefaults = [&] {
		return supplyWithDefaults(std::move(result), defaultIncompatible, defaultRequires, defaultLoadAfter,
								  hasIncompatible, hasRequires, hasLoadAfter);
	};

	if (result.virtual_mod)
	{
		result.caption = result.id;
		return supplyResultWithDefaults();
	}

	auto const path = loadFrom / mm::constant::mod_info_filename;

	std::ifstream datafile(path);

	if (!datafile)
	{
		result.caption = result.id;
		return supplyResultWithDefaults();
	}

	nlohmann::json data;

	try
	{
		data = nlohmann::json::parse(datafile);
	}
	catch (nlohmann::json::parse_error const& e)
	{
		wxLogError(e.what());
		wxLogError(wxString::Format("Error while parsing file %s"_lng, path.u8string()));
	}

	if (!data.is_object())
	{
		result.caption = result.id;
		return supplyResultWithDefaults();
	}

	if (const auto ver = find_object_value(&data, "version"))
	{
		result.mod_platform = get_string_value(ver, "platform");
		result.mod_version  = get_string_value(ver, "mod");
		result.info_version = get_string_value(ver, "info");
	}
	else
	{
		result.mod_platform = get_string_value(&data, "platform");
		result.mod_version  = get_string_value(&data, "mod_version");
		result.info_version = get_string_value(&data, "info_version");
	}

	[&] {  // load caption
		auto const it = data.find("caption");
		if (it == data.end() || !it->is_object())
		{
			result.caption = result.id;
			return;
		}

		if (auto const preferredIt = it->find(preferredLng);
			preferredIt != it->end() && preferredIt->is_string())
		{
			result.caption = wxString::FromUTF8(preferredIt->get<std::string>());
			if (!result.caption.empty())
				return;
		}

		if (preferredLng != mm::constant::default_language)
		{
			if (auto const defaultIt = it->find(mm::constant::default_language);
				defaultIt != it->end() && defaultIt->is_string())
			{
				result.caption = wxString::FromUTF8(defaultIt->get<std::string>());
				if (!result.caption.empty())
					return;
			}
		}

		result.caption = result.id;
	}();

	[&] {  // load description
		auto const it = data.find("description");
		if (it == data.end() || !it->is_object())
			return;

		[&] {  // load full
			if (auto const fullIt = it->find("full"); fullIt != it->end() && fullIt->is_object())
			{
				if (auto const preferredIt = fullIt->find(preferredLng);
					preferredIt != fullIt->end() && preferredIt->is_string())
				{
					result.full_description =
						wxString::FromUTF8(preferredIt->get<std::string>()).ToStdWstring();
					if (!result.full_description.empty())
						return;
				}

				if (preferredLng != mm::constant::default_language)
				{
					if (auto const defaultIt = fullIt->find(mm::constant::default_language);
						defaultIt != fullIt->end() && defaultIt->is_string())
					{
						result.full_description =
							wxString::FromUTF8(defaultIt->get<std::string>()).ToStdWstring();
						if (!result.full_description.empty())
							return;
					}
				}

				result.full_description = "readme.txt";
			}
		}();

		[&] {  // load short
			if (auto const shortIt = it->find("short"); shortIt != it->end() && shortIt->is_object())
			{
				if (auto const preferredIt = shortIt->find(preferredLng);
					preferredIt != shortIt->end() && preferredIt->is_string())
				{
					result.short_description =
						wxString::FromUTF8(preferredIt->get<std::string>()).ToStdWstring();
					if (!result.short_description.empty())
						return;
				}

				if (preferredLng != mm::constant::default_language)
				{
					if (auto const defaultIt = shortIt->find(mm::constant::default_language);
						defaultIt != shortIt->end() && defaultIt->is_string())
					{
						result.short_description =
							wxString::FromUTF8(defaultIt->get<std::string>()).ToStdWstring();
						if (!result.short_description.empty())
							return;
					}
				}
			}
		}();

	}();

	if (const auto ico = find_object_value(&data, "icon"))
	{
		result.icon_filename = get_string_value(ico, "file");

		if (const auto index = find_int_value(ico, "index"))
			result.icon_index = index->get<size_t>();
	}

	result.category = get_string_value(&data, "category");

	if (const auto author = find_string_value(&data, "author"))
		result.authors = wxString::FromUTF8(author->get<std::string>());

	result.homepage_link = get_string_value(&data, "homepage");

	if (auto const compat = data.find("compatibility"); compat != data.end())
	{
		if (auto const req = compat->find("requires"); req != compat->end() && req->is_array())
		{
			for (auto const item : *req)
				if (item.is_string())
					result.requires.emplace(item.get<std::string>());

			hasRequires = true;
		}

		if (auto const after = compat->find("load_after"); after != compat->end() && after->is_array())
		{
			for (auto const item : *after)
				if (item.is_string())
					result.load_after.emplace(item.get<std::string>());

			hasLoadAfter = true;
		}

		if (auto const inc = compat->find("incompatible"); inc != compat->end() && inc->is_array())
		{
			for (auto const item : *inc)
				if (item.is_string())
					result.incompatible.emplace(item.get<std::string>());

			hasIncompatible = true;
		}
	}

	return supplyResultWithDefaults();
}
