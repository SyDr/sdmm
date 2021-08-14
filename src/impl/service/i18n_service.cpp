// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "i18n_service.h"

#include <fstream>
#include <string>
#include <wx/stdpaths.h>
#include <boost/algorithm/string/case_conv.hpp>

#include "interface/iapp_config.h"
#include "utility/string_util.h"

using namespace mm;

I18nService::I18nService(const IAppConfig& config)
{
	std::ifstream datafile((config.programPath() / "lng" / (config.currentLanguageCode() + ".json")).string());

	if (datafile)
		build_cache(nlohmann::json::parse(datafile), "");
}

void I18nService::build_cache(const nlohmann::json& data, const wxString& prefix)
{
	wxASSERT_MSG(data.is_object(), "Unexpected type when parsing " + prefix);

	for (auto it = data.begin(); it != data.end(); ++it)
	{
		auto const key = prefix + it.key();

		switch (it->type())
		{
		case nlohmann::json::value_t::string:
			_data[key] = wxString::FromUTF8(it->get<std::string>());
			break;
		case nlohmann::json::value_t::object:
			build_cache(it.value(), key + "/");
			break;
		default:
			wxFAIL_MSG("Unexpected type when parsing " + prefix);
			break;
		}
	}
}

wxString I18nService::category(const wxString& category) const
{
	if (auto const it = _data.find("category/" + category.Lower()); it != _data.cend())
		return it->second;

	return category;
}

wxString I18nService::get(const wxString& key) const
{
	if (const auto it = _data.find(key); it != _data.cend())
		return it->second;

	wxLogDebug("Translation string not found \"%s\"", key);

	return key;
}

wxString I18nService::languageName(wxString const& code) const
{
	if (code == "en_US")
		return L"English";
	if (code == "ru_RU")
		return L"Русский";

	return code; // TODO: return name from lng file itself
}
