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
{}

bool ModList::isActive(const wxString& item) const
{
	return activePosition(item).has_value();
}

bool ModList::isHidden(const wxString& item) const
{
	return hidden.contains(item);
}

std::optional<size_t> ModList::activePosition(const wxString& item) const
{
	if (auto it = std::find(active.cbegin(), active.cend(), item); it != active.cend())
		return std::distance(active.cbegin(), it);

	return {};
}

void ModList::activate(const wxString& item)
{
	active.emplace(active.begin(), item);
}

void ModList::deactivate(const wxString& item)
{
	if (const auto position = activePosition(item); position.has_value())
		active.erase(active.begin() + position.value());
}

void ModList::switchState(const wxString& item)
{
	if (const auto position = activePosition(item); position.has_value())
		active.erase(active.begin() + position.value());
	else
		active.emplace(active.begin(), item);
}

bool ModList::canMove(const wxString& from, const wxString& to) const
{
	if (const auto position1 = activePosition(from); position1.has_value())
		if (const auto position2 = activePosition(to); position2.has_value())
			return true;

	return false;
}

void ModList::move(const wxString& from, const wxString& to)
{
	auto       posFrom = activePosition(from);
	const auto posTo   = activePosition(to);

	if (!posTo.has_value())
	{
		if (posFrom.has_value())
			deactivate(from);

		return;
	}

	if (!posFrom.has_value())
	{
		activate(from);
		posFrom = activePosition(from);
	}

	const auto step = posFrom.value() < posTo.value() ? 1 : -1;

	for (auto index = posFrom.value(); index != posTo.value(); index += step)
		std::swap(active[index], active[index + step]);
}

bool ModList::canMoveUp(const wxString& item) const
{
	if (const auto position = activePosition(item); position.has_value() && position.value() != 0)
		return true;

	return false;
}

void ModList::moveUp(const wxString& item)
{
	if (const auto position = activePosition(item); position.has_value() && position.value() != 0)
		std::swap(active[position.value()], active[position.value() - 1]);
}

bool ModList::canMoveDown(const wxString& item) const
{
	if (const auto position = activePosition(item))
		return position.has_value() && position.value() != active.size() - 1;

	return false;
}

void ModList::moveDown(const wxString& item)
{
	if (const auto position = activePosition(item); position.has_value() && position.value() != active.size() - 1)
	{
		std::swap(active[position.value()], active[position.value() + 1]);
	}
}

void ModList::hide(const wxString& item)
{
	hidden.emplace(item);
}

void ModList::show(const wxString& item)
{
	hidden.erase(item);
}

void ModList::switchVisibility(const wxString& item)
{
	if (hidden.contains(item))
		show(item);
	else
		hide(item);
}

void ModList::remove(const wxString& item)
{
	show(item);
	deactivate(item);
	available.erase(item);
}
