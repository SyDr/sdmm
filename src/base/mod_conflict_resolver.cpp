// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "application.h"
#include "domain/imod_data_provider.hpp"
#include "domain/mod_data.hpp"
#include "domain/mod_list.hpp"
#include "mod_conflict_resolver.hpp"

#include <wx/string.h>

#include <fmt/format.h>

#include <set>

using namespace mm;

ModList mm::resolve_mod_conflicts(ModList mods, IModDataProvider& modDataProvider)
{
	auto currentlyActive = mods.active;

	std::map<wxString, std::set<wxString>> activatedInSession;
	std::map<wxString, std::set<wxString>> disabledInSession;

	for (size_t i = 0; i < currentlyActive.size();)
	{
		auto const currentId = currentlyActive[i];

		auto modData = modDataProvider.modData(currentId);
		for (auto const& id : modData->requires)
		{
			activatedInSession[id].insert(currentId);

			if (std::find(currentlyActive.cbegin(), currentlyActive.cend(), id) ==
				currentlyActive.cend())
				currentlyActive.emplace_back(id);

			if (modDataProvider.modData(id)->virtual_mod)
				wxLogWarning(
					wxString::Format("Mod %s required by %s, "
									 "but unavailable"_lng,
									 id, currentId));

			if (auto it = disabledInSession.find(id); it != disabledInSession.cend())
				wxLogWarning(
					wxString::Format("Mod %s required by %s, "
									 "but incompatible with (%s)"_lng,
									 id, currentId, boost::algorithm::join(it->second, ", ")));
		}

		for (auto const& id : modData->incompatible)
		{
			disabledInSession[id].insert(currentId);

			if (auto it = std::find(currentlyActive.cbegin(), currentlyActive.cend(), id);
				it != currentlyActive.cend())
				currentlyActive.erase(it);

			if (auto it = activatedInSession.find(id); it != activatedInSession.cend())
				wxLogWarning(
					wxString::Format("Mod %s incompatible with %s, "
									 "but required by (%s)"_lng,
									 id, currentId, boost::algorithm::join(it->second, ", ")));
		}

		i++;
	}

	std::vector<wxString> sortedActive;

	for (size_t i = 0; !currentlyActive.empty();)
	{
		auto const& candidate = currentlyActive[i];

		bool ok = true;
		for (size_t j = 0; ok && j < currentlyActive.size(); ++j)
		{
			if (i == j)
				continue;

			if (modDataProvider.modData(currentlyActive[j])->load_after.count(candidate))
				ok = false;
		}

		if (ok)
		{
			sortedActive.emplace_back(candidate);
			currentlyActive.erase(currentlyActive.cbegin() + i);
			i = 0;
		}
		else
		{
			++i;
			if (i == currentlyActive.size())
			{
				wxLogWarning(wxString::Format(
					"No mods from (%s) can be placed above each other, "
					"%s placed at the top"_lng,
					boost::algorithm::join(currentlyActive, ", "), currentlyActive.front()));
				sortedActive.emplace_back(currentlyActive.front());
				currentlyActive.erase(currentlyActive.cbegin());
				i = 0;
			}
		}
	}

	mods.active = sortedActive;
	for (auto const& item : sortedActive)
		mods.hidden.erase(item);

	return mods;
}
