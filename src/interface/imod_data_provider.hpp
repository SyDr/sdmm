// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include "utility/non_owning_ptr.hpp"

class wxString;

namespace mm
{
	struct ModData;

	struct IModDataProvider
	{
		virtual ~IModDataProvider() = default;

		virtual non_owning_ptr<ModData const> modData(const wxString& id) = 0;
	};
}
