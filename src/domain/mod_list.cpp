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

std::string ModList::next(const std::string& id) const
{
	const auto position = [&] {
		const auto pos = this->position(id);
		if (!pos)
			return 0u;

		return pos == data.size() - 1 ? *pos - 1 : *pos + 1;
	}();

	return position >= data.size() ? "" : data[position].id;
}

std::optional<ModList::ModState> ModList::state(const std::string& id) const
{
	if (auto pos = position(id))
		return data[*pos].state;

	return {};
}

bool ModList::enabled(const std::string& id) const
{
	const auto s = state(id);

	return s && *s == ModState::enabled;
}

bool ModList::disabled(const std::string& id) const
{
	const auto s = state(id);

	return s && *s == ModState::disabled;
}

void ModList::enable(const std::string& id)
{
	const auto pos = position(id);
	if (!pos)
	{
		data.emplace(data.begin() + 0, Mod { id, ModState::enabled });
		rest.erase(id);
		return;
	}

	data[*pos].state = ModState::enabled;
}

void ModList::enable(const std::string& id, size_t at)
{
	const auto pos = position(id);
	if (!pos)
	{
		data.emplace(data.begin() + at, Mod { id, ModState::enabled });
		rest.erase(id);
		return;
	}

	if (*pos != at)
		move(*pos, at);

	data[at].state = ModState::enabled;
}

void ModList::disable(const std::string& id)
{
	const auto pos = position(id);
	if (pos)
		data[*pos].state = ModState::disabled;
}

void ModList::switchState(const std::string& id)
{
	if (rest.contains(id))
		enable(id);
	else
		disable(id);
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
		archive(from);
		return;
	}

	if (!posFrom && posTo)
	{
		enable(from);

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

void ModList::archive(const std::string& id)
{
	const auto pos = position(id);
	if (!pos)
		return;

	rest.emplace(data[*pos].id);
	data.erase(data.begin() + *pos);
}

void ModList::remove(const std::string& id)
{
	disable(id);
	data.erase(data.begin() + *position(id));
}
