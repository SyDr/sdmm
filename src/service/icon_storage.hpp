// SD Mod Manager

// Copyright (c) 2020-2024 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "interface/iicon_storage.hpp"

#include <boost/functional/hash.hpp>
#include <wx/gdicmn.h>

#include <unordered_map>

std::size_t hash_value(const wxSize& v);

namespace mm
{
	struct IconStorage : public IIconStorage
	{
		wxBitmap get(IconPredefined icon, IconPredefinedSize targetSize) override;
		wxBitmap get(const std::string& name) override;

		wxBitmap get(const std::string& name, const wxSize& targetSize);

	private:
		using IconCacheKey = std::pair<std::string, wxSize>;
		std::unordered_map<IconCacheKey, wxBitmap, boost::hash<IconCacheKey>> _iconCache;
	};
}
