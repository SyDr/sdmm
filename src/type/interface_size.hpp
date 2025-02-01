// SD Mod Manager

// Copyright (c) 2025 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <array>

#include <magic_enum.hpp>

#include "icon.hpp"

namespace mm
{
	enum class InterfaceSize
	{
		standard = 0,
		big      = 1,
	};

	inline constexpr const auto InterfaceSizeValues = magic_enum::enum_values<InterfaceSize>();

	inline Icon::Size toIconPredefinedSize(InterfaceSize value)
	{
		switch (value)
		{
		case InterfaceSize::standard: return Icon::Size::x16;
		case InterfaceSize::big: return Icon::Size::x32;
		}

		return Icon::Size::x16;
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
