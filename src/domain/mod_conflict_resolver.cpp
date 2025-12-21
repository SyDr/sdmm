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
	struct CompatibilityInfo
	{
		int priority = 0;

		std::set<std::string> incompatible;
		std::set<std::string> requires_;
		std::set<std::string> load_after;
	};

	using CompatMap = std::unordered_map<std::string, CompatibilityInfo>;

	void expandRequirements(std::vector<std::string>& where, CompatMap& cm, const std::string& currentId)
	{
		if (std::find(where.begin(), where.end(), currentId) == where.end())
			where.emplace_back(currentId);

		const auto& modData = cm[currentId];
		for (const auto& id : modData.requires_)
			expandRequirements(where, cm, id);
	}

	void reduceRequirements(std::set<std::string>& where, const std::vector<std::string>& active,
		CompatMap& cm, const std::string& currentId)
	{
		if (where.contains(currentId))  // requirements chain already added
			return;

		where.emplace(currentId);

		for (const auto& id : active)
		{
			const auto& modData = cm[id];
			if (modData.requires_.contains(currentId))
				reduceRequirements(where, active, cm, id);
		}
	}

	void reduceRequirementsTop(std::vector<std::string>& active, CompatMap& cm, const std::string& currentId)
	{
		if (currentId.empty())
			return;

		std::set<std::string> reducedRequirements;
		reduceRequirements(reducedRequirements, active, cm, currentId);

		boost::range::remove_erase_if(
			active, [&](const std::string& item) { return reducedRequirements.contains(item); });
	}

	void reduceIncompatibleChain(
		std::vector<std::string>& active, CompatMap& cm, const std::string& currentId)
	{
		if (currentId.empty())
			return;

		for (const auto& id : cm[currentId].incompatible)
			reduceRequirementsTop(active, cm, id);

		for (const auto& id : cm[currentId].requires_)
			reduceIncompatibleChain(active, cm, id);
	}
}

std::vector<std::string> mm::ResolveModConflicts(const ModList& mods, IModDataProvider& modDataProvider,
	const std::string& enablingMod, const std::string& disablingMod)
{
	// copy compatibility info from data provider
	CompatMap cm;
	for (const auto& mod : mods.data)
	{
		auto [it, _] = cm.insert({ mod.id, {} });

		auto& data = modDataProvider.modData(mod.id);

		it->second.priority = data.priority;

		it->second.incompatible = data.incompatible;
		it->second.requires_    = data.requires_;
		it->second.load_after   = data.load_after;
	}

	// incompatibility info is viral
	for (auto& [key, value] : cm)
	{
		for (const auto& item : value.incompatible)
			cm[item].incompatible.insert(key);
	}

	// expand current mod list to contain all mods, required by active mods
	std::vector<std::string> expandedRequirements;
	for (const auto& mod : mods.data)
		if (mod.state == ModList::ModState::enabled)
			expandedRequirements.emplace_back(mod.id);

	for (const auto& mod : mods.data)
		if (mod.state == ModList::ModState::enabled)
			expandRequirements(expandedRequirements, cm, mod.id);

	// remove mods if user disables mod
	// then remove mods, incompatible with mod, which user enables
	// then remove mods, incompatible with top mods
	reduceRequirementsTop(expandedRequirements, cm, disablingMod);
	reduceIncompatibleChain(expandedRequirements, cm, enablingMod);

	for (size_t i = 0; i < expandedRequirements.size(); ++i)
	{
		const auto copy = expandedRequirements[i];
		reduceIncompatibleChain(expandedRequirements, cm, copy);
	}

	std::vector<std::string> sortedActive;

	for (size_t i = 0; !expandedRequirements.empty();)
	{
		const auto& candidate = expandedRequirements[i];

		bool ok = true;
		for (size_t j = 0; ok && j < expandedRequirements.size(); ++j)
		{
			if (i == j)
				continue;

			if (cm[expandedRequirements[j]].load_after.contains(candidate))
				ok = false;

			if (cm[expandedRequirements[j]].priority > cm[candidate].priority)
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
					wxString::Format("message/warning/no_mods_can_be_placed_above_each_other"_lng,
						wxString::FromUTF8(boost::join(expandedRequirements, ", ")),
						wxString::FromUTF8(expandedRequirements.front())));
				sortedActive.emplace_back(expandedRequirements.front());
				expandedRequirements.erase(expandedRequirements.begin());
				i = 0;
			}
		}
	}

	return sortedActive;
}
