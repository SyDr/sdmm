// SD Mod Manager

// Copyright (c) 2024 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <array>
#include <string>
#include <utility>
#include <variant>
#include <vector>

class wxString;

namespace mm
{
	enum class ModListModelColumn
	{
		undefined  = 0,  // 0 is not used to allow save config in form -2, 1, 3
		status     = 1,
		priority   = 2,
		name       = 3,
		author     = 4,
		category   = 5,
		version    = 6,
		checkbox   = 7,
		load_order = 8,
		directory  = 9,

		total = directory + 1,
	};

	constexpr const std::array MainListColumns = { ModListModelColumn::priority, ModListModelColumn::name,
		ModListModelColumn::category, ModListModelColumn::version, ModListModelColumn::author,
		ModListModelColumn::directory };

	inline std::string to_string(ModListModelColumn value)
	{
		switch (value)
		{
			using enum ModListModelColumn;
		case priority: return "Priority";
		case name: return "Mod";
		case author: return "Author";
		case category: return "Category";
		case version: return "Version";
		case directory: return "Directory";
		}

		return "";
	}

	inline ModListModelColumn ModListModelColumn_from_string(
		const std::string& value)  // TODO: do something with this ugly name
	{
		if (value == "Priority")
			return ModListModelColumn::priority;

		if (value == "Mod")
			return ModListModelColumn::name;

		if (value == "Author")
			return ModListModelColumn::author;

		if (value == "Category")
			return ModListModelColumn::category;

		if (value == "Version")
			return ModListModelColumn::version;

		if (value == "Directory")
			return ModListModelColumn::directory;

		return ModListModelColumn::undefined;
	}

	enum class ModListModelManagedMode
	{
		as_flat_list = 0,
		as_group     = 1,
	};

	constexpr const std::array ManagedModeValues = { ModListModelManagedMode::as_flat_list,
		ModListModelManagedMode::as_group };

	inline std::string to_string(ModListModelManagedMode value)
	{
		switch (value)
		{
			using enum ModListModelManagedMode;
		case as_flat_list: return "as a flat list";
		case as_group: return "as a group";
		}

		return "";
	}

	enum class ModListModelArchivedMode
	{
		as_flat_list         = 0,
		as_single_group      = 1,
		as_individual_groups = 2,
	};

	constexpr const std::array ArchivedModeValues = { ModListModelArchivedMode::as_flat_list,
		ModListModelArchivedMode::as_single_group, ModListModelArchivedMode::as_individual_groups };

	inline std::string to_string(ModListModelArchivedMode value)
	{
		switch (value)
		{
			using enum ModListModelArchivedMode;
		case as_flat_list: return "as a flat list";
		case as_single_group: return "as a single group";
		case as_individual_groups: return "grouped by category";
		}

		return "";
	}

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

		static std::string
		GroupItemsByToString(const GroupItemsBy& value)
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
