// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "era2_mod_data_provider.hpp"
// #include "domain/mod_data.hpp"

#include "era2_mod_loader.hpp"
#include "system_info.hpp"
#include "utility/sdlexcept.h"

using namespace mm;

Era2ModDataProvider::Era2ModDataProvider(fs::path basePath, wxString preferredLng)
	: _basePath(std::move(basePath))
	, _preferredLng(std::move(preferredLng))
{
	loadDefaults();
}

 const ModData& Era2ModDataProvider::modData(const wxString& id)
{
	auto it = _data.find(id);

	if (it == _data.cend())
	{
		auto modData = mm::era2_mod_loader::updateAvailability(_basePath / id.ToStdString(wxConvUTF8),
			_preferredLng, _defaultIncompatible[id], _defaultRequires[id], _defaultLoadAfter[id]);

		std::tie(it, std::ignore) = _data.emplace(id, std::move(modData));
	}

	return it->second;
}

void Era2ModDataProvider::loadDefaults()
{
	boost::nowide::ifstream datafile(fs::path(mm::SystemInfo::DataDir) / "era2.json");

	auto data = nlohmann::json::parse(datafile);
	MM_EXPECTS(data.is_object(), unexpected_error);

	for (const auto& [modId, modData] : data.items())
	{
		auto wxKey = wxString::FromUTF8(modId);

		for (const auto& item : modData["incompatible"])
		{
			auto wxValue = wxString::FromUTF8(item.get<std::string>());

			_defaultIncompatible[wxKey].emplace(wxValue);
			_defaultIncompatible[wxValue].emplace(wxKey);
		}

		for (const auto& item : modData["requires"])
		{
			auto wxValue = wxString::FromUTF8(item.get<std::string>());
			_defaultRequires[wxKey].emplace(wxValue);
		}

		for (const auto& item : modData["load_after"])
		{
			auto wxValue = wxString::FromUTF8(item.get<std::string>());
			_defaultLoadAfter[wxKey].emplace(wxValue);
		}
	}
}
