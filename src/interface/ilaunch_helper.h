// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <sigslot/signal.hpp>

class wxString;
class wxIcon;

namespace mm
{
	struct ILaunchHelper
	{
		virtual ~ILaunchHelper() = default;

		virtual std::string getExecutable() const                 = 0;
		virtual void        setExecutable(const std::string& exe) = 0;

		virtual wxIcon   getIcon() const         = 0;
		virtual wxString getCaption() const      = 0;
		virtual wxString getLaunchString() const = 0;

		virtual sigslot::signal<>& onDataChanged() = 0;
	};
}
