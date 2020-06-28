// SD Mod Manager

// Copyright (c) 2020 Aliaksei Karalenka <sydr1991@gmail.com>.
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#pragma once

#include <unordered_map>
#include <wx/string.h>

#include "interface/service/iicon_storage.h"

namespace mm
{
	class IconStorage : public IIconStorage
	{
	public:
		wxIcon get(const wxString& name) override;

		void remove(const wxString& name) override;

	private:
		std::unordered_map<wxString, wxIcon> _cache;
	};
}
