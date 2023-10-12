// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <unordered_map>

#include "interface/iicon_storage.hpp"

namespace mm
{
	class IconStorage : public IIconStorage
	{
	public:
		wxIcon get(const std::string& name) override;

	private:
		std::unordered_map<std::string, wxIcon> _cache;
	};
}
