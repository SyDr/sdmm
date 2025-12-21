// SD Mod Manager

// Copyright (c) 2024 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <array>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <magic_enum.hpp>

class wxString;

namespace mm
{
	enum class ModListModelColumn
	{
		undefined  = 0,  // 0 is not used to allow save config in form -2, 1, 3
		status     = 1,
		priority   = 2,
		name       = 3,
		support    = 4,
		author     = 5,
		category   = 6,
		version    = 7,
		checkbox   = 8,
		load_order = 9,
		directory  = 10,

		total = directory + 1,
	};

	constexpr const std::array MainListColumns = { ModListModelColumn::priority, ModListModelColumn::name,
		ModListModelColumn::category, ModListModelColumn::version, ModListModelColumn::support,
		ModListModelColumn::author, ModListModelColumn::directory };

	enum class ModListModelManagedMode
	{
		as_flat_list = 0,
		as_group     = 1,
	};

	inline constexpr auto ManagedModeValues = magic_enum::enum_values<ModListModelManagedMode>();

	enum class ModListModelArchivedMode
	{
		as_flat_list         = 0,
		as_single_group      = 1,
		as_individual_groups = 2,
	};

	inline constexpr auto ArchivedModeValues = magic_enum::enum_values<ModListModelArchivedMode>();

	struct ModListDsplayedData
	{
		struct ManagedGroupTag
		{
			std::weak_ordering operator<=>(const ManagedGroupTag& other) const = default;
		};

		struct ArchivedGroupTag
		{
			std::weak_ordering operator<=>(const ArchivedGroupTag& other) const = default;
		};

		using GroupItemsBy       = std::variant<ManagedGroupTag, std::string, ArchivedGroupTag>;
		using CategoryAndCaption = std::pair<GroupItemsBy, wxString>;

		std::vector<std::string>        items;
		std::vector<CategoryAndCaption> categories;

		struct GroupItemsByToStringVisitor
		{
			std::string operator()(const ManagedGroupTag&)
			{
				return "@managed";
			}

			std::string operator()(const ArchivedGroupTag&)
			{
				return "@archived";
			}

			std::string operator()(const std::string& value)
			{
				return value;
			}
		};

		static std::string GroupItemsByToString(const GroupItemsBy& value)
		{
			return std::visit(GroupItemsByToStringVisitor(), value);
		}

		static GroupItemsBy StringToGroupItemsBy(std::string_view value)
		{
			if (value == "@managed")
				return ManagedGroupTag();

			if (value == "@archived")
				return ArchivedGroupTag();

			return std::string(value);
		}
	};
}
