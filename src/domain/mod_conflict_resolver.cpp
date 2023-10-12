// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include <wx/string.h>

#include "application.h"
#include "interface/imod_data_provider.hpp"
#include "domain/mod_data.hpp"
#include "domain/mod_list.hpp"
#include "mod_conflict_resolver.hpp"

#include <set>

using namespace mm;

ModList mm::resolve_mod_conflicts(ModList mods, IModDataProvider& modDataProvider)
{
	auto currentlyActive = mods.active;

	std::map<wxString, std::set<wxString>> activatedInSession;
	std::map<wxString, std::set<wxString>> disabledInSession;

	for (size_t i = 0; i < currentlyActive.size();)
	{
		const auto currentId = currentlyActive[i];

		auto modData = modDataProvider.modData(currentId);
		for (const auto& id : modData.requires_)
		{
			activatedInSession[id].insert(currentId);

			if (std::find(currentlyActive.begin(), currentlyActive.end(), id) ==
				currentlyActive.end())
				currentlyActive.emplace_back(id);

			if (modDataProvider.modData(id).virtual_mod)
				wxLogWarning(
					wxString::Format("Mod %s required by %s, "
									 "but unavailable"_lng,
									 id, currentId));

			if (auto it = disabledInSession.find(id); it != disabledInSession.end())
				wxLogWarning(
					wxString::Format("Mod %s required by %s, "
									 "but incompatible with (%s)"_lng,
									 id, currentId, boost::join(it->second, L", ")));
		}

		for (const auto& id : modData.incompatible)
		{
			disabledInSession[id].insert(currentId);

			if (auto it = std::find(currentlyActive.begin(), currentlyActive.end(), id);
				it != currentlyActive.end())
				currentlyActive.erase(it);

			if (auto it = activatedInSession.find(id); it != activatedInSession.end())
				wxLogWarning(
					wxString::Format("Mod %s incompatible with %s, "
									 "but required by (%s)"_lng,
									 id, currentId, boost::join(it->second, L", ")));
		}

		i++;
	}

	std::vector<wxString> sortedActive;

	for (size_t i = 0; !currentlyActive.empty();)
	{
		const auto& candidate = currentlyActive[i];

		bool ok = true;
		for (size_t j = 0; ok && j < currentlyActive.size(); ++j)
		{
			if (i == j)
				continue;

			if (modDataProvider.modData(currentlyActive[j]).load_after.contains(candidate))
				ok = false;
		}

		if (ok)
		{
			sortedActive.emplace_back(candidate);
			currentlyActive.erase(currentlyActive.begin() + i);
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
					boost::join(currentlyActive, L", "), currentlyActive.front()));
				sortedActive.emplace_back(currentlyActive.front());
				currentlyActive.erase(currentlyActive.begin());
				i = 0;
			}
		}
	}

	mods.active = sortedActive;
	for (const auto& item : sortedActive)
		mods.hidden.erase(item);

	return mods;
}
