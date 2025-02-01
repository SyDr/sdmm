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
	enum class InterfaceSize;

	struct IconStorage : public IIconStorage
	{
		IconStorage(InterfaceSize interfaceSize);

		wxBitmap get(IconPredefined icon, std::optional<IconPredefinedSize> targetSize) override;
		wxBitmap get(const std::string& name, std::optional<IconPredefinedSize> resizeTo) override;

		using IconLocation = std::variant<IconPredefined, std::string>;
		using IconCacheKey = std::pair<IconLocation, wxSize>;
		using IconCache    = std::unordered_map<IconCacheKey, wxBitmap, boost::hash<IconCacheKey>>;

	private:
		IconCache _iconCache;
		IconPredefinedSize _defaultSize = IconPredefinedSize::x16;
	};
}
