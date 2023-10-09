// SD Mod Manager

// Copyright (c) 2020-2023 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <wx/string.h>

#include <compare>
#include <optional>
#include <set>
#include <vector>

namespace mm
{
	struct ModList
	{
		std::set<wxString>    available;
		std::set<wxString>    hidden;
		std::vector<wxString> active;

		std::vector<wxString> invalid;

		ModList() = default;
		ModList(std::set<wxString> available, std::vector<wxString> active, std::set<wxString> hidden);

		bool isActive(const wxString& item) const;
		bool isHidden(const wxString& item) const;

		std::optional<size_t> activePosition(const wxString& item) const;
		void                  activate(const wxString& item);
		void                  deactivate(const wxString& item);
		void                  switchState(const wxString& item);

		bool canMove(const wxString& from, const wxString& to) const;
		void move(const wxString& from, const wxString& to);

		bool canMoveUp(const wxString& item) const;
		void moveUp(const wxString& item);

		bool canMoveDown(const wxString& item) const;
		void moveDown(const wxString& item);

		void hide(const wxString& item);
		void show(const wxString& item);
		void switchVisibility(const wxString& item);

		void remove(const wxString& item);

		std::weak_ordering operator<=>(const ModList& other) const = default;
	};
}
