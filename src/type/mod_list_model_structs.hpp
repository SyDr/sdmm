// SD Mod Manager

// Copyright (c) 2024 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <string>
#include <utility>
#include <variant>
#include <vector>

class wxString;

namespace mm
{
	enum class ModListModelManagedMode
	{
		as_flat_list,
		as_group,
	};

	enum class ModListModelArchivedMode
	{
		as_flat_list,
		as_single_group,
		as_individual_groups
	};

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
	};
}
