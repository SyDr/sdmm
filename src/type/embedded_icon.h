// SD Mod Manager

// Copyright (c) 2020-2024 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

namespace mm
{
	inline constexpr const auto IconBlank = "icons/blank.png";  // todo: make blank by code instead

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
		circle,
	};

	namespace embedded_icon
	{

		constexpr const auto blank          = "icons/blank.png";
		constexpr const auto bookmark       = "icons/bookmark.png";
		constexpr const auto clear          = "icons/clear.png";
		constexpr const auto cog            = "icons/cog.png";
		constexpr const auto copy           = "icons/copy.png";
		constexpr const auto cross_gray     = "icons/cross-gray.png";
		constexpr const auto double_down    = "icons/double-down.png";
		constexpr const auto double_up      = "icons/double-up.png";
		constexpr const auto down           = "icons/down.png";
		constexpr const auto folder         = "icons/folder.png";
		constexpr const auto main_icon      = "icons/main-icon.png";
		constexpr const auto maximize       = "icons/maximize.png";
		constexpr const auto minus          = "icons/minus.png";
		constexpr const auto minus_gray     = "icons/minus-gray.png";
		constexpr const auto plus           = "icons/plus.png";
		constexpr const auto question       = "icons/question.png";
		constexpr const auto reset_position = "icons/reset-position.png";
		constexpr const auto save_to_file   = "icons/save-to-file.png";
		constexpr const auto sort           = "icons/sort.png";
		constexpr const auto tick           = "icons/tick.png";
		constexpr const auto tick_gray      = "icons/tick-gray.png";
		constexpr const auto tick_green     = "icons/tick-green.png";
		constexpr const auto up             = "icons/up.png";
	}
}
