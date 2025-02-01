// SD Mod Manager

// Copyright (c) 2020-2025 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <wx/gdicmn.h>

namespace mm
{
	enum class IconPredefinedSize
	{
		x16,
		x24,
		x32,
		x48,
		x64,
	};

	enum class IconPredefined
	{
		blank,
		bookmark,
		checkmark,
		checkmark_green,
		circle,
		clear,
		cog,
		copy,
		cross_gray,
		double_down,
		double_up,
		down,
		folder,
		question,
		save_to_file,
		sort,
		up,
	};

	inline wxSize iconSize(IconPredefinedSize value)
	{
		switch (value)
		{
		case IconPredefinedSize::x16: return { 16, 16 };
		case IconPredefinedSize::x24: return { 24, 24 };
		case IconPredefinedSize::x32: return { 32, 32 };
		case IconPredefinedSize::x48: return { 48, 48 };
		case IconPredefinedSize::x64: return { 64, 64 };
		}

		return { 16, 16 };
	}
}
