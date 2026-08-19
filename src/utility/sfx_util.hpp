// SD Mod Manager

// Copyright (c) 2026 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "type/filesystem.hpp"

#include <nlohmann/json.hpp>

#include <string>
#include <unordered_map>

#include <optional>

namespace mm::sfx
{
	bool updateData(const fs::path& path, const fs::path& icon);
}
