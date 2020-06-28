// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <wx/string.h>

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

		ModList() = default;
		ModList(std::set<wxString> available, std::vector<wxString> active,
				std::set<wxString> hidden);

		bool isActive(wxString const& item) const;
		bool isHidden(wxString const& item) const;

		std::optional<size_t> activePosition(wxString const& item) const;
		void                  activate(wxString const& item);
		void                  deactivate(wxString const& item);
		void                  switchState(wxString const& item);

		bool canMove(wxString const& from, wxString const& to) const;
		void move(wxString const& from, wxString const& to);

		bool canMoveUp(wxString const& item) const;
		void moveUp(wxString const& item);

		bool canMoveDown(wxString const& item) const;
		void moveDown(wxString const& item);

		void hide(wxString const& item);
		void show(wxString const& item);
		void switchVisibility(wxString const& item);

		void remove(wxString const& item);
	};
}
