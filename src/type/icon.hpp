// SD Mod Manager

// Copyright (c) 2020-2025 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <wx/gdicmn.h>

namespace mm
{
	namespace Icon
	{
		enum class Stock
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

		enum class Size
		{
			x16,
			x24,
			x32,
			x48,
			x64,
		};
	}

	inline wxSize iconSize(Icon::Size value)
	{
		switch (value)
		{
		case Icon::Size::x16: return { 16, 16 };
		case Icon::Size::x24: return { 24, 24 };
		case Icon::Size::x32: return { 32, 32 };
		case Icon::Size::x48: return { 48, 48 };
		case Icon::Size::x64: return { 64, 64 };
		}

		return { 16, 16 };
	}
}
