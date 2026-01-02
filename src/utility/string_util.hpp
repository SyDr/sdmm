// SD Mod Manager

// Copyright (c) 2026 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include <string_view>
#include <wx/string.h>

namespace mm
{
	inline wxString wxStringFromUnspecified(std::string_view s)
	{
		auto r = wxString::FromUTF8(s.data(), s.length());
		if (!r.empty())
			return r;

		return wxString(s.data(), wxConvLocal, s.length());
	}
}
