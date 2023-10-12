// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

class wxString;

namespace mm
{
	struct ModData;

	struct IModDataProvider
	{
		virtual ~IModDataProvider() = default;

		[[nodiscard]] virtual const ModData& modData(const wxString& id) = 0;
	};
}
