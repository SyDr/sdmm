// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "mod_list.hpp"
#include "utility/sdlexcept.h"

using namespace mm;

ModList::ModList(
	std::set<std::string> available, std::vector<std::string> active, std::set<std::string> hidden)
	: available(std::move(available))
	, active(std::move(active))
	, hidden(std::move(hidden))
{}

bool ModList::isActive(const std::string& item) const
{
	return activePosition(item).has_value();
}

bool ModList::isHidden(const std::string& item) const
{
	return hidden.contains(item);
}

std::optional<size_t> ModList::activePosition(const std::string& item) const
{
	if (auto it = std::find(active.cbegin(), active.cend(), item); it != active.cend())
		return std::distance(active.cbegin(), it);

	return {};
}

void ModList::activate(const std::string& item)
{
	active.emplace(active.begin(), item);
}

void ModList::deactivate(const std::string& item)
{
	if (const auto position = activePosition(item); position.has_value())
		active.erase(active.begin() + position.value());
}

void ModList::switchState(const std::string& item)
{
	if (const auto position = activePosition(item); position.has_value())
		active.erase(active.begin() + position.value());
	else
		active.emplace(active.begin(), item);
}

bool ModList::canMove(const std::string& from, const std::string& to) const
{
	if (const auto position1 = activePosition(from); position1.has_value())
		if (const auto position2 = activePosition(to); position2.has_value())
			return true;

	return false;
}

void ModList::move(const std::string& from, const std::string& to)
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

bool ModList::canMoveUp(const std::string& item) const
{
	if (const auto position = activePosition(item); position.has_value() && position.value() != 0)
		return true;

	return false;
}

void ModList::moveUp(const std::string& item)
{
	if (const auto position = activePosition(item); position.has_value() && position.value() != 0)
		std::swap(active[position.value()], active[position.value() - 1]);
}

bool ModList::canMoveDown(const std::string& item) const
{
	if (const auto position = activePosition(item))
		return position.has_value() && position.value() != active.size() - 1;

	return false;
}

void ModList::moveDown(const std::string& item)
{
	if (const auto position = activePosition(item);
		position.has_value() && position.value() != active.size() - 1)
	{
		std::swap(active[position.value()], active[position.value() + 1]);
	}
}

void ModList::hide(const std::string& item)
{
	hidden.emplace(item);
}

void ModList::show(const std::string& item)
{
	hidden.erase(item);
}

void ModList::switchVisibility(const std::string& item)
{
	if (hidden.contains(item))
		show(item);
	else
		hide(item);
}

void ModList::remove(const std::string& item)
{
	show(item);
	deactivate(item);
	available.erase(item);
}
