// SD Mod Manager

// Copyright (c) 2020-2024 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#include "stdafx.h"

#include "mod_list.hpp"

#include "utility/sdlexcept.h"

using namespace mm;

bool ModList::managed(const std::string& id) const
{
	auto it = std::find_if(data.cbegin(), data.cend(), [&](const Mod& m) { return m.id == id; });

	return it != data.cend();
}

std::optional<size_t> ModList::position(const std::string& id) const
{
	auto it = std::find_if(data.cbegin(), data.cend(), [&](const Mod& m) { return m.id == id; });

	if (it != data.cend())
		return std::distance(data.cbegin(), it);

	return {};
}

std::optional<ModList::ModState> ModList::state(const std::string& id) const
{
	if (auto pos = position(id))
		return data[*pos].state;

	return {};
}

bool ModList::active(const std::string& id) const
{
	const auto s = state(id);

	return s && *s == ModState::active;
}

bool ModList::inactive(const std::string& id) const
{
	const auto s = state(id);

	return s && *s == ModState::inactive;
}

bool ModList::hidden(const std::string& id) const
{
	const auto s = state(id);

	return s && *s == ModState::hidden;
}

void ModList::activate(const std::string& id)
{
	const auto pos = position(id);
	if (!pos)
	{
		data.emplace(data.begin() + 0, Mod { id, ModState::active });
		rest.erase(id);
		return;
	}

	data[*pos].state = ModState::active;
}

void ModList::activate(const std::string& id, size_t at)
{
	const auto pos = position(id);
	if (!pos)
	{
		data.emplace(data.begin() + at, Mod { id, ModState::active });
		rest.erase(id);
		return;
	}

	if (*pos != at)
		move(*pos, at);

	data[at].state = ModState::active;
}

void ModList::deactivate(const std::string& id)
{
	const auto pos = position(id);
	if (pos)
		data[*pos].state = ModState::inactive;
}

void ModList::switchState(const std::string& id)
{
	if (rest.contains(id))
		activate(id);
	else
		deactivate(id);
}

bool ModList::canMove(const std::string&, const std::string&) const
{
	return true;
}

void ModList::move(size_t from, size_t to)
{
	auto item = data[from];

	data.erase(data.begin() + from);
	data.emplace(data.begin() + to, item);
}

void ModList::move(const std::string& from, const std::string& to)
{
	auto posFrom = position(from);
	auto posTo   = position(to);

	if (posFrom && !posTo)
	{
		reset(from);
		return;
	}

	if (!posFrom && posTo)
	{
		activate(from);

		posFrom = 0;
	}

	move(*posFrom, *posTo);
}

bool ModList::canMoveUp(const std::string& id) const
{
	auto pos = position(id);

	return pos && *pos > 0;
}

void ModList::moveUp(const std::string& id)
{
	auto posFrom = position(id);
	std::swap(data[*posFrom], data[*posFrom - 1]);
}

bool ModList::canMoveDown(const std::string& id) const
{
	auto pos = position(id);

	return pos && (*pos + 1 < data.size());
}

void ModList::moveDown(const std::string& id)
{
	auto posFrom = position(id);
	std::swap(data[*posFrom], data[*posFrom + 1]);
}

void ModList::hide(const std::string& id)
{
	const auto pos = position(id);
	if (pos)
		data[*pos].state = ModState::hidden;
}

void ModList::show(const std::string& id)
{
	deactivate(id);
}

void ModList::reset(const std::string& id)
{
	const auto pos = position(id);
	if (!pos)
		return;

	rest.emplace(data[*pos].id);
	data.erase(data.begin() + *pos);
}

void ModList::switchVisibility(const std::string& id)
{
	if (hidden(id))
		show(id);
	else
		hide(id);
}

void ModList::remove(const std::string& id)
{
	show(id);
	deactivate(id);
	data.erase(data.begin() + *position(id));
}
