// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <wigwag/signal_connector.hpp>

class wxString;
class wxIcon;

namespace mm
{
	struct ILaunchHelper
	{
		virtual ~ILaunchHelper() = default;

		virtual std::string getExecutable() const = 0;
		virtual void setExecutable(const std::string& exe) = 0;

		virtual wxIcon getIcon() const = 0;
		virtual wxString getCaption() const = 0;
		virtual wxString getLaunchString() const = 0;

		virtual wigwag::signal_connector<void()> onDataChanged() const = 0;
	};
}
