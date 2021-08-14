// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <filesystem>

namespace mm
{
	struct INonAutoApplicablePlatform
	{
		virtual ~INonAutoApplicablePlatform() = default;

		virtual bool changed() const = 0;
		virtual void apply()         = 0;
		virtual void revert()        = 0;
	};
}
