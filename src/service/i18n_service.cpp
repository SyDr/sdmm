// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "i18n_service.h"

#include "interface/iapp_config.hpp"

#include <wx/stdpaths.h>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/nowide/fstream.hpp>

#include <fstream>
#include <string>

using namespace mm;

I18nService::I18nService(const IAppConfig& config)
{
	boost::nowide::ifstream datafile(config.programPath() / "lng" / (config.currentLanguageCode() + ".json"));

	if (datafile)
		build_cache(nlohmann::json::parse(datafile), "");
}

void I18nService::build_cache(const nlohmann::json& data, const std::string& prefix)
{
	wxASSERT_MSG(data.is_object(), wxString::FromUTF8("Unexpected type when parsing " + prefix));

	for (auto it = data.begin(); it != data.end(); ++it)
	{
		const auto key = prefix + it.key();

		switch (it->type())
		{
		case nlohmann::json::value_t::string:
			_data[key] = it->get<std::string>();
			break;
		case nlohmann::json::value_t::object:
			build_cache(it.value(), key + "/");
			break;
		default:
			wxFAIL_MSG(wxString::FromUTF8("Unexpected type when parsing " + prefix));
			break;
		}
	}
}

std::string I18nService::category(const std::string& category) const
{
	if (const auto it = _data.find("category/" + boost::to_lower_copy(category)); it != _data.cend())
		return it->second;

	return category;
}

std::string I18nService::get(const std::string& key) const
{
	if (const auto it = _data.find(key); it != _data.cend())
		return it->second;

	wxLogDebug(L"Translation string not found \"%s\"", wxString::FromUTF8(key));

	return key;
}

std::string I18nService::languageName(const std::string& code) const
{
	if (code == "en_US")
		return "English";

	if (code == "ru_RU")
		return boost::nowide::narrow(L"Русский");

	return code; // TODO: return name from lng file itself
}

std::string I18nService::legacyCode(const std::string& code) const
{
	if (code == "en")
		return "en_US";

	if (code == "ru")
		return "ru_RU";

	return {};
}
