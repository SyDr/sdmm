// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "mod_list.hpp"
#include "utility/sdlexcept.h"

using namespace mm;

ModList::ModList(std::set<wxString> available, std::vector<wxString> active, std::set<wxString> hidden)
	: available(std::move(available))
	, active(std::move(active))
	, hidden(std::move(hidden))
{
}

bool ModList::isActive(wxString const& item) const
{
	return activePosition(item).has_value();
}

bool ModList::isHidden(wxString const& item) const
{
	return hidden.count(item);
}

std::optional<size_t> ModList::activePosition(wxString const& item) const
{
	if (auto it = std::find(active.cbegin(), active.cend(), item); it != active.cend())
		return std::distance(active.cbegin(), it);

	return {};
}

void ModList::activate(wxString const& item)
{
	active.emplace(active.begin(), item);
}

void ModList::deactivate(wxString const& item)
{
	if (auto position = activePosition(item); position.has_value())
		active.erase(active.begin() + position.value());
}

void ModList::switchState(wxString const& item)
{
	if (auto position = activePosition(item); position.has_value())
		active.erase(active.begin() + position.value());
	else
		active.emplace(active.begin(), item);
}

bool ModList::canMove(wxString const& from, wxString const& to) const
{
	if (auto position1 = activePosition(from); position1.has_value())
		if (auto position2 = activePosition(to); position2.has_value())
			return true;

	return false;
}

void ModList::move(wxString const& from, wxString const& to)
{
	auto posFrom = activePosition(from);
	if (!posFrom.has_value())
		return;

	auto posTo = activePosition(to);
	if (!posTo.has_value())
		return;

	const auto step = posFrom.value() < posTo.value() ? 1 : -1;

	for (auto index = posFrom.value(); index != posTo.value(); index += step)
		std::swap(active[index], active[index + step]);
}

bool ModList::canMoveUp(wxString const& item) const
{
	if (auto position = activePosition(item); position.has_value() && position.value() != 0)
		return true;

	return false;
}

void ModList::moveUp(wxString const& item)
{
	if (auto position = activePosition(item); position.has_value() && position.value() != 0)
		std::swap(active[position.value()], active[position.value() - 1]);
}

bool ModList::canMoveDown(wxString const& item) const
{
	if (auto position = activePosition(item))
		return position.has_value() && position.value() != active.size() - 1;

	return false;
}

void ModList::moveDown(wxString const& item)
{
	if (auto position = activePosition(item); position.has_value() && position.value() != active.size() - 1)
	{
		std::swap(active[position.value()], active[position.value() + 1]);
	}
}

void ModList::hide(wxString const& item)
{
	hidden.emplace(item);
}

void ModList::show(wxString const& item)
{
	hidden.erase(item);
}

void ModList::switchVisibility(wxString const& item)
{
	if (hidden.count(item))
		show(item);
	else
		hide(item);
}

void ModList::remove(wxString const& item)
{
	show(item);
	deactivate(item);
	available.erase(item);
}
