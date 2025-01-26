// SD Mod Manager

// Copyright (c) 2025 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <array>

#include <magic_enum.hpp>

#include "embedded_icon.h"

namespace mm
{
	enum class InterfaceSize
	{
		standard = 0,
		big      = 1,
	};

	inline constexpr const auto InterfaceSizeValues = magic_enum::enum_values<InterfaceSize>();

	inline IconPredefinedSize toIconPredefinedSize(InterfaceSize value)
	{
		switch (value)
		{
		case InterfaceSize::standard: return IconPredefinedSize::x16;
		case InterfaceSize::big: return IconPredefinedSize::x32;
		}

		return IconPredefinedSize::x16;
	}

	inline int toBaseSize(InterfaceSize value)
	{
		switch (value)
		{
		case InterfaceSize::standard: return 160;
		case InterfaceSize::big: return 32;
		}

		return 16;
	}
}
