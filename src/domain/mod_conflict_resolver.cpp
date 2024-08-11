// SD Mod Manager

// Copyright (c) 2020-2024 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include <wx/string.h>

#include "application.h"
#include "domain/mod_data.hpp"
#include "domain/mod_list.hpp"
#include "interface/imod_data_provider.hpp"
#include "mod_conflict_resolver.hpp"

#include <boost/range/algorithm_ext/erase.hpp>
#include <set>

using namespace mm;

namespace
{
	void expandRequirements(
		std::vector<std::string>& where, IModDataProvider& modDataProvider, const std::string& currentId)
	{
		if (std::find(where.begin(), where.end(), currentId) == where.end())
			where.emplace_back(currentId);

		const auto& modData = modDataProvider.modData(currentId);
		for (const auto& id : modData.requires_)
			expandRequirements(where, modDataProvider, id);
	}

	void reduceRequirements(std::set<std::string>& where, const std::vector<std::string>& active,
		IModDataProvider& modDataProvider, const std::string& currentId)
	{
		if (where.contains(currentId))
			return;

		where.emplace(currentId);

		for (const auto& id : active)
		{
			const auto& modData = modDataProvider.modData(id);
			if (modData.requires_.contains(currentId))
				reduceRequirements(where, active, modDataProvider, id);
		}
	}

	void reduceRequirementsTop(
		std::vector<std::string>& active, IModDataProvider& modDataProvider, const std::string& currentId)
	{
		if (currentId.empty())
			return;

		std::set<std::string> reducedRequirements;
		reduceRequirements(reducedRequirements, active, modDataProvider, currentId);

		boost::range::remove_erase_if(
			active, [&](const std::string& item) { return reducedRequirements.contains(item); });
	}
}

ModList mm::resolve_mod_conflicts(
	ModList mods, IModDataProvider& modDataProvider, const std::string& disablingMod)
{
	// expand current mod list to contain all mods, required by active mods
	std::vector<std::string> expandedRequirements;
	for (const auto& mod : mods.data)
		if (mod.state == ModList::ModState::active)
			expandRequirements(expandedRequirements, modDataProvider, mod.id);

	// remove mods if user disables mod
	// then remove mods, incompatible with top mods
	reduceRequirementsTop(expandedRequirements, modDataProvider, disablingMod);

	for (size_t i = 0; i < expandedRequirements.size(); ++i)
		for (const auto& id : modDataProvider.modData(expandedRequirements[i]).incompatible)
			reduceRequirementsTop(expandedRequirements, modDataProvider, id);

	std::vector<std::string> sortedActive;

	for (size_t i = 0; !expandedRequirements.empty();)
	{
		const auto& candidate = expandedRequirements[i];

		bool ok = true;
		for (size_t j = 0; ok && j < expandedRequirements.size(); ++j)
		{
			if (i == j)
				continue;

			if (modDataProvider.modData(expandedRequirements[j]).load_after.contains(candidate))
				ok = false;
		}

		if (ok)
		{
			sortedActive.emplace_back(candidate);
			expandedRequirements.erase(expandedRequirements.begin() + i);
			i = 0;
		}
		else
		{
			++i;
			if (i == expandedRequirements.size())
			{
				wxLogWarning(
					wxString::Format("No mods from (%s) can be placed above each other, "
									 "%s placed at the top"_lng,
						wxString::FromUTF8(boost::join(expandedRequirements, ", ")),
						wxString::FromUTF8(expandedRequirements.front())));
				sortedActive.emplace_back(expandedRequirements.front());
				expandedRequirements.erase(expandedRequirements.begin());
				i = 0;
			}
		}
	}

	std::unordered_set<std::string> active { sortedActive.cbegin(), sortedActive.cend() };

	size_t i = 0;

	size_t skip = 0;

	while (i < sortedActive.size())
	{
		const auto& id = sortedActive[i];

		if (mods.data.size() <= i + skip)
		{
			mods.activate(id, i + skip);
			++i;
		}
		else if (!active.count(mods.data[i + skip].id))
		{
			mods.deactivate(mods.data[i + skip].id);
			++skip;
		}
		else if (id != mods.data[i + skip].id)
		{
			mods.activate(id, i + skip);
			++i;
		}
		else if (mods.data[i + skip].state != ModList::ModState::active)
		{
			mods.activate(id, i + skip);
			++i;
		}
		else
		{
			++i;
		}
	}

	while (i + skip < mods.data.size())
	{
		mods.deactivate(mods.data[i + skip].id);
		++skip;
	}

	return mods;
}
