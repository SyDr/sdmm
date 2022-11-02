// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

class wxString;

namespace mm
{
	struct II18nService
	{
		virtual ~II18nService() = default;

		virtual wxString category(const wxString& category) const = 0;
		virtual wxString get(const wxString& key) const = 0;
		virtual wxString languageName(const wxString& code) const = 0;
	};
}
