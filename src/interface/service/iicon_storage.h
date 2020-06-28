// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

class wxIcon;
class wxString;

namespace mm
{
	class IIconStorage
	{
	public:
		virtual ~IIconStorage() = default;

		virtual wxIcon get(const wxString& name) = 0;

		virtual void remove(const wxString& name) = 0;
	};
}
