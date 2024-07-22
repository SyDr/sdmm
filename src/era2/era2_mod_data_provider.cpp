// SD Mod Manager

// Copyright (c) 2020-2024 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "era2_mod_data_provider.hpp"
// #include "domain/mod_data.hpp"

#include "era2_mod_data_loader.hpp"
#include "system_info.hpp"
#include "utility/sdlexcept.h"

using namespace mm;

Era2ModDataProvider::Era2ModDataProvider(
	fs::path basePath, std::string preferredLng, const II18nService& i18Service)
	: _basePath(std::move(basePath))
	, _preferredLng(std::move(preferredLng))
	, _i18Service(i18Service)
{
	loadDefaults();
}

const ModData& Era2ModDataProvider::modData(const std::string& id)
{
	auto it = _data.find(id);

	if (it == _data.cend())
	{
		auto modData = mm::Era2ModDataLoader::load(_basePath / id, _preferredLng,
			_defaultIncompatible[id], _defaultRequires[id], _defaultLoadAfter[id], _i18Service);

		std::tie(it, std::ignore) = _data.emplace(id, std::move(modData));
	}

	return it->second;
}

void Era2ModDataProvider::clear()
{
	_data.clear();
}

void Era2ModDataProvider::loadDefaults()
{
	boost::nowide::ifstream datafile(fs::path(mm::SystemInfo::DataDir) / "era2.json");

	auto data = nlohmann::json::parse(datafile);
	MM_EXPECTS(data.is_object(), unexpected_error);

	for (const auto& [modId, modData] : data.items())
	{
		for (const auto& item : modData["incompatible"])
		{
			auto value = item.get<std::string>();

			_defaultIncompatible[modId].emplace(value);
			_defaultIncompatible[value].emplace(modId);
		}

		for (const auto& item : modData["requires"])
			_defaultRequires[modId].emplace(item.get<std::string>());

		for (const auto& item : modData["load_after"])
			_defaultLoadAfter[modId].emplace(item.get<std::string>());
	}
}
