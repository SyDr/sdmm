// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "application.h"
#include "era2_mod_loader.hpp"
#include "system_info.hpp"
#include "utility/json_util.h"



using namespace mm;

namespace
{
	ModData supplyWithDefaults(ModData what, std::set<wxString> const& defaultIncompatible,
		std::set<wxString> const& defaultRequires, std::set<wxString> const& defaultLoadAfter,
		bool useIncompatbile, bool useRequires, bool useLoadAfter)
	{
		if (!useRequires)
		{
			what.requires_ = defaultRequires;
			if (what.id != L"WoG")
				what.requires_.emplace(L"WoG");
		}

		if (!useLoadAfter)
		{
			what.load_after = defaultLoadAfter;
			if (what.id != L"WoG")
				what.load_after.emplace(L"WoG");
		}

		if (!useIncompatbile)
		{
			what.incompatible = defaultIncompatible;
		}

		return what;
	}
}

ModData era2_mod_loader::updateAvailability(fs::path const& loadFrom,
	const wxString& preferredLng, std::set<wxString> const& defaultIncompatible,
	std::set<wxString> const& defaultRequires, std::set<wxString> const& defaultLoadAfter)
{
	bool hasRequires     = false;
	bool hasLoadAfter    = false;
	bool hasIncompatible = false;

	ModData result;
	result.data_path   = loadFrom;
	result.id          = loadFrom.filename().wstring();
	result.virtual_mod = !fs::is_directory(loadFrom);

	auto supplyResultWithDefaults = [&] {
		return supplyWithDefaults(std::move(result), defaultIncompatible, defaultRequires, defaultLoadAfter,
			hasIncompatible, hasRequires, hasLoadAfter);
	};

	if (result.virtual_mod)
	{
		result.caption = result.id;
		return supplyResultWithDefaults();
	}

	const auto path = loadFrom / mm::SystemInfo::ModInfoFilename;

	boost::nowide::ifstream datafile(path);

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
		wxLogError(wxString::FromUTF8(e.what()));
		wxLogError(wxString::Format("Error while parsing file %s"_lng, wxString::FromUTF8(path.string())));
	}

	if (!data.is_object())
	{
		result.caption = result.id;
		return supplyResultWithDefaults();
	}

	if (const auto ver = find_object_value(&data, "version"))
	{
		result.mod_platform = wxString::FromUTF8(get_string_value(ver, "platform"));
		result.mod_version  = wxString::FromUTF8(get_string_value(ver, "mod"));
		result.info_version = wxString::FromUTF8(get_string_value(ver, "info"));
	}
	else
	{
		result.mod_platform = wxString::FromUTF8(get_string_value(&data, "platform"));
		result.mod_version  = wxString::FromUTF8(get_string_value(&data, "mod_version"));
		result.info_version = wxString::FromUTF8(get_string_value(&data, "info_version"));
	}

	[&] {  // load caption
		const auto it = data.find("caption");
		if (it == data.end() || !it->is_object())
		{
			result.caption = result.id;
			return;
		}

		if (const auto preferredIt = it->find(preferredLng.ToStdString(wxConvUTF8));
			preferredIt != it->end() && preferredIt->is_string())
		{
			result.caption = wxString::FromUTF8(preferredIt->get<std::string>());
			if (!result.caption.empty())
				return;
		}

		if (preferredLng.ToStdString(wxConvUTF8) != mm::SystemInfo::DefaultLanguage)
		{
			if (const auto defaultIt = it->find(mm::SystemInfo::DefaultLanguage);
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
		const auto it = data.find("description");
		if (it == data.end() || !it->is_object())
			return;

		[&] {  // load full
			if (const auto fullIt = it->find("full"); fullIt != it->end() && fullIt->is_object())
			{
				if (const auto preferredIt = fullIt->find(preferredLng.ToStdString(wxConvUTF8));
					preferredIt != fullIt->end() && preferredIt->is_string())
				{
					result.full_description = preferredIt->get<std::string>();
					if (!result.full_description.empty())
						return;
				}

				if (preferredLng.ToStdString(wxConvUTF8) != mm::SystemInfo::DefaultLanguage)
				{
					if (const auto defaultIt = fullIt->find(mm::SystemInfo::DefaultLanguage);
						defaultIt != fullIt->end() && defaultIt->is_string())
					{
						result.full_description = defaultIt->get<std::string>();
						if (!result.full_description.empty())
							return;
					}
				}

				result.full_description = "readme.txt";
			}
		}();

		[&] {  // load short
			if (const auto shortIt = it->find("short"); shortIt != it->end() && shortIt->is_object())
			{
				if (const auto preferredIt = shortIt->find(preferredLng.ToStdString(wxConvUTF8));
					preferredIt != shortIt->end() && preferredIt->is_string())
				{
					result.short_description = wxString::FromUTF8(preferredIt->get<std::string>());
					if (!result.short_description.empty())
						return;
				}

				if (preferredLng.ToStdString(wxConvUTF8) != mm::SystemInfo::DefaultLanguage)
				{
					if (const auto defaultIt = shortIt->find(mm::SystemInfo::DefaultLanguage);
						defaultIt != shortIt->end() && defaultIt->is_string())
					{
						result.short_description = wxString::FromUTF8(defaultIt->get<std::string>());
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

	result.category = wxString::FromUTF8(get_string_value(&data, "category"));

	if (const auto author = find_string_value(&data, "author"))
		result.authors = wxString::FromUTF8(author->get<std::string>());

	result.homepage_link = wxString::FromUTF8(get_string_value(&data, "homepage"));

	if (const auto compat = data.find("compatibility"); compat != data.end())
	{
		if (const auto req = compat->find("requires"); req != compat->end() && req->is_array())
		{
			for (const auto& item : *req)
				if (item.is_string())
					result.requires_.emplace(wxString::FromUTF8(item.get<std::string>()));

			hasRequires = true;
		}

		if (const auto after = compat->find("load_after"); after != compat->end() && after->is_array())
		{
			for (const auto& item : *after)
				if (item.is_string())
					result.load_after.emplace(wxString::FromUTF8(item.get<std::string>()));

			hasLoadAfter = true;
		}

		if (const auto inc = compat->find("incompatible"); inc != compat->end() && inc->is_array())
		{
			for (const auto& item : *inc)
				if (item.is_string())
					result.incompatible.emplace(wxString::FromUTF8(item.get<std::string>()));

			hasIncompatible = true;
		}
	}

	return supplyResultWithDefaults();
}
